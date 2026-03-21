#include "st7789.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <string.h>

// ---------------------------------------------------------------------------
// ST7789 command codes
// ---------------------------------------------------------------------------
#define ST7789_NOP        0x00
#define ST7789_SWRESET    0x01
#define ST7789_SLPOUT     0x11
#define ST7789_NORON      0x13
#define ST7789_INVON      0x21   // inversion on  (most ST7789 panels need this)
#define ST7789_DISPON     0x29
#define ST7789_CASET      0x2A   // column address set
#define ST7789_RASET      0x2B   // row address set
#define ST7789_RAMWR      0x2C   // memory write
#define ST7789_MADCTL     0x36   // memory data access control
#define ST7789_COLMOD     0x3A   // interface pixel format

// MADCTL bits
#define MADCTL_MY         0x80
#define MADCTL_MX         0x40
#define MADCTL_MV         0x20
#define MADCTL_RGB        0x00

// COLMOD: 16-bit colour (RGB565)
#define COLMOD_16BIT      0x55

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static inline void _dc_cmd(void)  { gpio_put(PIN_DISP_DC, 0); }
static inline void _dc_data(void) { gpio_put(PIN_DISP_DC, 1); }
static inline void _cs_low(void)  { gpio_put(PIN_DISP_CS, 0); }
static inline void _cs_high(void) { gpio_put(PIN_DISP_CS, 1); }

static void _write_cmd(uint8_t cmd) {
    _dc_cmd();
    _cs_low();
    spi_write_blocking(spi1, &cmd, 1);
    _cs_high();
}

static void _write_data(const uint8_t *data, size_t len) {
    _dc_data();
    _cs_low();
    spi_write_blocking(spi1, data, len);
    _cs_high();
}

static void _write_data_byte(uint8_t b) {
    _write_data(&b, 1);
}

// ---------------------------------------------------------------------------
// Window (CASET + RASET + RAMWR)
// ---------------------------------------------------------------------------
static void _set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    x0 += DISP_X_OFFSET;
    x1 += DISP_X_OFFSET;
    y0 += DISP_Y_OFFSET;
    y1 += DISP_Y_OFFSET;

    uint8_t col[4] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    uint8_t row[4] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    _write_cmd(ST7789_CASET); _write_data(col, 4);
    _write_cmd(ST7789_RASET); _write_data(row, 4);
    _write_cmd(ST7789_RAMWR);
}

// ---------------------------------------------------------------------------
// st7789_init
// ---------------------------------------------------------------------------
void st7789_init(void) {
    // SPI
    spi_init(spi1, DISP_SPI_BAUDRATE);
    gpio_set_function(PIN_DISP_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_DISP_MOSI, GPIO_FUNC_SPI);

    // Control GPIOs
    gpio_init(PIN_DISP_CS);  gpio_set_dir(PIN_DISP_CS,  GPIO_OUT); _cs_high();
    gpio_init(PIN_DISP_DC);  gpio_set_dir(PIN_DISP_DC,  GPIO_OUT); _dc_data();
    gpio_init(PIN_DISP_RST); gpio_set_dir(PIN_DISP_RST, GPIO_OUT);

    // Hardware reset
    gpio_put(PIN_DISP_RST, 0); sleep_ms(10);
    gpio_put(PIN_DISP_RST, 1); sleep_ms(120);

    // Backlight PWM (start at 0 — fade in after init)
    gpio_set_function(PIN_DISP_BLK, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_DISP_BLK);
    pwm_set_wrap(slice, 100);
    pwm_set_gpio_level(PIN_DISP_BLK, 0);
    pwm_set_enabled(slice, true);

    // Initialisation sequence
    _write_cmd(ST7789_SWRESET); sleep_ms(150);
    _write_cmd(ST7789_SLPOUT);  sleep_ms(10);

    _write_cmd(ST7789_COLMOD);
    _write_data_byte(COLMOD_16BIT);

    // Orientation: landscape 320×172
    // Adjust MADCTL if panel appears mirrored/rotated
    _write_cmd(ST7789_MADCTL);
    _write_data_byte(MADCTL_MX | MADCTL_MV | MADCTL_RGB);  // 90° clockwise

    _write_cmd(ST7789_INVON);   // most 172×320 modules need inversion
    sleep_ms(10);
    _write_cmd(ST7789_NORON);
    sleep_ms(10);
    _write_cmd(ST7789_DISPON);
    sleep_ms(10);
}

// ---------------------------------------------------------------------------
// LVGL flush callback
// ---------------------------------------------------------------------------
void st7789_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint16_t x0 = (uint16_t)area->x1;
    uint16_t x1 = (uint16_t)area->x2;
    uint16_t y0 = (uint16_t)area->y1;
    uint16_t y1 = (uint16_t)area->y2;

    _set_window(x0, y0, x1, y1);

    uint32_t pixel_count = (x1 - x0 + 1) * (y1 - y0 + 1);

    // lv_color_t is RGB565 in 16-bit mode; the SPI just needs the raw bytes.
    // LVGL stores colours natively so byte order matches the display.
    _dc_data();
    _cs_low();
    spi_write_blocking(spi1, (uint8_t *)color_p, pixel_count * sizeof(lv_color_t));
    _cs_high();

    lv_disp_flush_ready(drv);
}

// ---------------------------------------------------------------------------
// Backlight
// ---------------------------------------------------------------------------
void st7789_set_backlight(uint8_t pct) {
    if (pct > 100) pct = 100;
    uint slice = pwm_gpio_to_slice_num(PIN_DISP_BLK);
    pwm_set_gpio_level(PIN_DISP_BLK, pct);
}
