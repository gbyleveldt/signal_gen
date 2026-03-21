#include "ui.h"
#include "config.h"
#include "display/st7789.h"
#include "audio/audio_engine.h"
#include "encoder/encoder.h"
#include "lvgl.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>

// ---------------------------------------------------------------------------
// LVGL draw buffer  (~20KB: 320 × 32 × 2 bytes)
// ---------------------------------------------------------------------------
#define LV_BUF_ROWS     32
static lv_color_t _lv_buf1[DISP_WIDTH * LV_BUF_ROWS];
static lv_color_t _lv_buf2[DISP_WIDTH * LV_BUF_ROWS];
static lv_disp_draw_buf_t _draw_buf;
static lv_disp_drv_t      _disp_drv;
static lv_disp_t         *_disp = NULL;

// ---------------------------------------------------------------------------
// Encoders
// ---------------------------------------------------------------------------
static encoder_t *_enc_freq = NULL;
static encoder_t *_enc_amp  = NULL;

// Frequency step cycle (short press on freq encoder)
static const float _freq_steps[]     = { FREQ_STEP_FINE, FREQ_STEP_MEDIUM, FREQ_STEP_COARSE };
static const char *_freq_step_strs[] = { "1 Hz", "10 Hz", "100 Hz" };
static uint8_t     _freq_step_idx    = 1;   // default: 10 Hz

// ---------------------------------------------------------------------------
// Audio constants
// ---------------------------------------------------------------------------
#define FULL_SCALE_VRMS  2.2f     // measured at 100% amplitude, 1kHz
#define ATT_DB_MIN      -80.0f   // attenuation display lower limit

// ---------------------------------------------------------------------------
// Colour palette
// ---------------------------------------------------------------------------
#define COL_BLUE    lv_color_make(0x29, 0x9D, 0xFF)   // freq arc
#define COL_GREEN   lv_color_make(0x00, 0xCC, 0x66)   // att arc
#define COL_WHITE   lv_color_white()                   // main values
#define COL_GREY    lv_color_make(0xAA, 0xAA, 0xAA)   // units / titles
#define COL_YELLOW  lv_color_make(0xFF, 0xD7, 0x00)   // step / Vrms / channel
#define COL_RED     lv_color_make(0xFF, 0x40, 0x40)   // MUTE
#define COL_DARK    lv_color_make(0x30, 0x30, 0x30)   // arc background track

// ---------------------------------------------------------------------------
// Layout constants
//
//  Display: 320 × 172 (landscape)
//
//  ┌──────────────────────────────────────────┐
//  │  FREQ                  ATT               │ ← titles y=4
//  │ ╭──────────╮      ╭──────────╮          │
//  │╱            ╲    ╱            ╲          │
//  ││   1.00      │  │   -6.0      │          │
//  ││   kHz       │  │   dB        │          │
//  │╲   10 Hz    ╱    ╲   1.10V   ╱          │
//  │ ╰──────────╯      ╰──────────╯          │
//  └──────────────────────────────────────────┘
//
//  Arc size: 130 × 130, arc width: 10px
//  Left arc:  pos (10, 20),  centre (75, 85)
//  Right arc: pos (180, 20), centre (245, 85)
// ---------------------------------------------------------------------------
#define ARC_SIZE    130
#define ARC_WIDTH   10

#define FREQ_ARC_X  10
#define FREQ_ARC_Y  20
#define FREQ_CX     (FREQ_ARC_X + ARC_SIZE / 2)   // 75
#define FREQ_CY     (FREQ_ARC_Y + ARC_SIZE / 2)   // 85

#define ATT_ARC_X   180
#define ATT_ARC_Y   20
#define ATT_CX      (ATT_ARC_X + ARC_SIZE / 2)    // 245
#define ATT_CY      (ATT_ARC_Y + ARC_SIZE / 2)    // 85

#define LBL_W       80

