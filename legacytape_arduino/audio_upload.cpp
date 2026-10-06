// Pulls chunks from audio_record + POSTs them to Supabase upload_chunk.
// After audio_record signals finalize, sends finalize_recording.
#include "audio_upload.h"
#include "audio_record.h"
#include "pairing.h"
#include "book.h"
#include "backend_config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static volatile bool        g_running     = false;
static volatile uint32_t    g_uploaded    = 0;
static char                 g_err[96]     = {0};
static TaskHandle_t         g_task        = nullptr;
static volatile bool        g_finalize_pending = false;
static volatile uint32_t    g_final_duration   = 0;
static volatile int         g_final_chapter    = 0;

static void set_err(const char *m) {
    strncpy(g_err, m, sizeof(g_err) - 1);
    g_err[sizeof(g_err) - 1] = 0;
    Serial.printf("[upload] error: %s\n", g_err);
}

static bool post_chunk(const audio_chunk_t *chunk) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30);

    HTTPClient http;
    http.setTimeout(30000);
    String url = String(LT_SUPABASE_URL) + LT_EP_UPLOAD_CHUNK;
    if (!http.begin(client, url)) {
        set_err("http.begin failed");
        return false;
    }
    http.addHeader("apikey",        LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Authorization", String("Bearer ") + LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Content-Type",  "application/octet-stream");
    http.addHeader("x-hardware-id", pairing_get_device_id());
    http.addHeader("x-pair-token",  pairing_get_token_hex());
    http.addHeader("x-session-id",  audio_record_session_id());
    http.addHeader("x-chunk-idx",   String(chunk->idx));

    int code = http.POST(chunk->data, chunk->size);
    bool ok = (code >= 200 && code < 300);
    if (!ok) {
        char msg[80];
        if (code == 404)
            snprintf(msg, sizeof(msg), "Cloud upload not enabled yet (server 404)");
        else if (code < 0)
            snprintf(msg, sizeof(msg), "Network error: %s", http.errorToString(code).c_str());
        else
            snprintf(msg, sizeof(msg), "Upload rejected (HTTP %d)", code);
        set_err(msg);
    } else {
        Serial.printf("[upload] chunk %u OK (%u bytes)\n", (unsigned)chunk->idx, (unsigned)chunk->size);
    }
    http.end();
    return ok;
}

static bool post_finalize(uint32_t duration, int chapter_idx) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30);
    HTTPClient http;
    http.setTimeout(30000);
    String url = String(LT_SUPABASE_URL) + LT_EP_FINALIZE_RECORDING;
    if (!http.begin(client, url)) return false;
    http.addHeader("apikey",        LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Authorization", String("Bearer ") + LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Content-Type",  "application/json");
    http.addHeader("x-hardware-id", pairing_get_device_id());
    http.addHeader("x-pair-token",  pairing_get_token_hex());
    http.addHeader("x-session-id",  audio_record_session_id());
    http.addHeader("x-acct",        pairing_get_account());
    http.addHeader("x-duration",    String(duration));
    http.addHeader("x-chapter",     String(chapter_idx));
    http.addHeader("x-book-name",   book_get_name());
    const char *cn = book_get_chapter_name(chapter_idx);
    http.addHeader("x-chapter-name", cn ? cn : "Chapter");

    int code = http.POST("{}");
    bool ok = (code >= 200 && code < 300);
    if (ok) {
        String resp = http.getString();
        Serial.printf("[upload] finalize OK: %s\n", resp.c_str());
    } else {
        char m[64];
        snprintf(m, sizeof(m), "finalize HTTP %d", code);
        set_err(m);
    }
    http.end();
    return ok;
}

// Network-failure threshold: after this many consecutive failures while
// recording is active, force-stop capture and surface the error on screen.
// At 2 s between retries this is ~30 s of trying before we give up.
#define UPLOAD_FAILURE_LIMIT 15

