// BLE pairing receiver — the GATT contract shared with the companion iOS app
// (also documented in the README).
//
// Connection flow:
//   1. App scans for service 1ec0de7a-...0001
//   2. App connects, discovers 3 characteristics:
//        ...0002 (write)  — credentials payload
//        ...0003 (notify) — status bytes
//        ...0004 (read/notify) — JSON array of visible WiFi networks
//   3. App reads ...0004 to populate its WiFi picker
//   4. App subscribes to notifications on ...0003
//   5. App writes JSON to ...0002:
//        {"token":"...","ssid":"...","pw":"...","acct":"...","ssid2":"...","pw2":"..."}
//      ssid2/pw2 are optional ("setup for someone else" flow).
//   6. Device validates token  → 0x01
//   7. Device stores creds in NVS → 0x02
//   8. Device tries WiFi.begin(primary), falls back to secondary
//   9. On WiFi success: 0x03  (NOT yet "paired" from device's POV — the iOS app
//      still has to walk the user through onboarding + write onboarding_complete=true
//      to Supabase. The cloud_sync module polls for that and triggers screen
//      advance. See cloud_sync.cpp.)
//
// JSON parsing uses ArduinoJson (must be installed via Arduino Library Manager).

#include "pairing_ble.h"
#include "pairing.h"
#include "cloud_sync.h"            // declares cloud_sync_begin() with C linkage
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static BLEServer         *g_server        = nullptr;
static BLECharacteristic *g_pair_char     = nullptr;
static BLECharacteristic *g_status_char   = nullptr;
static BLECharacteristic *g_wifilist_char = nullptr;
static volatile bool      g_client_connected = false;
static bool               g_wifi_scan_published = false;
static volatile bool      g_scan_requested = false;   // a callback asked for a scan
static volatile bool      g_creds_received = false;   // creds in -> stop scanning (scan disconnects WiFi)
// True while wifi_try() is mid-connect. Main loop checks this and skips
// lv_timer_handler() so the RGB panel DMA doesn't fight WiFi's PSRAM use.
static volatile bool      g_busy = false;
static uint32_t           g_busy_since = 0;     // millis() when g_busy went true
#define BUSY_MAX_MS       90000                 // hard ceiling: never freeze >90s

// ─── Status notifications ───────────────────────────────────────────────────
static void notify_status(uint8_t code) {
    if (!g_status_char) return;
    uint8_t b = code;
    g_status_char->setValue(&b, 1);
    g_status_char->notify();
    Serial.printf("[ble] status notify: 0x%02X\n", code);
    delay(20);
}

// ─── WiFi list characteristic ───────────────────────────────────────────────
// Request a scan. The actual scan runs synchronously in pairing_ble_loop
// (main-loop context) — NOT here, because callbacks run in the BLE task and a
// blocking scan there would stall the BLE stack. Async scans proved unreliable
// while a BLE connection is active (the scan-done callback often never fires
// under radio coexistence), which is why reads were returning the empty [].
// Last good (non-empty) scan result, as ready-to-serve JSON. Persists across
// BLE connects so even if a live re-scan returns 0 under coexistence, we still
// serve the real networks found at boot. Starts empty.
static char   g_cached_list[512] = "[]";
static size_t g_cached_len = 2;

static void request_wifi_scan() {
    g_scan_requested = true;
    g_wifi_scan_published = false;
}

// ESP32-S3 radio is 2.4GHz-only, so scan results are inherently networks the
// device can join — no 5GHz filtering needed (it can't see them).
//
// A BLE characteristic value maxes at 512 bytes (ATT). We keep JSON under ~480
// bytes, strongest-RSSI-first, so a reassembled read returns the COMPLETE list.
#define WIFI_JSON_MAX 480

