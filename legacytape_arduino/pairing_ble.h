// Legacy Tape — BLE pairing receiver.
//
// Advertises a BLE service that the iOS app discovers via the QR code's
// hardware_id. App writes a JSON credentials payload to the pair characteristic;
// device validates the token, stores SSID/PW/account in NVS, attempts WiFi
// connection, and notifies status bytes back to the app.
//
// See pairing.h for UUIDs + status byte constants.
#ifndef LEGACYTAPE_PAIRING_BLE_H
#define LEGACYTAPE_PAIRING_BLE_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize BLE stack + start advertising. Call once in setup() AFTER pairing_begin().
// No-op if pairing is already complete (device shouldn't re-advertise after pairing).
void pairing_ble_begin(void);

// Stop advertising and tear down the BLE stack. Frees ~30 KB RAM.
// Call after pairing completes (the iOS app has disconnected).
void pairing_ble_stop(void);

// Call from main loop — publishes WiFi scan results to the BLE characteristic
// once an async scan completes.
void pairing_ble_loop(void);

// True while the BLE module is mid-WiFi-connect. The main loop should skip
// lv_timer_handler() while this is true — RGB panel DMA + WiFi PSRAM use
// otherwise contend and cause visible screen glitching.
bool pairing_ble_is_busy(void);

#ifdef __cplusplus
}
#endif
#endif