// Vertical offsets from arc centre for three label rows
#define LBL_ROW0_DY  (-30)   // main value  (font_22)
#define LBL_ROW1_DY  (-1)    // unit        (font_14)
#define LBL_ROW2_DY  ( 20)   // sub-value   (font_14)

// ---------------------------------------------------------------------------
// Widget references
// ---------------------------------------------------------------------------
static lv_obj_t *_arc_freq      = NULL;
static lv_obj_t *_arc_att       = NULL;

static lv_obj_t *_lbl_freq_val  = NULL;   // "1.00"        white   font_22
static lv_obj_t *_lbl_freq_unit = NULL;   // "kHz"         grey    font_14
static lv_obj_t *_lbl_freq_step = NULL;   // "10 Hz"       yellow  font_14

static lv_obj_t *_lbl_att_val   = NULL;   // "-6.0"/"MUTE" white/red font_22
static lv_obj_t *_lbl_att_unit  = NULL;   // "dB"          grey    font_14
static lv_obj_t *_lbl_att_sub   = NULL;   // Vrms/ch/---   yellow  font_14

// ---------------------------------------------------------------------------
// Helper: create a fixed-width centre-aligned label
// ---------------------------------------------------------------------------
static lv_obj_t *_make_label(lv_obj_t *parent,
                              const char *text,
                              const lv_font_t *font,
                              lv_color_t color,
                              lv_coord_t cx, lv_coord_t y,
                              lv_coord_t w)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, color, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(lbl, 0, LV_PART_MAIN);
    lv_obj_set_width(lbl, w);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(lbl, cx - w / 2, y);
    return lbl;
}

// ---------------------------------------------------------------------------
// Helper: create a styled display-only arc
// ---------------------------------------------------------------------------
static lv_obj_t *_make_arc(lv_obj_t *parent,
                            lv_coord_t x, lv_coord_t y,
                            lv_color_t color)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, ARC_SIZE, ARC_SIZE);
    lv_obj_set_pos(arc, x, y);

    // 270° sweep: start 135°, end 45° (clockwise)
    lv_arc_set_bg_angles(arc, 135, 45);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);

    lv_obj_set_style_arc_color(arc, color,    LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, COL_DARK, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, ARC_WIDTH, LV_PART_MAIN);

    // Display only — hide knob, disable interaction
    lv_obj_set_style_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    return arc;
}

// ---------------------------------------------------------------------------
// Mapping helpers
// ---------------------------------------------------------------------------

// Logarithmic frequency → arc value 0–100
// 20Hz → 0,  20kHz → 100
static uint8_t _freq_to_arc(float hz) {
    static const float log_min = 1.30103f;   // log10(20)
    static const float log_rng = 3.0f;       // log10(20000) - log10(20)
    float v = (log10f(hz) - log_min) / log_rng * 100.0f;
    if (v < 0.0f)   v = 0.0f;
    if (v > 100.0f) v = 100.0f;
    return (uint8_t)v;
}

// Amplitude % → dB   (0% → ATT_DB_MIN to avoid log(0))
static float _amp_to_db(uint8_t pct) {
    if (pct == 0) return ATT_DB_MIN;
    float db = 20.0f * log10f(pct / 100.0f);
    return db < ATT_DB_MIN ? ATT_DB_MIN : db;
}

// Amplitude % → Vrms
static float _amp_to_vrms(uint8_t pct) {
    return FULL_SCALE_VRMS * (pct / 100.0f);
}

