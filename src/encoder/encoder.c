#include "encoder.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/time.h"

// ---------------------------------------------------------------------------
// Quadrature decode table (unchanged from original)
// ---------------------------------------------------------------------------
const int8_t encoder_qem_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

// ---------------------------------------------------------------------------
// Instance registry
// The RP2040 has a single GPIO IRQ callback entry point. We register one
// shared handler here and dispatch to whichever instance owns the GPIO.
// ---------------------------------------------------------------------------
static encoder_t *_instances[ENCODER_MAX_INSTANCES];
static int        _instance_count = 0;

// ---------------------------------------------------------------------------
// Internal: handle one encoder's IRQ
// ---------------------------------------------------------------------------
static void _handle_encoder(encoder_t *enc, uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == enc->pin_a || gpio == enc->pin_b) {
        uint8_t a = gpio_get(enc->pin_a) ? 1 : 0;
        uint8_t b = gpio_get(enc->pin_b) ? 1 : 0;
        uint8_t current_ab = (a << 1) | b;
        int8_t direction = encoder_qem_table[(enc->last_ab << 2) | current_ab];
        if (direction != 0)
            enc->delta += direction;
        enc->last_ab = current_ab;
    }

    if (gpio == enc->pin_pb) {
        if (events & GPIO_IRQ_EDGE_FALL) {
            // Debounce only the press edge
            if ((now - enc->last_pb_time) > ENCODER_DEBOUNCE_MS) {
                enc->last_pb_time  = now;
                enc->pb_state      = true;
                enc->pb_press_time = now;
                enc->pb_held       = false;
            }
        } else if (events & GPIO_IRQ_EDGE_RISE) {
            // Always process release if we have a valid press recorded
            if (enc->pb_state) {
                enc->pb_state = false;
                if (!enc->pb_held)
                    enc->pb_event = true;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Shared GPIO IRQ dispatcher
// ---------------------------------------------------------------------------
static void _gpio_irq_handler(uint gpio, uint32_t events) {
    for (int i = 0; i < _instance_count; i++) {
        encoder_t *enc = _instances[i];
        if (gpio == enc->pin_a ||
            gpio == enc->pin_b ||
            gpio == enc->pin_pb) {
            _handle_encoder(enc, gpio, events);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void encoder_driver_init(void) {
    _instance_count = 0;
    // The actual callback is registered on first encoder_init() call.
    // Subsequent encoders just enable their GPIO IRQ via gpio_set_irq_enabled().
}

void encoder_init(encoder_t *enc, uint pin_a, uint pin_b, uint pin_pb) {
    enc->pin_a        = pin_a;
    enc->pin_b        = pin_b;
    enc->pin_pb       = pin_pb;
    enc->delta        = 0;
    enc->pb_event     = false;
    enc->pb_state     = false;
    enc->pb_held      = false;
    enc->pb_press_time = 0;
    enc->last_pb_time  = 0;

    gpio_init(pin_a);  gpio_set_dir(pin_a,  GPIO_IN);
    gpio_init(pin_b);  gpio_set_dir(pin_b,  GPIO_IN);
    gpio_init(pin_pb); gpio_set_dir(pin_pb, GPIO_IN);

    // Capture initial AB state to avoid a spurious step on first edge
    uint8_t a = gpio_get(pin_a) ? 1 : 0;
    uint8_t b = gpio_get(pin_b) ? 1 : 0;
    enc->last_ab = (a << 1) | b;

    // Register instance before enabling IRQs
    _instances[_instance_count++] = enc;

    if (_instance_count == 1) {
        // First encoder: register the shared callback
        gpio_set_irq_enabled_with_callback(pin_a,
            GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &_gpio_irq_handler);
    } else {
        // Subsequent encoders: reuse the already-registered callback
        gpio_set_irq_enabled(pin_a,
            GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    }

    gpio_set_irq_enabled(pin_b,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(pin_pb,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
}

int8_t encoder_get_delta(encoder_t *enc) {
    uint32_t irq_status = save_and_disable_interrupts();
    int8_t steps = 0;

    // Report only complete detents (4 quadrature pulses each)
    if (enc->delta >= 4) {
        steps = enc->delta / 4;
        enc->delta -= steps * 4;
    } else if (enc->delta <= -4) {
        steps = enc->delta / 4;
        enc->delta -= steps * 4;
    }

    restore_interrupts(irq_status);
    return steps;
}

bool encoder_button_pressed(encoder_t *enc) {
    uint32_t irq_status = save_and_disable_interrupts();
    bool event = enc->pb_event;
    enc->pb_event = false;
    restore_interrupts(irq_status);
    return event;
}

bool encoder_button_held(encoder_t *enc) {
    if (enc->pb_state && !enc->pb_held) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - enc->pb_press_time) > ENCODER_HOLD_MS) {
            enc->pb_held = true;
            return true;
        }
    }
    return false;
}
