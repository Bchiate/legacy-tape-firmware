// Legacy Tape — Supabase polling for onboarding_complete signal.
//
// After BLE pairing succeeds and WiFi connects, the iOS app walks the user
// through the onboarding survey (setupFor → personInfo → 10 questions) and
// finally writes UPDATE devices SET onboarding_complete=true on Supabase.
//
// The device polls Supabase via an RPC (or REST) every 5 seconds. When it
// sees onboarding_complete=true on its own device row, it calls
// pairing_mark_complete() which triggers Screen1 -> Screen2 transition in
// the main loop.
//
// The Supabase URL + publishable key come from config.h at compile time
// (see backend_config.h). RLS or a public RPC must allow the device to check
// its own row by hardware_id + token. See cloud_sync.cpp for the endpoint.
#ifndef LEGACYTAPE_CLOUD_SYNC_H
#define LEGACYTAPE_CLOUD_SYNC_H

#ifdef __cplusplus
extern "C" {
#endif

// Start polling. Call once after BLE reports WiFi connected.
void cloud_sync_begin(void);

// Stop polling. Call after pairing_mark_complete() fires.
void cloud_sync_stop(void);

// Tick the poller. Call from the main loop; cheap when not active or when
// the poll interval hasn't elapsed.
void cloud_sync_loop(void);

#ifdef __cplusplus
}
#endif
#endif
