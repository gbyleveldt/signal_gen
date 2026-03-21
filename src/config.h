#pragma once

// =============================================================================
// signal_gen - Hardware Configuration
// Board: Waveshare RP2040-Zero
// =============================================================================

// -----------------------------------------------------------------------------
// System clock
// 122.88 MHz gives an exact integer PIO divider for 48 kHz I2S:
//   BCK  = 3.072 MHz = MCLK / 4    (32-bit frames, stereo)
//   LRCK = 48 kHz    = BCK  / 64
//   PIO runs at BCK * 2 = 6.144 MHz
//   Divider = 122,880,000 / 6,144,000 = 20  (integer, no jitter)
// -----------------------------------------------------------------------------
#define SYS_CLOCK_KHZ       122880

// -----------------------------------------------------------------------------
// I2S  (PIO0, state machine 0)
// Pins must be consecutive: DATA, BCK, LRCK in that order for PIO side-set
// -----------------------------------------------------------------------------
#define PIN_I2S_DATA        10
#define PIN_I2S_BCK         11
#define PIN_I2S_LRCK        12

// -----------------------------------------------------------------------------
// SPI1 - ST7789V3 display
// Note: GP17-GP25 are bottom pads only on RP2040-Zero, not suitable for
// breakout pins. Display moved to SPI1 on GP26/GP27.
// -----------------------------------------------------------------------------
#define PIN_DISP_SCK        26   // SPI1 SCK  (labelled SCL on breakout)
#define PIN_DISP_MOSI       27   // SPI1 MOSI (labelled SDA on breakout)
#define PIN_DISP_CS         9    // Chip select
#define PIN_DISP_DC         13   // Data/Command
#define PIN_DISP_RST        0    // Reset
#define PIN_DISP_BLK        1    // Backlight (PWM-capable)

#define DISP_WIDTH          320
#define DISP_HEIGHT         172
#define DISP_SPI_BAUDRATE   (40 * 1000 * 1000)   // 40 MHz

// ST7789 internal framebuffer is 240×320; visible panel is 172×320.
// A 34px Y offset is required to address the correct region.
#define DISP_X_OFFSET       0
#define DISP_Y_OFFSET       34

// -----------------------------------------------------------------------------
// I2C1 - Si5351A clock generator (RESERVED - unpopulated for now)
// When populated: generates 12.288 MHz MCLK for future DSP slave project
// -----------------------------------------------------------------------------
#define PIN_SI5351_SDA      14   // I2C1 SDA
#define PIN_SI5351_SCL      15   // I2C1 SCL
#define SI5351_I2C_ADDR     0x60

// -----------------------------------------------------------------------------
// Rotary encoder 1 - Frequency
// -----------------------------------------------------------------------------
#define PIN_ENC_FREQ_A      3
#define PIN_ENC_FREQ_B      4
#define PIN_ENC_FREQ_SW     5

// -----------------------------------------------------------------------------
// Rotary encoder 2 - Amplitude
// -----------------------------------------------------------------------------
#define PIN_ENC_AMP_A       6
#define PIN_ENC_AMP_B       7
#define PIN_ENC_AMP_SW      8

// -----------------------------------------------------------------------------
// WS2812 RGB LED (on-board, GP16 reserved by Waveshare)
// -----------------------------------------------------------------------------
#define PIN_WS2812          16

// -----------------------------------------------------------------------------
// Audio engine
// -----------------------------------------------------------------------------
#define AUDIO_SAMPLE_RATE       48000
#define AUDIO_SINE_LUT_SIZE     1024        // must be power of 2
#define AUDIO_SINE_LUT_SHIFT    22          // 32 - log2(LUT_SIZE) for Q32 phase
#define AUDIO_DMA_BUF_STEREO    256         // stereo sample pairs per DMA buffer
#define AUDIO_FREQ_MIN_HZ       20.0f
#define AUDIO_FREQ_MAX_HZ       20000.0f
#define AUDIO_FREQ_DEFAULT_HZ   1000.0f
#define AUDIO_AMP_DEFAULT_PCT   50          // 0-100 (≈ -6dB, ~1.1Vrms at startup)

// Encoder step sizes (changed via short-press on freq encoder)
#define FREQ_STEP_FINE          1.0f        // Hz
#define FREQ_STEP_MEDIUM        10.0f       // Hz
#define FREQ_STEP_COARSE        100.0f      // Hz
#define AMP_STEP_PCT            1           // % per click