// ---------------------------------------------------------------------------
// Screen construction
// ---------------------------------------------------------------------------
static void _create_screen(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    // Titles
    _make_label(scr, "FREQ", &lv_font_montserrat_14, COL_GREY,
                FREQ_CX, FREQ_ARC_Y - 16, LBL_W);
    _make_label(scr, "ATT",  &lv_font_montserrat_14, COL_GREY,
                ATT_CX,  ATT_ARC_Y  - 16, LBL_W);

    // Arcs
    _arc_freq = _make_arc(scr, FREQ_ARC_X, FREQ_ARC_Y, COL_BLUE);
    _arc_att  = _make_arc(scr, ATT_ARC_X,  ATT_ARC_Y,  COL_GREEN);

    // Frequency labels
    _lbl_freq_val  = _make_label(scr, "1.00",
                                 &lv_font_montserrat_22, COL_WHITE,
                                 FREQ_CX, FREQ_CY + LBL_ROW0_DY, LBL_W);
    _lbl_freq_unit = _make_label(scr, "kHz",
                                 &lv_font_montserrat_14, COL_GREY,
                                 FREQ_CX, FREQ_CY + LBL_ROW1_DY, LBL_W);
    _lbl_freq_step = _make_label(scr, _freq_step_strs[_freq_step_idx],
                                 &lv_font_montserrat_14, COL_YELLOW,
                                 FREQ_CX, FREQ_CY + LBL_ROW2_DY, LBL_W);

    // Attenuation labels
    _lbl_att_val  = _make_label(scr, "0.0",
                                &lv_font_montserrat_22, COL_WHITE,
                                ATT_CX, ATT_CY + LBL_ROW0_DY, LBL_W);
    _lbl_att_unit = _make_label(scr, "dB",
                                &lv_font_montserrat_14, COL_GREY,
                                ATT_CX, ATT_CY + LBL_ROW1_DY, LBL_W);
    _lbl_att_sub  = _make_label(scr, "2.20V",
                                &lv_font_montserrat_14, COL_YELLOW,
                                ATT_CX, ATT_CY + LBL_ROW2_DY, LBL_W);
}

