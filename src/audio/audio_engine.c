#include "audio_engine.h"
#include "i2s/i2s.h"
#include "config.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/sync.h"
#include <math.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Sine LUT
// 1024 entries, 16-bit signed.
// Full scale: ±32767
// ---------------------------------------------------------------------------
static int16_t _sine_lut[AUDIO_SINE_LUT_SIZE];

static void _build_lut(void) {
    for (int i = 0; i < AUDIO_SINE_LUT_SIZE; i++) {
        _sine_lut[i] = (int16_t)(32767.0f *
            sinf(2.0f * (float)M_PI * i / AUDIO_SINE_LUT_SIZE));
    }
}

// ---------------------------------------------------------------------------
// Phase accumulator (Q32 fixed point)
// ---------------------------------------------------------------------------
static uint32_t _phase     = 0;
static uint32_t _phase_inc = 0;

static inline uint32_t _calc_phase_inc(float hz) {
    return (uint32_t)((hz / (float)AUDIO_SAMPLE_RATE) * 4294967296.0f);
}

// ---------------------------------------------------------------------------
// Shared parameters + spin lock
// ---------------------------------------------------------------------------
static spin_lock_t   *_lock   = NULL;
static audio_params_t _params = {
    .freq_hz   = AUDIO_FREQ_DEFAULT_HZ,
    .amplitude = AUDIO_AMP_DEFAULT_PCT,
    .channel   = AUDIO_CH_ALL
};
static audio_params_t _local = {
    .freq_hz   = AUDIO_FREQ_DEFAULT_HZ,
    .amplitude = AUDIO_AMP_DEFAULT_PCT,
    .channel   = AUDIO_CH_ALL
};

// ---------------------------------------------------------------------------
// DMA fill callback
// ---------------------------------------------------------------------------
static void _fill_buffer(uint32_t *buf, uint32_t word_count) {
    uint32_t save = spin_lock_blocking(_lock);
    audio_params_t p = _params;
    spin_unlock(_lock, save);

    if (p.freq_hz != _local.freq_hz) {
        _phase_inc     = _calc_phase_inc(p.freq_hz);
        _local.freq_hz = p.freq_hz;
    }
    _local.amplitude = p.amplitude;
    _local.channel   = p.channel;

    // Amplitude scale Q15: 0–100% maps to 0–32767
    int32_t scale = (int32_t)(_local.amplitude * 327);

    // Per-channel silence flags
    bool left_silent  = (_local.channel == AUDIO_CH_MUTE ||
                         _local.channel == AUDIO_CH_RIGHT);
    bool right_silent = (_local.channel == AUDIO_CH_MUTE ||
                         _local.channel == AUDIO_CH_LEFT);

    uint32_t stereo_pairs = word_count / 2;

    for (uint32_t i = 0; i < stereo_pairs; i++) {
        _phase += _phase_inc;

        int16_t raw = _sine_lut[_phase >> AUDIO_SINE_LUT_SHIFT];
        int16_t s   = (int16_t)((raw * scale) >> 15);

        uint32_t word = (uint32_t)(uint16_t)s << 16;

        buf[i * 2 + 0] = left_silent  ? 0 : word;   // left
        buf[i * 2 + 1] = right_silent ? 0 : word;   // right
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void audio_engine_init(void) {
    _build_lut();

    int lock_num = spin_lock_claim_unused(true);
    _lock = spin_lock_instance(lock_num);

    _phase_inc = _calc_phase_inc(_params.freq_hz);

    i2s_init();
    i2s_set_fill_callback(_fill_buffer);
}

void audio_engine_run(void) {
    i2s_start();
    while (true)
        tight_loop_contents();
}

// --- Thread-safe setters ----------------------------------------------------

void audio_set_freq(float hz) {
    if (hz < AUDIO_FREQ_MIN_HZ) hz = AUDIO_FREQ_MIN_HZ;
    if (hz > AUDIO_FREQ_MAX_HZ) hz = AUDIO_FREQ_MAX_HZ;
    uint32_t save = spin_lock_blocking(_lock);
    _params.freq_hz = hz;
    spin_unlock(_lock, save);
}

void audio_set_amplitude(uint8_t pct) {
    if (pct > 100) pct = 100;
    uint32_t save = spin_lock_blocking(_lock);
    _params.amplitude = pct;
    spin_unlock(_lock, save);
}

void audio_set_channel(audio_channel_t ch) {
    uint32_t save = spin_lock_blocking(_lock);
    _params.channel = ch;
    spin_unlock(_lock, save);
}

// --- Thread-safe getters ----------------------------------------------------

float audio_get_freq(void) {
    uint32_t save = spin_lock_blocking(_lock);
    float v = _params.freq_hz;
    spin_unlock(_lock, save);
    return v;
}

uint8_t audio_get_amplitude(void) {
    uint32_t save = spin_lock_blocking(_lock);
    uint8_t v = _params.amplitude;
    spin_unlock(_lock, save);
    return v;
}

audio_channel_t audio_get_channel(void) {
    uint32_t save = spin_lock_blocking(_lock);
    audio_channel_t v = _params.channel;
    spin_unlock(_lock, save);
    return v;
}
