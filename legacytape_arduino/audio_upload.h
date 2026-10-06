// Continuous chunk uploader — pulls chunks from audio_record and POSTs them
// to a Supabase Edge Function. After all chunks are uploaded, sends a
// finalize call so the server can assemble the WAV + start transcription.
#ifndef LEGACYTAPE_AUDIO_UPLOAD_H
#define LEGACYTAPE_AUDIO_UPLOAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start the background upload task. Call once at boot. Idempotent.
void audio_upload_begin(void);

// Stop the background task (e.g. for shutdown). Safe to omit; task exits on idle.
void audio_upload_stop(void);

// Stats for the UI / debugging
uint32_t    audio_upload_chunks_uploaded(void);   // count of successful chunk POSTs
const char *audio_upload_last_error(void);

// Reset per-recording upload counters + error. Call when a new recording starts.
void        audio_upload_reset(void);

// Called by audio_record when it stops capturing so finalize can run
void audio_upload_request_finalize(uint32_t duration_sec, int chapter_idx);

// Internal: called by audio_record once the firmware sees server-side complete
void audio_record_mark_complete(void);   // implemented in audio_record.cpp

#ifdef __cplusplus
}
#endif
#endif
