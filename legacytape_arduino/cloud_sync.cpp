// Supabase polling for onboarding_complete=true.
//
// Polls every 5 seconds. Uses anon key + a stored procedure that takes
// (hardware_id, pairing_token) and returns the row's onboarding_complete
// flag. The RPC is part of the companion app's backend:
//
//   create or replace function public.device_status(hw_id text, tok text)
//   returns table (onboarding_complete boolean)
//   language sql security definer stable as $$
//     select onboarding_complete from devices
//     where hardware_id = hw_id and pairing_token = tok
//     limit 1;
//   $$;
//
// If the RPC isn't deployed yet, this module logs failures and keeps polling
// — once it is deployed, the next poll succeeds and the screen transitions.
// No firmware reflash needed.

#include "cloud_sync.h"
#include "pairing.h"
#include "backend_config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const unsigned long POLL_INTERVAL_MS = 5000;
static volatile bool g_active     = false;
static unsigned int  g_poll_count = 0;
static TaskHandle_t  g_task       = nullptr;

static void cloud_poll_task(void *);

void cloud_sync_begin(void) {
    if (g_active) return;
    g_active = true;
    g_poll_count = 0;
    Serial.println("[cloud] polling started — waiting for onboarding_complete=true");
    // Poll on its OWN core-0 task, NOT the main loop. do_poll() does a blocking
    // HTTPS POST (TLS handshake can take hundreds of ms to seconds); running it
    // on loop() froze LVGL + touch every 5s, so taps on the pairing screen were
    // dropped and the UI "glitched". The task only sets a flag on success
    // (pairing_mark_complete); the actual screen change still happens on the
    // main loop via pairing_consume_complete_event(), so this stays LVGL-safe.
    xTaskCreatePinnedToCore(cloud_poll_task, "cloud_sync", 8192, NULL, 3, &g_task, 0);
}

void cloud_sync_stop(void) {
    if (g_active) Serial.println("[cloud] polling stopped");
    g_active = false;   // task observes this and exits on its own
}

static void do_poll(void) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[cloud] WiFi not connected, skipping poll");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();   // TODO: pin Supabase root cert for production

    HTTPClient http;
    String url = String(LT_SUPABASE_URL) + LT_EP_DEVICE_STATUS;
    if (!http.begin(client, url)) {
        Serial.println("[cloud] http.begin failed");
        return;
    }
    http.addHeader("apikey", LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Authorization", String("Bearer ") + LT_SUPABASE_PUBLISHABLE_KEY);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<192> req;
    req["hw_id"] = pairing_get_device_id();
    req["tok"]   = pairing_get_token_hex();
    String body;
    serializeJson(req, body);

    g_poll_count++;
    int code = http.POST(body);

    if (code == 200) {
        String resp = http.getString();
        StaticJsonDocument<256> respDoc;
        DeserializationError err = deserializeJson(respDoc, resp);
        if (err) {
            Serial.printf("[cloud] poll #%u: JSON parse error: %s\n", g_poll_count, err.c_str());
        } else {
            bool complete = false;
            // RPC returns an array of rows
            if (respDoc.is<JsonArray>()) {
                JsonArray arr = respDoc.as<JsonArray>();
                if (arr.size() > 0) {
                    complete = arr[0]["onboarding_complete"] | false;
                }
            } else {
                complete = respDoc["onboarding_complete"] | false;
            }

            if (complete) {
                Serial.println("[cloud] onboarding_complete=true — firing screen advance");
                pairing_mark_complete();
                g_active = false;
            } else if (g_poll_count % 6 == 1) {
                // Every ~30s, log we're still waiting
                Serial.printf("[cloud] poll #%u: onboarding not yet complete\n", g_poll_count);
            }
        }
    } else if (code == 404) {
        Serial.printf("[cloud] poll #%u: 404 — device_status RPC not deployed on the backend\n",
                      g_poll_count);
    } else {
        Serial.printf("[cloud] poll #%u: HTTP %d\n", g_poll_count, code);
        if (code < 0) {
            Serial.printf("[cloud] error: %s\n", http.errorToString(code).c_str());
        }
    }

    http.end();
}

// Background poller. Polls immediately, then every POLL_INTERVAL_MS, checking
// g_active frequently so cloud_sync_stop() tears it down within ~100ms.
static void cloud_poll_task(void *) {
    while (g_active) {
        do_poll();
        for (uint32_t w = 0; w < POLL_INTERVAL_MS && g_active; w += 100)
            vTaskDelay(pdMS_TO_TICKS(100));
    }
    Serial.println("[cloud] poll task exiting");
    g_task = nullptr;
    vTaskDelete(NULL);
}

// Polling now runs on its own task (see cloud_sync_begin). Kept as a no-op so
// the main loop's call site still links without edits to the .ino.
void cloud_sync_loop(void) {}
