/**
 * lv_conf.h  –  LVGL v8 configuration for signal_gen
 * RP2040 + ST7789V3 172×320 (landscape: 320×172), 16-bit colour
 */

#if 1  /* Set to 1 to enable, 0 to disable */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOUR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   1   /* Required for ST7789 over SPI */

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (16 * 1024U)   /* 16 KB */

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD    30
#define LV_INDEV_DEF_READ_PERIOD   30

/*====================
   FEATURE CONFIG
 *====================*/
#define LV_USE_PERF_MONITOR    0
#define LV_USE_MEM_MONITOR     0
#define LV_USE_REFR_DEBUG      0

/*====================
   FONT USAGE
 *====================*/
#define LV_FONT_MONTSERRAT_14  1   /* titles, units, step label, Vrms  */
#define LV_FONT_MONTSERRAT_16  1   /* MUTE label                       */
#define LV_FONT_MONTSERRAT_22  1   /* arc main values (freq, dB)       */

/* All unused sizes disabled to save flash */
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  0
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_24  0
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0
#define LV_FONT_MONTSERRAT_48  0

#define LV_FONT_DEFAULT        &lv_font_montserrat_14

/*====================
   WIDGET USAGE
 *====================*/
#define LV_USE_ARC         1
#define LV_USE_BAR         0   /* not used in arc-based UI */
#define LV_USE_LABEL       1
#define LV_USE_LINE        0
#define LV_USE_IMG         0
#define LV_USE_TABLE       0
#define LV_USE_BTN         0
#define LV_USE_BTNMATRIX   0
#define LV_USE_CANVAS      0
#define LV_USE_CHECKBOX    0
#define LV_USE_DROPDOWN    0
#define LV_USE_IMGBTN      0
#define LV_USE_KEYBOARD    0
#define LV_USE_LED         0
#define LV_USE_LIST        0
#define LV_USE_MENU        0
#define LV_USE_METER       0
#define LV_USE_MSGBOX      0
#define LV_USE_ROLLER      0
#define LV_USE_SLIDER      0
#define LV_USE_SPAN        0
#define LV_USE_SPINBOX     0
#define LV_USE_SPINNER     0
#define LV_USE_SWITCH      0
#define LV_USE_TABVIEW     0
#define LV_USE_TEXTAREA    0
#define LV_USE_TILEVIEW    0
#define LV_USE_WIN         0

/* Extra widgets — all disabled */
#define LV_USE_ANIMIMG     0
#define LV_USE_CALENDAR    0
#define LV_USE_CHART       0
#define LV_USE_COLORWHEEL  0

/*====================
   MISC
 *====================*/
#define LV_USE_ANIMATION   1
#define LV_ANIM_RESOLUTION 1024
#define LV_USE_LOG         0

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#endif /* LV_CONF_H */
#endif /* End of "Content enable" */
