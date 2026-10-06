// Legacy Tape — continuous-streaming audio recording.
//
// Captures from the built-in PDM mic (GPIO 19 CLK + GPIO 20 DATA) and
// chunks the raw PCM into 10-second segments. Each chunk is handed off
// to the upload module which POSTs it to a Supabase Edge Function.
// The server reassembles the chunks into one WAV file per session.
//
// Two PSRAM chunk buffers (320 KB each = 10 s @ 16 kHz mono 16-bit), used
// as a double buffer: while one fills with mic data, the other is being
// uploaded. This gives us ~10 seconds of WiFi-drop tolerance before audio
// loss begins.
//
// Recording is "unlimited" length: as long as WiFi keeps up with the
// 32 KB/s capture rate, the user can record for hours.
#ifndef LEGACYTAPE_AUDIO_RECORD_H
#define LEGACYTAPE_AUDIO_RECORD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_CHUNK_SECONDS 10
#define AUDIO_CHUNK_BYTES   (AUDIO_SAMPLE_RATE * 2 * AUDIO_CHUNK_SECONDS)  // 320 KB

typedef enum {
    AUDIO_STATE_IDLE,
    AUDIO_STATE_RECORDING,
    AUDIO_STATE_FINALIZING,    // STOP pressed, last chunk + finalize call in flight
    AUDIO_STATE_COMPLETE,      // recording fully uploaded + finalized server-side
    AUDIO_STATE_ERROR
} audio_state_t;

// Boot-time init: allocate PSRAM buffers, unmute mic via I2C 0x30. Idempotent.
bool audio_record_begin(void);

// Start a fresh recording session. Generates a new session_id (32-char hex).
// Returns false if already recording or if start fails.
bool audio_record_start(void);

// Stop capturing. Submits any partial chunk, then triggers finalize on the
// upload side. State goes RECORDING -> FINALIZING -> COMPLETE.
void audio_record_stop(void);

audio_state_t  audio_record_state(void);
uint32_t       audio_record_seconds(void);            // total captured time
uint8_t        audio_record_level_percent(void);      // 0..100 for VU meter
const char    *audio_record_session_id(void);         // current/last session UUID
uint32_t       audio_record_chunks_captured(void);    // sequential ID of next chunk
const char    *audio_record_last_error(void);         // human-readable, "" if none

// True while the capture task is still running (still flushing chunks). The
// uploader must wait for this to go false before judging a session as
// "nothing uploaded" — otherwise it races the final partial-chunk flush.
bool           audio_record_capture_active(void);

// Network-failure abort path. Called by audio_upload after N consecutive upload
// failures (i.e. WiFi is gone and isn't coming back). Sets state to ERROR,
// records the reason, and tells capture_task to exit immediately without
// queuing the partial buffer. The reason is available from
// audio_record_last_error(); the UI does not display it yet.
void audio_record_force_stop_for_network(const char *reason);

// Called by audio_upload once the server-side finalize POST has succeeded
// (or is skipped because no chunks were uploaded). Advances state from
// FINALIZING to COMPLETE so the UI ticks past "Uploading…".
// Declared here so the .cpp implementation inherits C linkage and the
// audio_upload.cpp call site (which sees the same extern "C") resolves.
void audio_record_mark_complete(void);

// ─── Internal API used by audio_upload only ───
// Get the next ready-to-upload chunk. Caller must release it. Returns false
// if no chunk is ready. Non-blocking.
typedef struct {
    uint8_t *data;
    size_t   size;
    uint32_t idx;
    void    *_internal;   // opaque handle so release knows which slot to free
} audio_chunk_t;
bool audio_record_take_chunk(audio_chunk_t *out);
void audio_record_release_chunk(const audio_chunk_t *chunk);

#ifdef __cplusplus
}
#endif
#endif
