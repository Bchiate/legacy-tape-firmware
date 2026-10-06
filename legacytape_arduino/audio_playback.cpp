// Streaming playback: a whole chapter (oldest->newest takes) -> I2S speaker.
//
// get_recording now returns the full chapter timeline:
//   { found, count, total_duration_sec, recordings: [ {url, duration_sec}, ... ] }
// We play the clips back to back as one continuous timeline. I2S stays open
// across clips (all are 16k/mono/16-bit) so there's no gap/pop between takes.
#include "audio_playback.h"
#include "pairing.h"
#include "backend_config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESP_I2S.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/ringbuf.h>

// ArduinoJson allocator that lives in PSRAM, so parsing the get_recording
// response never depends on a large contiguous block of (fragmented) internal
// heap — which was making deserializeJson fail with a "bad JSON" even on a
// small, valid response after BLE/WiFi/audio had chewed up internal RAM.
struct SpiRamAllocator : ArduinoJson::Allocator {
    void *allocate(size_t size) override   { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
    void  deallocate(void *p) override      { heap_caps_free(p); }
    void *reallocate(void *p, size_t s) override { return heap_caps_realloc(p, s, MALLOC_CAP_SPIRAM); }
};

#define SPK_BCLK 5
#define SPK_LRC  6
#define SPK_DOUT 4
#define PLAY_SAMPLE_RATE 16000

#define MAX_CLIPS  64
// Supabase signed URLs are /storage/v1/object/sign/...?token=<JWT>; the JWT
// alone is ~200 chars, so the full URL runs ~300-380. 768 gives safe headroom
// — a truncated URL would silently fail the GET.
#define URL_LEN    768
typedef struct { char url[URL_LEN]; uint32_t dur; } clip_t;

static I2SClass        g_spk;
static volatile playback_state_t g_state = PLAYBACK_IDLE;
static volatile uint32_t g_pos_sec  = 0;
static volatile uint32_t g_dur_sec  = 0;
static volatile bool     g_stop_req = false;
static char              g_err[96]  = {0};
static int               g_chapter  = 0;
static TaskHandle_t      g_task     = nullptr;

static clip_t *g_clips = nullptr;     // PSRAM
static int     g_clip_count = 0;

// ─── Volume (software scale, persisted) ─────────────────────────────────────
// Default 70%: the amp adds ~9 dB of fixed hardware gain, so full-scale
// samples clip hard; the right-channel-only output (see playback_task) costs
// some level, hence 70 rather than lower. User-adjustable on Screen7.
static int g_volume = -1;   // -1 = not loaded from NVS yet

static void volume_load(void) {
    if (g_volume >= 0) return;
    Preferences p;
    p.begin("ltvol", true);
    g_volume = p.getInt("v", 70);   // right-channel-only may halve level; start higher
    p.end();
    if (g_volume < 0) g_volume = 0;
    if (g_volume > 100) g_volume = 100;
}

int audio_playback_get_volume(void) { volume_load(); return g_volume; }

void audio_playback_set_volume(int vol) {
    if (vol < 0) vol = 0; if (vol > 100) vol = 100;
    g_volume = vol;
    Preferences p;
    p.begin("ltvol", false);
    p.putInt("v", g_volume);
    p.end();
}

void audio_playback_volume_step(int delta) {
    volume_load();
    audio_playback_set_volume(g_volume + delta);
}

static void set_err(const char *m) {
    strncpy(g_err, m, sizeof(g_err) - 1);
    g_err[sizeof(g_err) - 1] = 0;
    Serial.printf("[playback] error: %s\n", g_err);
}

// Fetch the chapter's clip list. Fills g_clips/g_clip_count/g_dur_sec.
// Returns: 1 = have clips, 0 = empty chapter (found:false), -1 = error.
static int fetch_chapter(int chapter) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(20);
    HTTPClient http;
    http.setTimeout(20000);
    String url = String(LT_SUPABASE_URL) + LT_EP_GET_RECORDING;
    if (!http.begin(client, url)) { set_err("http.begin failed"); return -1; }
    http.addHeader("apikey",        LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Authorization", String("Bearer ") + LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Content-Type",  "application/json");
    http.addHeader("x-hardware-id", pairing_get_device_id());
    http.addHeader("x-pair-token",  pairing_get_token_hex());
    http.addHeader("x-chapter",     String(chapter));

    int code = http.POST("{}");
    if (code != 200) {
        char m[48]; snprintf(m, sizeof(m), "get_recording HTTP %d", code);
        set_err(m); http.end(); return -1;
    }
    String resp = http.getString();
    http.end();

