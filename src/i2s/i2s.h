#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// ---------------------------------------------------------------------------
// DMA ping-pong buffer layout
//
// Two buffers of AUDIO_DMA_BUF_STEREO stereo pairs each.
// Each stereo pair = 2 x uint32_t words: [left_word, right_word]
// Word format: bits[31:16] = 16-bit signed sample, bits[15:0] = 0
//
// Core 0 audio engine fills the "back" buffer via i2s_get_fill_buffer()
// while DMA plays the "front" buffer into the PIO TX FIFO.
// When DMA completes a buffer the IRQ swaps front/back and signals Core 0.
// ---------------------------------------------------------------------------
#define I2S_BUF_WORDS   (AUDIO_DMA_BUF_STEREO * 2)   // uint32_t words per buffer

typedef struct {
    uint32_t buf[2][I2S_BUF_WORDS];   // ping-pong buffers
} i2s_buffers_t;

// Callback invoked from DMA IRQ on Core 0 when the next buffer needs filling.
// The audio engine assigns this via i2s_set_fill_callback().
typedef void (*i2s_fill_callback_t)(uint32_t *buf, uint32_t word_count);

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Initialise PIO + DMA, start streaming silence.
// Call after set_sys_clock_khz() so the PIO divider is calculated correctly.
void i2s_init(void);

// Register the audio engine's buffer-fill callback.
void i2s_set_fill_callback(i2s_fill_callback_t cb);

// Start/stop the I2S stream (DMA + PIO).
void i2s_start(void);
void i2s_stop(void);
