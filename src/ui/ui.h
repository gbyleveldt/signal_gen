#pragma once

#include "encoder/encoder.h"

// ---------------------------------------------------------------------------
// UI module — runs entirely on Core 1
//
// Responsibilities:
//   - LVGL tick and task handler
//   - ST7789 display init and flush
//   - Rotary encoder polling
//   - Calls audio_set_*() to update audio engine parameters
// ---------------------------------------------------------------------------

// Initialise display + LVGL + create screen widgets.
// Call once from Core 1 before ui_run().
void ui_init(encoder_t *enc_freq, encoder_t *enc_amp);

// Core 1 main loop — never returns.
void ui_run(void);
