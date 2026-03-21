#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Channel mode
// Cycled via short press on the att encoder: ALL → LEFT → RIGHT → MUTE → ALL
// ---------------------------------------------------------------------------
typedef enum {
    AUDIO_CH_ALL = 0,   // stereo — signal on both channels (default)
    AUDIO_CH_LEFT,      // signal on left channel only,  right = silence
    AUDIO_CH_RIGHT,     // signal on right channel only, left  = silence
    AUDIO_CH_MUTE       // both channels silent
} audio_channel_t;

// ---------------------------------------------------------------------------
// Shared audio state  (written by Core 1 UI, read by Core 0 audio engine)
// Protected by a spin lock — use audio_set_*() / audio_get_*() always.
// ---------------------------------------------------------------------------
typedef struct {
    float           freq_hz;    // current target frequency
    uint8_t         amplitude;  // 0–100 %
    audio_channel_t channel;    // output channel routing
} audio_params_t;

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Call once from Core 0 before launching Core 1.
void audio_engine_init(void);

// Core 0 main loop — blocks forever running the audio engine.
void audio_engine_run(void);

// Thread-safe setters (called from Core 1 / UI task)
void audio_set_freq(float hz);
void audio_set_amplitude(uint8_t pct);
void audio_set_channel(audio_channel_t ch);

// Thread-safe getters (for display refresh)
float           audio_get_freq(void);
uint8_t         audio_get_amplitude(void);
audio_channel_t audio_get_channel(void);
