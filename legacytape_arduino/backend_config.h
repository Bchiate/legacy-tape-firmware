// Legacy Tape — single include point for backend settings.
//
// Deployment-specific values (project URL, publishable key) come from
// config.h, which is gitignored. Copy config.example.h to config.h first.
#pragma once

// Quoted includes search this file's directory first, so this picks up
// legacytape_arduino/config.h whenever it exists.
#if __has_include("config.h")
#include "config.h"
#endif

// Without the project's config.h the include above can resolve to an unrelated
// config.h on the toolchain's include path, so check for the settings
// themselves rather than for the file.
#if !defined(LT_SUPABASE_URL) || !defined(LT_SUPABASE_PUBLISHABLE_KEY)
#error "Backend settings missing: copy legacytape_arduino/config.example.h to legacytape_arduino/config.h and set LT_SUPABASE_URL and LT_SUPABASE_PUBLISHABLE_KEY (see README, Configuration)."
#endif

// Backend API used by the device, relative to LT_SUPABASE_URL.
#ifndef LT_EP_UPLOAD_CHUNK
#define LT_EP_UPLOAD_CHUNK       "/functions/v1/upload_chunk"        // POST raw PCM chunk
#endif
#ifndef LT_EP_FINALIZE_RECORDING
#define LT_EP_FINALIZE_RECORDING "/functions/v1/finalize_recording"  // POST close a session
#endif
#ifndef LT_EP_GET_RECORDING
#define LT_EP_GET_RECORDING      "/functions/v1/get_recording"       // POST chapter clip list
#endif
#ifndef LT_EP_DEVICE_STATUS
#define LT_EP_DEVICE_STATUS      "/rest/v1/rpc/device_status"        // POST onboarding poll
#endif