// Run a SYNCHRONOUS scan and, if it finds networks, rebuild g_cached_list.
// Returns the network count (<=0 means scan found nothing / failed — common
// under live-BLE coexistence, in which case g_cached_list keeps its last good
// value). Applies the documented coexistence mitigations: STA mode, WiFi power
// save OFF, a settle delay, fresh scanDelete before scanning.
static int scan_and_cache(void) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);          // WIFI_PS_NONE — power save breaks coex scans
    delay(120);                    // let the radio settle before scanning
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false /*sync*/, false /*hidden*/);
    Serial.printf("[ble] scan_and_cache: %d network(s)\n", n);
    if (n <= 0) { WiFi.scanDelete(); return n; }

    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.to<JsonArray>();
    int order[40];
    int cnt = (n > 40) ? 40 : n;
    for (int i = 0; i < cnt; i++) order[i] = i;
    for (int i = 0; i < cnt - 1; i++) {              // sort by RSSI desc
        int best = i;
        for (int j = i + 1; j < cnt; j++)
            if (WiFi.RSSI(order[j]) > WiFi.RSSI(order[best])) best = j;
        int tmp = order[i]; order[i] = order[best]; order[best] = tmp;
    }
    for (int k = 0; k < cnt; k++) {
        String ssid = WiFi.SSID(order[k]);
        if (ssid.length() == 0) continue;            // skip hidden/blank
        JsonObject net = arr.createNestedObject();
        net["ssid"] = ssid;
        net["rssi"] = WiFi.RSSI(order[k]);
        if (measureJson(doc) > WIFI_JSON_MAX) { arr.remove(arr.size() - 1); break; }
    }
    g_cached_len = serializeJson(doc, g_cached_list, sizeof(g_cached_list));
    WiFi.scanDelete();
    Serial.printf("[ble] cached %u-byte network list\n", (unsigned)g_cached_len);
    return n;
}

// Push whatever is in g_cached_list to char 0x0004 (setValue persists it for
// reassembled reads; notify wakes the app). Never silent — worst case "[]".
static void publish_cached_list(void) {
    if (g_wifilist_char) {
        g_wifilist_char->setValue((uint8_t *)g_cached_list, g_cached_len);
        g_wifilist_char->notify();
    }
    Serial.printf("[ble] published cached list to 0x0004 (%u bytes)\n", (unsigned)g_cached_len);
    g_wifi_scan_published = true;
}

// ─── WiFi credentials handler ───────────────────────────────────────────────
// Attempt WiFi connection. Returns true if connected within timeout.
//
// IMPORTANT: We deliberately do NOT call lv_timer_handler() during the wait.
// The ESP32-S3 RGB panel framebuffer lives in PSRAM and is read continuously
// by the LCD DMA. WiFi.begin() / packet activity also hits PSRAM heavily for
// its own buffers. Triggering an LVGL frame redraw while WiFi is active
// causes visible PSRAM bandwidth contention -> horizontal jitter / glitching.
//
// So during this 5-15 second WiFi connect window, we let LVGL freeze on its
// last frame. BLE notifications still flow (they're handled on a separate
// FreeRTOS task, not the main loop). Touch input is ignored during the wait,
// which is fine — the user just submitted credentials and is waiting.
static bool wifi_try(const char *ssid, const char *pw, uint32_t timeout_ms) {
    if (!ssid || strlen(ssid) == 0) return false;
    Serial.printf("[ble] connecting to WiFi SSID='%s'\n", ssid);
    // g_busy is already true (set in onConnect). LVGL stays suspended.

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true);
    delay(100);
    WiFi.begin(ssid, pw);

    uint32_t start = millis();
    uint32_t last_log = start;
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
        delay(100);
        if (millis() - last_log > 1000) {
            Serial.print('.');
            last_log = millis();
        }
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[ble] WiFi connected, IP=%s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    Serial.printf("[ble] WiFi '%s' connect failed, status=%d\n", ssid, WiFi.status());
    return false;
}

bool pairing_ble_is_busy(void) {
    // Watchdog: g_busy should clear via onDisconnect or pairing_ble_stop. If
    // any code path ever leaves it stuck (forced BLE deinit, dropped callback,
    // etc.), self-heal after BUSY_MAX_MS so the UI can never be permanently
    // frozen. The legitimate busy window (WiFi connect) is ~10s; the whole
    // pairing session is well under 90s.
    if (g_busy && g_busy_since != 0 && (millis() - g_busy_since) > BUSY_MAX_MS) {
        Serial.println("[ble] g_busy watchdog tripped — force-clearing");
        g_busy = false;
    }
    return g_busy;
}