    // Parse into a PSRAM-backed, elastic JsonDocument so a fragmented internal
    // heap can't sink the parse. Log the exact error + response head on failure
    // so a real malformed/short response is diagnosable instead of opaque.
    static SpiRamAllocator psram_alloc;
    JsonDocument doc(&psram_alloc);
    DeserializationError jerr = deserializeJson(doc, resp);
    if (jerr) {
        Serial.printf("[playback] JSON parse error: %s | %u bytes | head: %.120s\n",
                      jerr.c_str(), (unsigned)resp.length(), resp.c_str());
        set_err("bad JSON from get_recording");
        return -1;
    }
    if (!(doc["found"] | false)) return 0;

    g_dur_sec = doc["total_duration_sec"] | 0;
    JsonArray arr = doc["recordings"].as<JsonArray>();
    g_clip_count = 0;
    for (JsonObject c : arr) {
        if (g_clip_count >= MAX_CLIPS) break;
        const char *u = c["url"] | (const char *)nullptr;
        if (!u) continue;
        strncpy(g_clips[g_clip_count].url, u, URL_LEN - 1);
        g_clips[g_clip_count].url[URL_LEN - 1] = 0;
        g_clips[g_clip_count].dur = c["duration_sec"] | 0;
        g_clip_count++;
    }
    if (g_clip_count == 0) return 0;
    if (g_dur_sec == 0) {  // fall back to summing if server omitted total
        for (int i = 0; i < g_clip_count; i++) g_dur_sec += g_clips[i].dur;
    }
    Serial.printf("[playback] chapter has %d clip(s), total %us\n", g_clip_count, (unsigned)g_dur_sec);
    return 1;
}

// ─── Streaming ring buffer (producer = network, consumer = I2S) ─────────────
// A bounded PSRAM byte buffer decouples network jitter from I2S timing, so
// playback is smooth AND unlimited length — memory stays fixed regardless of
// recording duration. The producer task GETs each clip and pushes raw PCM into
// the ring; the consumer (playback_task) drains it to the speaker at a steady
// rate. Sample alignment is reassembled across ring boundaries so an odd-byte
// network read can't byte-swap the audio into static.
#define RING_BYTES      (320 * 1024)     // ~10s of 16k/mono/16-bit cushion
#define PREBUFFER_BYTES (96 * 1024)      // ~3s buffered before audio starts

static RingbufHandle_t   g_ring = nullptr;
static StaticRingbuffer_t g_ring_struct;          // control block (internal RAM)
static uint8_t          *g_ring_storage = nullptr; // data area (PSRAM)
static volatile bool    g_producer_done = false;
static TaskHandle_t     g_producer = nullptr;

// Producer: stream every clip's PCM body into the ring (skipping each 44-byte
// WAV header). Blocks on xRingbufferSend when the ring is full — that's the
// backpressure that paces the network to the speaker.
static void producer_task(void *) {
    for (int i = 0; i < g_clip_count && !g_stop_req; i++) {
        Serial.printf("[playback] producer clip %d/%d\n", i + 1, g_clip_count);
        WiFiClientSecure client;
        client.setInsecure();
        client.setTimeout(20);
        HTTPClient http;
        http.setTimeout(20000);
        http.begin(client, g_clips[i].url);
        const char *hdrs[] = {"Location"};
        http.collectHeaders(hdrs, 1);
        int code = http.GET();
        if (code == 301 || code == 302 || code == 307) {
            String loc = http.header("Location");
            http.end(); http.begin(client, loc); code = http.GET();
        }
        if (code != 200) {
            Serial.printf("[playback] producer clip GET HTTP %d (skip)\n", code);
            http.end();
            continue;
        }
        WiFiClient *stream = http.getStreamPtr();
        stream->setTimeout(3000);

        // Skip 44-byte WAV header
        uint8_t hdr[44]; int hr = 0;
        while (hr < 44 && !g_stop_req) {
            int r = stream->readBytes(hdr + hr, 44 - hr);
            if (r <= 0) { if (!stream->connected()) break; vTaskDelay(1); continue; }
            hr += r;
        }
        // Body -> ring
        uint8_t buf[2048];
        while (!g_stop_req) {
            int n = stream->readBytes(buf, sizeof(buf));
            if (n <= 0) {
                if (!stream->connected() && stream->available() == 0) break;  // clip EOF
                vTaskDelay(1);   // no data yet — yield, don't busy-spin
                continue;
            }
            // Block until the ring has room (backpressure paces the download),
            // but retry on a timeout so a stop request can't deadlock us when
            // the consumer has stopped draining a full ring.
            while (!g_stop_req && xRingbufferSend(g_ring, buf, n, pdMS_TO_TICKS(200)) != pdTRUE) {}
            if (g_stop_req) break;
            // CRITICAL: yield every iteration so the CPU-0 IDLE task runs and
            // feeds the task watchdog. Without this, when network data + ring
            // space are both ready (e.g. while filling the prebuffer) the loop
            // is CPU-bound and starves IDLE0 -> watchdog panic + reboot.
            vTaskDelay(1);
        }
        http.end();
    }
    g_producer_done = true;
    Serial.println("[playback] producer done");
    g_producer = nullptr;
    vTaskDelete(NULL);
}

