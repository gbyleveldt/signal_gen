#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"

#include "config.h"
#include "encoder/encoder.h"
#include "audio/audio_engine.h"
#include "ui/ui.h"

// ---------------------------------------------------------------------------
// Encoder instances (global so both cores can reference them if needed)
// ---------------------------------------------------------------------------
static encoder_t enc_freq;
static encoder_t enc_amp;

// ---------------------------------------------------------------------------
// Core 1 entry point  — UI + display + encoder polling
// ---------------------------------------------------------------------------
static void core1_main(void) {
    ui_init(&enc_freq, &enc_amp);
    ui_run();   // never returns
}

// ---------------------------------------------------------------------------
// Core 0 entry point
// ---------------------------------------------------------------------------
int main(void) {
    // Set system clock to 122.88 MHz for exact I2S divisor
    // If this fails (PLL can't achieve it) it falls back to 125 MHz.
    // A small frequency error (~0.2%) is acceptable for most audio work.
    set_sys_clock_khz(SYS_CLOCK_KHZ, false);

    stdio_init_all();   // USB serial for debug output

    // Encoders must be initialised before Core 1 starts so that the GPIO
    // IRQ callback is registered before any encoder events can fire.
    encoder_driver_init();
    encoder_init(&enc_freq, PIN_ENC_FREQ_A, PIN_ENC_FREQ_B, PIN_ENC_FREQ_SW);
    encoder_init(&enc_amp,  PIN_ENC_AMP_A,  PIN_ENC_AMP_B,  PIN_ENC_AMP_SW);

    // Initialise audio engine on Core 0 (sets up PIO + DMA)
    audio_engine_init();

    // Launch Core 1 (UI)
    multicore_launch_core1(core1_main);

    // Core 0: run audio engine forever (driven by DMA IRQs)
    audio_engine_run();

    // Never reached
    return 0;
}
