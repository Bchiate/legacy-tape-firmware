// Continuous PDM mic capture into chunked PSRAM buffers.
#include "audio_record.h"
#include <Arduino.h>
#include <ESP_I2S.h>
#include <Wire.h>
#include <esp_random.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define MIC_CLK_PIN   19
#define MIC_DATA_PIN  20
#define NUM_BUFFERS   2

typedef enum {
    BUF_EMPTY,        // available for capture to fill
    BUF_FILLING,      // capture is writing into it
    BUF_READY,        // full, waiting for upload
    BUF_UPLOADING     // upload module has taken it
} buf_state_t;

typedef struct {
    uint8_t           *data;
    volatile size_t    used;
    volatile uint32_t  idx;
    volatile buf_state_t state;
} chunk_buf_t;

static I2SClass     g_i2s;
static chunk_buf_t  g_bufs[NUM_BUFFERS];
static volatile int g_active_idx = 0;
static volatile audio_state_t g_state = AUDIO_STATE_IDLE;
static volatile uint8_t  g_level = 0;
static volatile uint32_t g_chunks_captured = 0;
static volatile uint32_t g_total_pcm_bytes = 0;
static char        g_session_id[33] = {0};
static char        g_last_error[96]  = {0};
static TaskHandle_t g_task = nullptr;

static const char *hex = "0123456789abcdef";

static void new_session_id(void) {
    uint8_t r[16];
    esp_fill_random(r, sizeof(r));
    for (int i = 0; i < 16; i++) {
        g_session_id[i * 2]     = hex[(r[i] >> 4) & 0xF];
        g_session_id[i * 2 + 1] = hex[r[i] & 0xF];
    }
    g_session_id[32] = 0;
}

// Mic unmute + restore backlight. Both peripherals live on the STC8H1K28 µC
// at I2C 0x30, so the unmute byte sequence can put the µC in a state where
// backlight goes dark. Immediately after unmuting, re-send 0x10 (max
// brightness) so the screen doesn't black out mid-session.
static bool unmute_mic(void) {
    Wire.beginTransmission(0x30);
    Wire.write((uint8_t)0x00);
    Wire.write((uint8_t)0x17);
    uint8_t err = Wire.endTransmission();
    if (err != 0) Serial.printf("[audio] mic unmute I2C err=%u (continuing)\n", err);
    else          Serial.println("[audio] mic unmuted");

    // Restore backlight to max — same byte the boot sequence sends
    delay(10);
    Wire.beginTransmission(0x30);
    Wire.write((uint8_t)0x10);
    Wire.endTransmission();
    return true;
}

static void update_level(const uint8_t *buf, size_t n) {
    int16_t peak = 0;
    for (size_t i = 0; i + 1 < n; i += 2) {
        int16_t s = (int16_t)(buf[i] | (buf[i + 1] << 8));
        int16_t a = s < 0 ? -s : s;
        if (a > peak) peak = a;
    }
    // Normal speech peaks well below full-scale, so a raw peak/32768 barely
    // moves the meter. Reference against ~1/8 full-scale (4096) instead of
    // 32768 -> 8x more sensitive, then clamp. Makes the VU lively for talking.
    uint32_t pct = (uint32_t)peak * 100 / 4096;
    if (pct > 100) pct = 100;
    g_level = (uint8_t)pct;
}