static void playback_task(void *) {
    g_state = PLAYBACK_FETCHING;
    g_pos_sec = 0;
    g_err[0] = 0;
    g_producer_done = false;
    volume_load();

    int r = fetch_chapter(g_chapter);
    if (r == 0) { g_state = PLAYBACK_NONE;  g_task = nullptr; vTaskDelete(NULL); return; }
    if (r < 0)  { g_state = PLAYBACK_ERROR; g_task = nullptr; vTaskDelete(NULL); return; }

    // Ring buffer: control block in internal RAM, data area in PSRAM. (The
    // ...WithCaps helper isn't in this core; CreateStatic with our own PSRAM
    // storage gives the same result and is portable.)
    if (!g_ring) {
        if (!g_ring_storage)
            g_ring_storage = (uint8_t *)heap_caps_malloc(RING_BYTES, MALLOC_CAP_SPIRAM);
        if (g_ring_storage)
            g_ring = xRingbufferCreateStatic(RING_BYTES, RINGBUF_TYPE_BYTEBUF,
                                             g_ring_storage, &g_ring_struct);
    }
    if (!g_ring) { set_err("ring alloc failed"); g_state = PLAYBACK_ERROR; g_task = nullptr; vTaskDelete(NULL); return; }

    // Unmute the panel audio path (0x30 µC) + restore backlight, then open I2S.
    Wire.beginTransmission(0x30); Wire.write((uint8_t)0x00); Wire.write((uint8_t)0x17); Wire.endTransmission();
    delay(10);
    Wire.beginTransmission(0x30); Wire.write((uint8_t)0x10); Wire.endTransmission();

    g_spk.setPins(SPK_BCLK, SPK_LRC, SPK_DOUT);
    if (!g_spk.begin(I2S_MODE_STD, PLAY_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
        set_err("I2S output begin failed");
        g_state = PLAYBACK_ERROR; g_task = nullptr; vTaskDelete(NULL); return;
    }

    // Start the network producer (own task so it can stay ahead of the speaker)
    // 12KB stack: the producer does an HTTPS (TLS/mbedTLS) GET per clip, which
    // is stack-hungry; 8KB risked a stack overflow (crash/glitch).
    xTaskCreatePinnedToCore(producer_task, "play_prod", 12288, NULL, 5, &g_producer, 0);

    // Prebuffer: wait until we've got a cushion (or the producer already finished
    // a short clip) before letting audio start — prevents an immediate underrun.
    g_state = PLAYBACK_PLAYING;
    {
        uint32_t t0 = millis();
        while (!g_stop_req
               && xRingbufferGetCurFreeSize(g_ring) > (RING_BYTES - PREBUFFER_BYTES)
               && !g_producer_done
               && millis() - t0 < 8000) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    // Consumer: drain ring -> stereo+volume -> I2S, steadily. Reassemble
    // aligned 16-bit samples across ring chunks via a 1-byte carry.
    // Read cap is 1024 bytes => up to 512 mono samples => 512 stereo frames =>
    // 1024 int16 in `stereo`. (The earlier crackle/crash was a 2x overflow of
    // this buffer when reads were up to ~2KB but stereo[] was only 1024.)
    #define READ_CAP 1024
    uint8_t  carry = 0; bool has_carry = false;
    int16_t  stereo[READ_CAP];           // 512 frames * 2 ch = 1024 int16
    uint64_t played = 0;
    const uint64_t bps = PLAY_SAMPLE_RATE * 2;

    while (!g_stop_req) {
        size_t rn = 0;
        // Pull up to READ_CAP-2 bytes (room for the 1-byte carry, keeps the
        // assembled buffer within `stereo` after mono->stereo expansion).
        uint8_t *rp = (uint8_t *)xRingbufferReceiveUpTo(g_ring, &rn, pdMS_TO_TICKS(200), READ_CAP - 2);
        if (rp == nullptr) {
            if (g_producer_done) break;   // producer finished + ring drained
            continue;                     // still buffering
        }

        // Assemble with carry so sample boundaries stay aligned across reads.
        // Worst case: 1 carry + rn bytes. Process complete 16-bit samples.
        static uint8_t mono[2048];
        int mlen = 0;
        if (has_carry) { mono[mlen++] = carry; has_carry = false; }
        memcpy(mono + mlen, rp, rn);
        mlen += rn;
        vRingbufferReturnItem(g_ring, rp);

        int samples = mlen / 2;
        if (mlen & 1) { carry = mono[mlen - 1]; has_carry = true; }   // stash odd byte

        // Audio on the RIGHT channel only, LEFT silent — EXACTLY matching the
        // CrowPanel factory playback. Putting the same sample on both channels
        // distorts because the panel's amp path sums L+R (= 2x = hard clipping),
        // which is the extreme distortion we heard. Right-only avoids that.
        int vol = g_volume;
        for (int i = 0; i < samples; i++) {
            int16_t raw = (int16_t)(mono[i * 2] | (mono[i * 2 + 1] << 8));
            int16_t s = (int16_t)(((int32_t)raw * vol) / 100);
            stereo[i * 2]     = 0;   // left muted
            stereo[i * 2 + 1] = s;   // right carries the audio
        }
        if (samples > 0) g_spk.write((uint8_t *)stereo, samples * 4);
        played += samples * 2;
        g_pos_sec = (uint32_t)(played / bps);
    }

    if (g_producer) g_stop_req = true;   // ensure producer also winds down

    // Flush ~120 ms of silence so the I2S DMA drains cleanly before deinit —
    // avoids the end-of-playback pop AND lets the DMA finish in flight so
    // tearing it down doesn't disrupt the RGB panel's DMA (which was flashing
    // the screen black at the end).
    memset(stereo, 0, sizeof(stereo));
    for (int k = 0; k < 4; k++) g_spk.write((uint8_t *)stereo, sizeof(stereo));
    delay(20);
    g_spk.end();

    g_state = g_stop_req ? PLAYBACK_IDLE : PLAYBACK_DONE;
    Serial.printf("[playback] chapter finished at %us\n", (unsigned)g_pos_sec);
    g_task = nullptr;
    vTaskDelete(NULL);
}

void audio_playback_start(int chapter_idx) {
    if (g_state == PLAYBACK_PLAYING || g_state == PLAYBACK_FETCHING) return;
    if (WiFi.status() != WL_CONNECTED) { set_err("WiFi not connected"); g_state = PLAYBACK_ERROR; return; }
    // If a previous playback's tasks are still tearing down (stop is
    // non-blocking), wait briefly so we don't run two consumers on one I2S.
    for (int i = 0; i < 60 && (g_task != nullptr || g_producer != nullptr); i++) delay(10);
    if (g_task != nullptr || g_producer != nullptr) { set_err("still stopping previous playback"); return; }
    if (!g_clips) {
        g_clips = (clip_t *)heap_caps_malloc(sizeof(clip_t) * MAX_CLIPS, MALLOC_CAP_SPIRAM);
        if (!g_clips) { set_err("PSRAM alloc failed"); g_state = PLAYBACK_ERROR; return; }
    }
    g_chapter  = chapter_idx;
    g_stop_req = false;
    g_pos_sec  = 0;
    g_dur_sec  = 0;
    // Consumer (ring -> I2S) on CORE 1, away from the network producer + WiFi
    // on core 0 — the key anti-underrun move. Priority 2: above the Arduino/
    // LVGL loop (1) so audio wins the DMA-refill race, but not so high it
    // starves the display flush + idle/watchdog (priority 3 was glitching the
    // RGB panel black near the end). It blocks on i2s.write yielding to LVGL.
    xTaskCreatePinnedToCore(playback_task, "audio_play", 12288, NULL, 2, &g_task, 1);
}

void audio_playback_stop(void) {
    // NON-BLOCKING. This is called from the LVGL event handler (main loop) when
    // the user taps STOP / leaves Screen7. The old version blocked up to 800ms
    // waiting for the tasks to exit, which froze LVGL — that was the
    // black-screen-for-a-second glitch on stop. Just signal; the producer +
    // consumer tasks see g_stop_req, tear down I2S/HTTP themselves, and exit.
    // They only touch module-static state (ring, g_spk), never LVGL objects,
    // so it's safe for them to finish after the screen has moved on.
    g_stop_req = true;
}

playback_state_t audio_playback_state(void)        { return g_state; }
uint32_t         audio_playback_position_sec(void) { return g_pos_sec; }
uint32_t         audio_playback_duration_sec(void) { return g_dur_sec; }
const char      *audio_playback_last_error(void)   { return g_err; }
