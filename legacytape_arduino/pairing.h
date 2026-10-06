// Legacy Tape — pairing token + QR URL generator.
//
// Device-side state for the app-driven pairing flow. Generates a stable
// per-device random token on first boot, persists it in NVS (Preferences),
// and exposes the deep-link URL the QR code encodes. Also stores the WiFi
// credentials + account ID the app delivers and the "paired" flag.
//
// The BLE transport that receives the credentials lives in pairing_ble.cpp;
// cloud_sync.cpp flips "paired" once the backend reports onboarding complete.
#ifndef LEGACYTAPE_PAIRING_H
#define LEGACYTAPE_PAIRING_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Call once in setup() AFTER ui_init() and BEFORE the first display refresh.
// Initializes NVS, loads-or-generates the device token + device ID.
void pairing_begin(void);

// Returns true if pairing has been completed previously (NVS flag set by
// pairing_mark_complete()).
bool pairing_is_complete(void);

// Mark the device as paired. Called by cloud_sync once the backend reports
// onboarding_complete for this device. Persisted in NVS, survives reboot.
void pairing_mark_complete(void);

// Wipe stored token + paired flag. Forces a fresh pairing on next boot.
// Useful for a Settings → Reset Pairing button.
void pairing_factory_reset(void);

// Store credentials received over BLE. Returns true on success (NVS write OK).
// Called by the BLE module after validating the payload. ssid2/pw2 may be NULL
// for the single-network case.
bool pairing_store_credentials(const char *ssid,  const char *pw,
                               const char *ssid2, const char *pw2,
                               const char *acct);

// Retrieve stored credentials (only valid once pairing_is_complete()).
// Returned pointers are valid until pairing_get_*() is called again.
const char *pairing_get_wifi_ssid(void);
const char *pairing_get_wifi_pw(void);
const char *pairing_get_wifi_ssid2(void);   // secondary network ("" if not provided)
const char *pairing_get_wifi_pw2(void);
const char *pairing_get_account(void);

// Set when pairing_mark_complete() is called; consumed by the main loop
// to trigger Screen1 -> Screen2 transition. Cleared after one read.
bool pairing_consume_complete_event(void);

// Pairing URL the QR code encodes, e.g.:
//   legacytape://pair?d=LT-A1B2C3&t=8c4a7b9e6f1d2a3b5c4d6e7f8a9b0c1d
// Pointer stays valid for the lifetime of the program.
const char *pairing_get_qr_url(void);

// BLE service + characteristic UUIDs (locked with iOS app)
#define LT_BLE_SERVICE_UUID    "1ec0de7a-7e2d-4f4f-9c1d-1ec0de7a0001"
#define LT_BLE_PAIR_CHAR_UUID  "1ec0de7a-7e2d-4f4f-9c1d-1ec0de7a0002"  // write
#define LT_BLE_STATUS_UUID     "1ec0de7a-7e2d-4f4f-9c1d-1ec0de7a0003"  // notify
#define LT_BLE_WIFILIST_UUID   "1ec0de7a-7e2d-4f4f-9c1d-1ec0de7a0004"  // read/notify

// Status bytes the device notifies on the status characteristic
#define LT_STATUS_IDLE           0x00
#define LT_STATUS_VALIDATING     0x01
#define LT_STATUS_WIFI_CONNECTING 0x02
#define LT_STATUS_PAIRED         0x03
#define LT_STATUS_ERR_TOKEN      0xE1
#define LT_STATUS_ERR_JSON       0xE2
#define LT_STATUS_ERR_WIFI       0xE3
#define LT_STATUS_ERR_INTERNAL   0xE4

// Human-readable device ID derived from the ESP32 MAC.
// Format: "LT-XXXXXX" (last 3 bytes of MAC as hex).
const char *pairing_get_device_id(void);

// Raw token (32 lowercase hex chars, null-terminated).
const char *pairing_get_token_hex(void);

#ifdef __cplusplus
}
#endif
#endif