static void capture_task(void *) {
    Serial.println("[audio] capture task started");

    // PDM mic warmup gap: the first ~500 ms after i2s.begin() in PDM mode
    // contains DC offset / saturation glitches that confuse Whisper.
    // Drain and discard before chunk 0 starts filling.
    const size_t WARMUP_BYTES = AUDIO_SAMPLE_RATE * 2 / 2;   // 0.5 s = 16 KB
    uint8_t discard[2048];
    size_t drained = 0;
    while (drained < WARMUP_BYTES && g_state == AUDIO_STATE_RECORDING) {
        size_t got = g_i2s.readBytes((char *)discard, sizeof(discard));
        drained += got;
        vTaskDelay(1);
    }
    Serial.printf("[audio] PDM warmup drained %u bytes\n", (unsigned)drained);

    // First chunk gets BUF_FILLING
    g_bufs[g_active_idx].used  = 0;
    g_bufs[g_active_idx].state = BUF_FILLING;

    while (g_state == AUDIO_STATE_RECORDING || g_state == AUDIO_STATE_FINALIZING) {
        chunk_buf_t *buf = &g_bufs[g_active_idx];

        if (buf->state != BUF_FILLING) {
            // Other buffer is still uploading; wait. (Rare; only happens on WiFi stall.)
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        size_t want = AUDIO_CHUNK_BYTES - buf->used;
        if (want > 4096) want = 4096;
        size_t got = g_i2s.readBytes((char *)(buf->data + buf->used), want);
        if (got > 0) {
            update_level(buf->data + buf->used, got);
            buf->used += got;
            g_total_pcm_bytes += got;
        }

        // Buffer full or finalizing → hand to upload, swap to next
        bool full = buf->used >= AUDIO_CHUNK_BYTES;
        bool finalizing_with_partial = (g_state == AUDIO_STATE_FINALIZING && buf->used > 0);
        if (full || finalizing_with_partial) {
            buf->idx = g_chunks_captured++;
            buf->state = BUF_READY;
            Serial.printf("[audio] chunk %u ready (%u bytes)\n",
                          (unsigned)buf->idx, (unsigned)buf->used);

            // Find next EMPTY buffer for capture
            int next = -1;
            for (int i = 0; i < NUM_BUFFERS; i++) {
                int probe = (g_active_idx + 1 + i) % NUM_BUFFERS;
                if (g_bufs[probe].state == BUF_EMPTY) { next = probe; break; }
            }
            if (next < 0) {
                // Both buffers tied up. Wait for upload to release one.
                Serial.println("[audio] all buffers in flight, capture stalling");
                while (g_state == AUDIO_STATE_RECORDING) {
                    bool found = false;
                    for (int i = 0; i < NUM_BUFFERS; i++) {
                        if (g_bufs[i].state == BUF_EMPTY) {
                            next = i;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
            if (g_state == AUDIO_STATE_RECORDING && next >= 0) {
                g_active_idx = next;
                g_bufs[next].used  = 0;
                g_bufs[next].state = BUF_FILLING;
            } else if (g_state == AUDIO_STATE_FINALIZING) {
                // No more captures; just exit
                break;
            }
        }
    }

    g_i2s.end();
    Serial.println("[audio] capture task exiting");
    g_task = nullptr;
    vTaskDelete(NULL);
}

bool audio_record_begin(void) {
    if (g_bufs[0].data) return true;
    for (int i = 0; i < NUM_BUFFERS; i++) {
        g_bufs[i].data = (uint8_t *)heap_caps_malloc(AUDIO_CHUNK_BYTES, MALLOC_CAP_SPIRAM);
        if (!g_bufs[i].data) {
            Serial.println("[audio] PSRAM alloc failed");
            g_state = AUDIO_STATE_ERROR;
            return false;
        }
        g_bufs[i].used  = 0;
        g_bufs[i].state = BUF_EMPTY;
    }
    Serial.printf("[audio] %d x %u byte chunk buffers in PSRAM\n",
                  NUM_BUFFERS, (unsigned)AUDIO_CHUNK_BYTES);
    // NOTE: unmute_mic() is NOT called here anymore. The unmute byte sequence
    // shares an I2C address with the backlight µC and was killing the screen
    // at boot. We unmute on demand at audio_record_start() instead.
    return true;
}

bool audio_record_start(void) {
    if (!g_bufs[0].data && !audio_record_begin()) return false;
    if (g_state == AUDIO_STATE_RECORDING) return false;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        g_bufs[i].used  = 0;
        g_bufs[i].state = BUF_EMPTY;
    }
    g_active_idx       = 0;
    g_chunks_captured  = 0;
    g_total_pcm_bytes  = 0;
    g_level            = 0;
    g_last_error[0]    = 0;
    new_session_id();

    // Unmute mic + immediately restore backlight in case the unmute disturbs
    // the shared I2C 0x30 µC.
    unmute_mic();

    g_i2s.setPinsPdmRx(MIC_CLK_PIN, MIC_DATA_PIN);
    if (!g_i2s.begin(I2S_MODE_PDM_RX, AUDIO_SAMPLE_RATE,
                     I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
        Serial.println("[audio] I2S PDM begin failed");
        g_state = AUDIO_STATE_ERROR;
        return false;
    }
    g_state = AUDIO_STATE_RECORDING;
    Serial.printf("[audio] session %s started\n", g_session_id);

    xTaskCreatePinnedToCore(capture_task, "audio_cap", 4096, NULL, 5, &g_task, 1);
    return true;
}

void audio_record_stop(void) {
    if (g_state != AUDIO_STATE_RECORDING) return;
    Serial.println("[audio] STOP — entering FINALIZING");
    g_state = AUDIO_STATE_FINALIZING;
    // capture task will flush its partial buffer and exit
}

audio_state_t  audio_record_state(void)             { return g_state; }
uint32_t       audio_record_seconds(void)           { return g_total_pcm_bytes / (AUDIO_SAMPLE_RATE * 2); }
uint8_t        audio_record_level_percent(void)     { return g_level; }
const char    *audio_record_session_id(void)        { return g_session_id; }
uint32_t       audio_record_chunks_captured(void)   { return g_chunks_captured; }
const char    *audio_record_last_error(void)        { return g_last_error; }
bool           audio_record_capture_active(void)    { return g_task != nullptr; }

void audio_record_force_stop_for_network(const char *reason) {
    if (g_state != AUDIO_STATE_RECORDING) return;
    strncpy(g_last_error, reason ? reason : "network failure", sizeof(g_last_error) - 1);
    g_last_error[sizeof(g_last_error) - 1] = 0;
    Serial.printf("[audio] FORCE STOP: %s\n", g_last_error);
    // Mark ERROR so capture_task exits its loop AND skips queuing partial chunk
    g_state = AUDIO_STATE_ERROR;
}

// Called by audio_upload after the finalize POST succeeds (or is skipped for
// an empty session) to advance state machine FINALIZING -> COMPLETE so the
// UI can show "Uploaded" instead of "Uploading…".
void audio_record_mark_complete(void) {
    if (g_state == AUDIO_STATE_FINALIZING) {
        g_state = AUDIO_STATE_COMPLETE;
        Serial.println("[audio] recording fully uploaded + finalized");
    }
}

// Pulled by audio_upload — return next READY chunk if any
bool audio_record_take_chunk(audio_chunk_t *out) {
    // A take aborted by audio_record_force_stop_for_network() is never
    // finalized, so its leftover chunks are not uploaded. This also guarantees
    // the uploader holds no buffer when the next audio_record_start() resets them.
    if (g_state == AUDIO_STATE_ERROR) return false;
    int best = -1;
    uint32_t best_idx = UINT32_MAX;
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (g_bufs[i].state == BUF_READY && g_bufs[i].idx < best_idx) {
            best     = i;
            best_idx = g_bufs[i].idx;
        }
    }
    if (best < 0) return false;
    g_bufs[best].state = BUF_UPLOADING;
    out->data      = g_bufs[best].data;
    out->size      = g_bufs[best].used;
    out->idx       = g_bufs[best].idx;
    out->_internal = (void *)(intptr_t)best;
    return true;
}

void audio_record_release_chunk(const audio_chunk_t *chunk) {
    int idx = (int)(intptr_t)chunk->_internal;
    if (idx < 0 || idx >= NUM_BUFFERS) return;
    g_bufs[idx].used  = 0;
    g_bufs[idx].state = BUF_EMPTY;
}
