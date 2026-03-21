#pragma once

#include <stdint.h>
#include "lvgl.h"

// ---------------------------------------------------------------------------
// ST7789V3 driver for LVGL on RP2040
// Hardware SPI0, partial framebuffer (1/10 screen height strips)
// ---------------------------------------------------------------------------

// Initialise SPI, GPIO, reset and configure the panel.
// Must be called from Core 1 before lv_init().
void st7789_init(void);

// LVGL flush callback — registered via lv_disp_drv_t.flush_cb
void st7789_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);

// Backlight brightness: 0–100 (PWM on PIN_DISP_BLK)
void st7789_set_backlight(uint8_t pct);