static void upload_task(void *) {
    Serial.println("[upload] task started");
    uint32_t consecutive_failures = 0;

    while (g_running) {
        if (WiFi.status() != WL_CONNECTED) {
            // WiFi dropped (coexistence, BLE teardown, power blip, etc).
            // Actively reconnect with the credentials stored at pairing
            // instead of just failing — recording audio shouldn't be lost
            // because WiFi briefly went away.
            const char *ssid = pairing_get_wifi_ssid();
            if (strlen(ssid) > 0) {
                Serial.printf("[upload] WiFi down — reconnecting to '%s'\n", ssid);
                WiFi.begin(ssid, pairing_get_wifi_pw());
                uint32_t t0 = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - t0 < 6000) delay(100);
                if (WiFi.status() != WL_CONNECTED) {
                    const char *ssid2 = pairing_get_wifi_ssid2();
                    if (strlen(ssid2) > 0) {
                        WiFi.begin(ssid2, pairing_get_wifi_pw2());
                        t0 = millis();
                        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 6000) delay(100);
                    }
                }
            }
            if (WiFi.status() == WL_CONNECTED) { consecutive_failures = 0; continue; }
            consecutive_failures++;
            if (audio_record_state() == AUDIO_STATE_RECORDING && consecutive_failures >= UPLOAD_FAILURE_LIMIT) {
                audio_record_force_stop_for_network("WiFi disconnected");
                consecutive_failures = 0;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        audio_chunk_t chunk;
        if (audio_record_take_chunk(&chunk)) {
            if (post_chunk(&chunk)) {
                g_uploaded++;
                consecutive_failures = 0;
                audio_record_release_chunk(&chunk);
            } else {
                consecutive_failures++;
                audio_record_release_chunk(&chunk);
                if (audio_record_state() == AUDIO_STATE_RECORDING && consecutive_failures >= UPLOAD_FAILURE_LIMIT) {
                    audio_record_force_stop_for_network("upload failures exceeded threshold");
                    consecutive_failures = 0;
                }
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            continue;
        }

        // No chunks pending. Send finalize if capture is done AND we actually
        // uploaded something. Don't finalize an empty session — leave it for
        // server-side janitor cleanup.
        if (g_finalize_pending && audio_record_state() == AUDIO_STATE_FINALIZING) {
            // CRITICAL: don't judge the session until the capture task has
            // finished flushing. On a short recording (< one 10s chunk) the
            // only chunk is the final partial one, flushed when capture exits.
            // If we evaluate before that, we race ahead and wrongly report
            // "nothing uploaded". Wait for capture to finish, then loop back
            // so take_chunk() at the top grabs + uploads the partial chunk.
            if (audio_record_capture_active()) {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
            if (g_uploaded == 0) {
                // Capture is done and genuinely nothing uploaded — real failure.
                Serial.println("[upload] FAILED: 0 chunks uploaded");
                if (g_err[0] == 0) set_err("Nothing uploaded - check connection");
                g_finalize_pending = false;
                audio_record_mark_complete();   // advance state so UI doesn't hang
            } else if (post_finalize(g_final_duration, g_final_chapter)) {
                g_finalize_pending = false;
                audio_record_mark_complete();
            } else {
                vTaskDelay(pdMS_TO_TICKS(3000));   // retry finalize
            }
            continue;
        }

        // If state is ERROR, give up cleanly — don't try to finalize a force-stopped session
        if (audio_record_state() == AUDIO_STATE_ERROR) {
            g_finalize_pending = false;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
    Serial.println("[upload] task exiting");
    g_task = nullptr;
    vTaskDelete(NULL);
}

void audio_upload_begin(void) {
    if (g_running) return;
    g_running = true;
    g_uploaded = 0;
    g_err[0] = 0;
    // Pin to core 0 (shares with the WiFi/BLE stack) so the blocking TLS
    // POSTs don't starve the Arduino loop + LVGL on core 1. Without this,
    // every upload's ~1-2s TLS handshake freezes the UI — most visibly the
    // VU meter, which stops updating mid-recording.
    xTaskCreatePinnedToCore(upload_task, "audio_up", 8192, NULL, 4, &g_task, 0);
}

void audio_upload_stop(void) {
    g_running = false;
}

void audio_upload_request_finalize(uint32_t duration_sec, int chapter_idx) {
    g_final_duration   = duration_sec;
    g_final_chapter    = chapter_idx;
    g_finalize_pending = true;
    Serial.printf("[upload] finalize queued: %us, chapter %d\n",
                  (unsigned)duration_sec, chapter_idx);
}

uint32_t    audio_upload_chunks_uploaded(void) { return g_uploaded; }
const char *audio_upload_last_error(void)      { return g_err; }

// Reset per-recording counters. Called when a new recording starts so the
// uploaded-count + error from the previous take don't leak into this one
// (the g_uploaded==0 'nothing uploaded' check depends on a fresh count).
void audio_upload_reset(void) {
    g_uploaded = 0;
    g_err[0] = 0;
    g_finalize_pending = false;
}
