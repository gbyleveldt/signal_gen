#pragma once
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdbool.h>

// Maximum number of encoder instances.
// One shared GPIO IRQ dispatcher routes events to all of them.
#define ENCODER_MAX_INSTANCES   4

#define ENCODER_DEBOUNCE_MS     150
#define ENCODER_HOLD_MS         1000

typedef struct {
    uint pin_a;
    uint pin_b;
    uint pin_pb;

    volatile int8_t   delta;
    volatile uint8_t  last_ab;

    volatile bool     pb_event;       // short press (set on release)
    volatile bool     pb_state;       // true while button held
    volatile bool     pb_held;        // true once long-press threshold crossed
    volatile uint32_t pb_press_time;
    volatile uint32_t last_pb_time;
} encoder_t;

// Quadrature decode table (same as original single-instance implementation)
// Index = (prev_ab << 2) | curr_ab
// Value: +1 CW, -1 CCW, 0 invalid/bounce
extern const int8_t encoder_qem_table[16];

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Call once before any encoder_init(). Sets up the shared IRQ dispatcher.
void encoder_driver_init(void);

// Initialise one encoder instance. Call encoder_driver_init() first.
void encoder_init(encoder_t *enc, uint pin_a, uint pin_b, uint pin_pb);

// Returns signed step count since last call (accounts for 4 pulses/detent).
// Thread-safe: disables IRQs around the read-modify of delta.
int8_t encoder_get_delta(encoder_t *enc);

// Returns true (once) on short press. Clears the event flag.
bool encoder_button_pressed(encoder_t *enc);

// Returns true (once) when button has been held past ENCODER_HOLD_MS.
// Clears the held flag so it won't fire again until next press.
bool encoder_button_held(encoder_t *enc);