// ─── BLE callbacks ──────────────────────────────────────────────────────────
class PairCharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *chr) override {
        // ESP32 Arduino core 3.x returns Arduino String here (was std::string in 2.x)
        String raw = chr->getValue();
        Serial.printf("[ble] pair char write: %u bytes\n", (unsigned)raw.length());
        if (raw.length() == 0 || raw.length() > 480) {
            notify_status(LT_STATUS_ERR_JSON);
            return;
        }

        notify_status(LT_STATUS_VALIDATING);

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, raw.c_str());
        if (err) {
            Serial.printf("[ble] JSON parse failed: %s\n", err.c_str());
            notify_status(LT_STATUS_ERR_JSON);
            return;
        }

        const char *token = doc["token"] | (const char *)nullptr;
        const char *ssid  = doc["ssid"]  | (const char *)nullptr;
        const char *pw    = doc["pw"]    | "";
        const char *acct  = doc["acct"]  | (const char *)nullptr;
        const char *ssid2 = doc["ssid2"] | "";   // optional
        const char *pw2   = doc["pw2"]   | "";   // optional

        if (!token || !ssid || !acct) {
            Serial.println("[ble] payload missing required field (token/ssid/acct)");
            notify_status(LT_STATUS_ERR_JSON);
            return;
        }

        const char *expected = pairing_get_token_hex();
        if (strcmp(token, expected) != 0) {
            Serial.println("[ble] token mismatch");   // never log the expected token
            notify_status(LT_STATUS_ERR_TOKEN);
            return;
        }

        // Persist all credentials BEFORE WiFi attempt so they survive a crash
        if (!pairing_store_credentials(ssid, pw, ssid2, pw2, acct)) {
            notify_status(LT_STATUS_ERR_INTERNAL);
            return;
        }

        // Credentials received — STOP scanning from now on. scan_and_cache()
        // calls WiFi.disconnect(), so a scan triggered by a BLE reconnect
        // after this point would tear down the WiFi we're about to connect.
        g_creds_received = true;
        g_scan_requested = false;

        notify_status(LT_STATUS_WIFI_CONNECTING);

        // Try primary, then secondary if provided
        bool connected = wifi_try(ssid, pw, 20000);
        if (!connected && strlen(ssid2) > 0) {
            Serial.printf("[ble] primary failed, trying secondary SSID='%s'\n", ssid2);
            connected = wifi_try(ssid2, pw2, 20000);
        }

        if (!connected) {
            notify_status(LT_STATUS_ERR_WIFI);
            return;
        }

        // WiFi up. 0x03 means "WiFi connected" per the iOS contract. The actual
        // device-side "paired" state (screen advance) waits for Supabase to flip
        // onboarding_complete = true after the user finishes the survey.
        notify_status(LT_STATUS_PAIRED);

        // Start polling Supabase for onboarding_complete
        cloud_sync_begin();
    }
};

class WifiListCharCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *chr) override {
        Serial.println("[ble] wifi list char read");
        // The app reads this after connecting to populate its picker. If no
        // scan is running and we haven't published a result yet, kick a fresh
        // scan (covers the app's "Rescan" which re-reads this characteristic).
        // The current value is returned now; the fresh list is notify'd when
        // the scan completes.
        if (!g_wifi_scan_published && !g_creds_received) {
            request_wifi_scan();
        }
    }
};

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *srv) override {
        g_client_connected = true;
        // Suspend LVGL for the ENTIRE BLE session, not just WiFi connect.
        // Reason: the credential write, the BLE notify acks, the JSON parse,
        // and any incidental BLE traffic all spike PSRAM use. The RGB panel
        // DMA + LVGL frame redraws fight that for bandwidth → visible glitches.
        // Phone-side user is staring at their phone the whole time, so the
        // frozen device screen isn't seen.
        g_busy = true;
        g_busy_since = millis();
        Serial.println("[ble] client connected (LVGL suspended)");

        // The iOS app now DEPENDS on the BLE scan list (no manual SSID entry).
        // Request a fresh 2.4GHz scan on every connect so the picker is
        // populated, and a reconnect ('Rescan' in the app) re-scans for newly
        // powered-on routers. The scan runs in pairing_ble_loop (main loop),
        // result notify'd + stored to 0x0004 when done. Skip once credentials
        // are in — a scan from here would WiFi.disconnect() the live link.
        if (!g_creds_received) request_wifi_scan();
    }
    void onDisconnect(BLEServer *srv) override {
        g_client_connected = false;
        g_busy = false;
        Serial.println("[ble] client disconnected (LVGL resumed)");
        if (!pairing_is_complete()) {
            BLEDevice::startAdvertising();
        }
    }
};