// ---------------------------------------------------------------------------
// Display refresh
// ---------------------------------------------------------------------------
static void _refresh_display(void) {
    static float           last_freq = -1.0f;
    static uint8_t         last_amp  = 255;
    static audio_channel_t last_ch   = (audio_channel_t)255;

    float           freq = audio_get_freq();
    uint8_t         amp  = audio_get_amplitude();
    audio_channel_t ch   = audio_get_channel();

    // --- Frequency ---
    if (freq != last_freq) {
        last_freq = freq;

        lv_arc_set_value(_arc_freq, _freq_to_arc(freq));

        char buf[12];
        if (freq >= 1000.0f)
            snprintf(buf, sizeof(buf), "%.2f", freq / 1000.0f);
        else
            snprintf(buf, sizeof(buf), "%.0f", freq);
        lv_label_set_text(_lbl_freq_val, buf);
        lv_label_set_text(_lbl_freq_unit, freq >= 1000.0f ? "kHz" : "Hz");
    }

    // --- Attenuation / channel ---
    if (amp != last_amp || ch != last_ch) {
        last_amp = amp;
        last_ch  = ch;

        if (ch == AUDIO_CH_MUTE) {
            // Arc collapses to zero, value shows MUTE in red
            lv_arc_set_value(_arc_att, 0);
            lv_label_set_text(_lbl_att_val, "MUTE");
            lv_obj_set_style_text_color(_lbl_att_val, COL_RED, LV_PART_MAIN);
            lv_obj_add_flag(_lbl_att_unit, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(_lbl_att_sub, "---");
        } else {
            // Arc tracks amplitude % linearly (0–100 maps directly to arc range)
            lv_arc_set_value(_arc_att, amp);

            float db   = _amp_to_db(amp);
            char db_buf[10];
            snprintf(db_buf, sizeof(db_buf), "%.1f", db);
            lv_label_set_text(_lbl_att_val, db_buf);
            lv_obj_set_style_text_color(_lbl_att_val, COL_WHITE, LV_PART_MAIN);
            lv_obj_clear_flag(_lbl_att_unit, LV_OBJ_FLAG_HIDDEN);

            // Sub-label: Vrms for ALL, channel indicator for L/R
            if (ch == AUDIO_CH_ALL) {
                char v_buf[10];
                snprintf(v_buf, sizeof(v_buf), "%.2fV", _amp_to_vrms(amp));
                lv_label_set_text(_lbl_att_sub, v_buf);
            } else if (ch == AUDIO_CH_LEFT) {
                lv_label_set_text(_lbl_att_sub, "L CH");
            } else {
                lv_label_set_text(_lbl_att_sub, "R CH");
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Encoder handling
// ---------------------------------------------------------------------------
static void _handle_encoders(void) {
    // --- Frequency encoder ---
    int8_t df = encoder_get_delta(_enc_freq);
    if (df != 0) {
        float f = audio_get_freq() + df * _freq_steps[_freq_step_idx];
        audio_set_freq(f);
    }

    // Short press: cycle step size
    if (encoder_button_pressed(_enc_freq)) {
        _freq_step_idx = (_freq_step_idx + 1) % 3;
        lv_label_set_text(_lbl_freq_step, _freq_step_strs[_freq_step_idx]);
    }

    (void)encoder_button_held(_enc_freq);   // reserved

    // --- Amplitude encoder ---
    int8_t da = encoder_get_delta(_enc_amp);
    if (da != 0) {
        // Encoder adjusts amplitude even while in L/R channel mode.
        // Mute blocks amplitude changes — must cycle out of mute first.
        if (audio_get_channel() != AUDIO_CH_MUTE) {
            int32_t amp = (int32_t)audio_get_amplitude() + da * AMP_STEP_PCT;
            if (amp < 0)   amp = 0;
            if (amp > 100) amp = 100;
            audio_set_amplitude((uint8_t)amp);
        }
    }

    // Short press: cycle ALL → LEFT → RIGHT → MUTE → ALL
    if (encoder_button_pressed(_enc_amp)) {
        audio_channel_t current = audio_get_channel();
        audio_channel_t next;
        switch (current) {
            case AUDIO_CH_ALL:   next = AUDIO_CH_LEFT;  break;
            case AUDIO_CH_LEFT:  next = AUDIO_CH_RIGHT; break;
            case AUDIO_CH_RIGHT: next = AUDIO_CH_MUTE;  break;
            case AUDIO_CH_MUTE:  next = AUDIO_CH_ALL;   break;
            default:             next = AUDIO_CH_ALL;   break;
        }
        audio_set_channel(next);
    }

    (void)encoder_button_held(_enc_amp);    // reserved
}

// ---------------------------------------------------------------------------
// LVGL 5ms tick
// ---------------------------------------------------------------------------
static bool _lvgl_tick_cb(struct repeating_timer *t) {
    (void)t;
    lv_tick_inc(5);
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void ui_init(encoder_t *enc_freq, encoder_t *enc_amp) {
    _enc_freq = enc_freq;
    _enc_amp  = enc_amp;

    st7789_init();
    lv_init();

    lv_disp_draw_buf_init(&_draw_buf, _lv_buf1, _lv_buf2,
                          DISP_WIDTH * LV_BUF_ROWS);
    lv_disp_drv_init(&_disp_drv);
    _disp_drv.draw_buf  = &_draw_buf;
    _disp_drv.flush_cb  = st7789_flush;
    _disp_drv.hor_res   = DISP_WIDTH;
    _disp_drv.ver_res   = DISP_HEIGHT;
    _disp = lv_disp_drv_register(&_disp_drv);

    _create_screen();

    // Prime arcs to match audio defaults
    lv_arc_set_value(_arc_freq, _freq_to_arc(AUDIO_FREQ_DEFAULT_HZ));
    lv_arc_set_value(_arc_att,  AUDIO_AMP_DEFAULT_PCT);   // linear: direct %

    st7789_set_backlight(80);
}

void ui_run(void) {
    struct repeating_timer tick_timer;
    add_repeating_timer_ms(-5, _lvgl_tick_cb, NULL, &tick_timer);

    while (true) {
        _handle_encoders();
        _refresh_display();
        lv_task_handler();
        sleep_ms(5);
    }
}
