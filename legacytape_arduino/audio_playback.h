// Legacy Tape — stream a chapter's recordings from Supabase Storage to the
// I2S speaker.
//
// Flow:
//   1. POST /functions/v1/get_recording with hw_id + token + chapter idx
//      → server returns the chapter timeline: { found, total_duration_sec,
//        recordings: [ { url (signed Storage URL), duration_sec }, ... ] }
//   2. Producer task (core 0): HTTP GET each signed URL in order, skip the
//      44-byte WAV header, push the PCM into a PSRAM ring buffer.
//   3. Consumer task (core 1): drain the ring into the I2S output (speaker
//      amp on the SPK port: BCLK=5, WS/LRC=6, DOUT=4). i2s.write() blocks on
//      the DMA buffer, which paces playback in real time.
//
// 16 kHz mono 16-bit — matches what the device recorded.
//
// v1 is sequential play/stop only. RWD/FF (HTTP Range seeks) come in v2.
#ifndef LEGACYTAPE_AUDIO_PLAYBACK_H
#define LEGACYTAPE_AUDIO_PLAYBACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLAYBACK_IDLE,
    PLAYBACK_FETCHING,     // requesting URL / connecting
    PLAYBACK_PLAYING,
    PLAYBACK_DONE,         // reached end of file
    PLAYBACK_NONE,         // no recording exists for this chapter
    PLAYBACK_ERROR
} playback_state_t;

// Start streaming the recording for the given chapter index. Non-blocking.
void audio_playback_start(int chapter_idx);

// Stop playback + free the I2S output. Safe to call any time.
void audio_playback_stop(void);

playback_state_t audio_playback_state(void);
uint32_t         audio_playback_position_sec(void);   // seconds played so far
uint32_t         audio_playback_duration_sec(void);   // total, from server
const char      *audio_playback_last_error(void);

// Volume 0..100 (%). Applied as a software scale to samples before I2S, so it
// works on top of the MAX98357's fixed hardware gain. Persisted in NVS.
// Default is a conservative level that won't clip the amp.
int  audio_playback_get_volume(void);
void audio_playback_set_volume(int vol);              // clamped 0..100, saved
void audio_playback_volume_step(int delta);           // +/- by delta, clamped, saved

#ifdef __cplusplus
}
#endif
#endif