// ─── Init / teardown ────────────────────────────────────────────────────────
void pairing_ble_begin(void) {
    if (pairing_is_complete()) {
        Serial.println("[ble] already paired, skipping BLE init");
        return;
    }

    // Boot-time WiFi scan BEFORE any BLE radio activity. This is the one window
    // with zero BLE coexistence interference, so the scan is reliable here. We
    // cache the result and serve it on connect; a live re-scan refreshes it but
    // falls back to this cache if coexistence zeroes the live scan.
    Serial.println("[ble] boot-time WiFi scan (pre-BLE, no coexistence)...");
    scan_and_cache();

    char ble_name[24];
    snprintf(ble_name, sizeof(ble_name), "LegacyTape-%s", pairing_get_device_id());

    BLEDevice::init(ble_name);
    BLEDevice::setMTU(247);

    g_server = BLEDevice::createServer();
    g_server->setCallbacks(new ServerCallbacks());

    BLEService *svc = g_server->createService(LT_BLE_SERVICE_UUID);

    g_pair_char = svc->createCharacteristic(
        LT_BLE_PAIR_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    g_pair_char->setCallbacks(new PairCharCallbacks());

    g_status_char = svc->createCharacteristic(
        LT_BLE_STATUS_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    g_status_char->addDescriptor(new BLE2902());
    uint8_t idle = LT_STATUS_IDLE;
    g_status_char->setValue(&idle, 1);

    g_wifilist_char = svc->createCharacteristic(
        LT_BLE_WIFILIST_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    g_wifilist_char->addDescriptor(new BLE2902());
    // Seed with the boot-time scan cache so the very first read returns real
    // networks even before the on-connect re-scan runs.
    g_wifilist_char->setValue((uint8_t *)g_cached_list, g_cached_len);
    g_wifilist_char->setCallbacks(new WifiListCharCallbacks());

    svc->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(LT_BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);
    adv->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.printf("[ble] advertising as '%s' with service %s\n", ble_name, LT_BLE_SERVICE_UUID);
}

void pairing_ble_stop(void) {
    Serial.println("[ble] stopping BLE stack");
    // Force-clear the LVGL-suspend flag BEFORE deinit. onDisconnect would
    // normally clear it, but BLEDevice::deinit(true) tears down the stack
    // without firing the disconnect callback, leaving g_busy stuck true
    // and LVGL frozen — every touch ignored — until next reboot.
    g_busy = false;
    g_client_connected = false;
    if (g_server) {
        BLEDevice::stopAdvertising();
        BLEDevice::deinit(true);
        g_server = nullptr;
        g_pair_char = nullptr;
        g_status_char = nullptr;
        g_wifilist_char = nullptr;
    }
}

// Called from main loop. Runs a SYNCHRONOUS WiFi scan when one was requested.
// Synchronous is far more reliable than async while BLE is connected — the
// async scan-done callback often never fires under radio coexistence, which
// left the characteristic empty. Blocking here (~2-4s) is safe: LVGL is
// already suspended during pairing (g_busy) and BLE runs on its own task, so
// blocking the main loop doesn't stall the BLE connection.
void pairing_ble_loop(void) {
    if (!g_server) return;
    if (g_creds_received) return;     // never scan after creds — it'd drop WiFi
    if (!g_scan_requested) return;
    g_scan_requested = false;

    // Serve the cached list immediately so the app's picker fills fast even if
    // the live re-scan below comes back empty (BLE-coexistence can zero it).
    publish_cached_list();

    // Attempt a fresh scan to pick up newly-powered routers. If it finds
    // networks it refreshes the cache; if coexistence zeroes it, the cache
    // (from the boot scan / a prior connect) is preserved. Re-publish either way.
    int n = scan_and_cache();
    if (n > 0) publish_cached_list();
}
