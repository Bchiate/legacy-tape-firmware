// Legacy Tape — backend configuration template.
//
// Copy this file to config.h (same folder) and fill in your own values.
// config.h is gitignored, so deployment settings never land in the repo;
// the build stops with an #error if it is missing (see backend_config.h).
//
// The firmware talks to a Supabase project. Endpoint paths are defined in
// backend_config.h and can be overridden here if your deployment differs.
#pragma once

// Project URL, without a trailing slash.
#define LT_SUPABASE_URL "https://YOUR-PROJECT-REF.supabase.co"

// Publishable (client-side) API key, sent as the `apikey` header and as the
// Bearer token. Publishable keys are meant to ship inside client apps; device
// requests also carry the device ID and pairing token (x-hardware-id /
// x-pair-token headers, or the RPC's hw_id / tok) for the backend to check.
#define LT_SUPABASE_PUBLISHABLE_KEY "YOUR-PUBLISHABLE-KEY"
