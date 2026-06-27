#pragma once

#include <cstddef>

#include "lvgl.h"
#include "novapanel_fonts.hpp"

#define NP_C_BG           lv_color_hex(0x0D0F18)
#define NP_C_CARD         lv_color_hex(0x141721)
#define NP_C_CARD2        lv_color_hex(0x1B1E2D)
#define NP_C_BORDER       lv_color_hex(0x1E2235)
#define NP_C_SEP          lv_color_hex(0x181B28)
#define NP_C_ACCENT       lv_color_hex(0xE8A83C)
#define NP_C_ACCENT_BG    lv_color_hex(0x1C1900)
#define NP_C_ACCENT_BD    lv_color_hex(0x2C2700)
#define NP_C_TEXT         lv_color_hex(0xE8EAF2)
#define NP_C_TEXT_DIM     lv_color_hex(0x7A8298)
#define NP_C_TEXT_MUTED   lv_color_hex(0x464E64)
#define NP_C_TEXT_DARK    lv_color_hex(0x30364A)
#define NP_C_TEXT_MED     lv_color_hex(0xBCC0CE)
#define NP_C_GREEN        lv_color_hex(0x4ABB78)
#define NP_C_GREEN_BG     lv_color_hex(0x0F1D15)
#define NP_C_GREEN_BD     lv_color_hex(0x1A3828)
#define NP_C_BLUE         lv_color_hex(0x4F7ECB)
#define NP_C_RED          lv_color_hex(0xD05252)
#define NP_C_RED_BG       lv_color_hex(0x3A1818)
#define NP_C_RED_BD       lv_color_hex(0x5A2020)
#define NP_C_PURPLE       lv_color_hex(0xB77ABB)
#define NP_C_RAIL_BG      lv_color_hex(0x0B0D16)
#define NP_C_ITEM_BG      lv_color_hex(0x141721)
#define NP_C_ITEM_BG2     lv_color_hex(0x252A3C)
#define NP_C_DARK_FG      lv_color_hex(0x090C12)

#define NP_W              1024
#define NP_H              600
#define NP_RAIL_W         62
#define NP_TOPBAR_H       60
#define NP_CONTENT_W      (NP_W - NP_RAIL_W)
#define NP_CONTENT_H      (NP_H - NP_TOPBAR_H)
#define NP_PAD            14
#define NP_GAP            12
#define NP_R_CARD         12
#define NP_R_BTN          9
#define NP_R_SM           7
#define NP_BDW            1
#define NP_DOTS_H         16

#define NP_F_XS    (&lv_font_montserrat_14)
#define NP_F_SM    (&lv_font_montserrat_14)
#define NP_F_MD    (&lv_font_montserrat_14)
#define NP_F_LG    (&lv_font_montserrat_16)
#define NP_F_XL    (&lv_font_montserrat_20)
#define NP_F_2XL   (&lv_font_montserrat_24)
#define NP_F_3XL   (&lv_font_montserrat_28)
#define NP_F_4XL   (&lv_font_montserrat_32)
#define NP_F_ICON_SM (&fontawesome_16)
#define NP_F_ICON    (&fontawesome_20)
#define NP_F_HERO  (&lv_font_montserrat_48)
#define NP_F_CLOCK  (&lv_font_montserrat_bold_94)

#define NP_I_HOME      "\xEF\x80\x95"
#define NP_I_LIST      "\xEF\x80\xBA"
#define NP_I_CALENDAR  "\xEF\x81\xB3"
#define NP_I_CHART     "\xEF\x88\x81"
#define NP_I_DEVICE    "\xEF\x84\x88"
#define NP_I_BELL      "\xEF\x83\xB3"
#define NP_I_REFRESH   "\xEF\x80\xA1"
#define NP_I_SETTINGS  "\xEF\x80\x93"
#define NP_I_FOCUS     "\xEF\x85\x80"
#define NP_I_INFO      "\xEF\x84\xA9"
#define NP_I_WIFI      "\xEF\x87\xAB"
#define NP_I_MUSIC     "\xEF\x80\x81"
#define NP_I_PAUSE     "\xEF\x81\x8C"
#define NP_I_PLAY      "\xEF\x81\x8B"
#define NP_I_IMAGE     "\xEF\x80\xBE"
#define NP_I_SUN       "\xEF\x86\x85"
#define NP_I_CHEV_L    "\xEF\x81\x93"
#define NP_I_CHEV_R    "\xEF\x81\x94"

static inline void np_obj_clear_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

static inline lv_obj_t *np_card(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_bg_color(o, NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(o, NP_BDW, 0);
    lv_obj_set_style_radius(o, NP_R_CARD, 0);
    lv_obj_set_style_pad_all(o, 16, 0);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    return o;
}

static inline lv_obj_t *np_label(lv_obj_t *parent,
                                 const char *text,
                                 const lv_font_t *font,
                                 lv_color_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    return l;
}


static inline lv_obj_t *np_toggle(lv_obj_t *parent, bool checked)
{
    const uint32_t indicator_checked =
        static_cast<uint32_t>(LV_PART_INDICATOR) | static_cast<uint32_t>(LV_STATE_CHECKED);
    const uint32_t knob_checked =
        static_cast<uint32_t>(LV_PART_KNOB) | static_cast<uint32_t>(LV_STATE_CHECKED);

    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 44, 24);
    lv_obj_set_style_bg_color(sw, NP_C_ITEM_BG2, LV_PART_MAIN);
    lv_obj_set_style_border_color(sw, NP_C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(sw, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x2C3820), static_cast<int>(indicator_checked));
    lv_obj_set_style_bg_color(sw, NP_C_TEXT_DIM, LV_PART_KNOB);
    lv_obj_set_style_bg_color(sw, NP_C_ACCENT, static_cast<int>(knob_checked));
    lv_obj_set_style_pad_all(sw, 2, LV_PART_KNOB);
    if (checked) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    return sw;
}
static inline lv_obj_t *np_hsep(lv_obj_t *parent)
{
    lv_obj_t *s = lv_obj_create(parent);
    lv_obj_set_size(s, lv_pct(100), 1);
    lv_obj_set_style_bg_color(s, NP_C_BORDER, 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_set_style_radius(s, 0, 0);
    return s;
}

static inline lv_obj_t *np_row(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    np_obj_clear_style(o);
    lv_obj_set_size(o, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return o;
}

static inline lv_obj_t *np_col(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    np_obj_clear_style(o);
    lv_obj_set_size(o, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return o;
}

static inline lv_obj_t *np_chip(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(btn, NP_C_ACCENT_BG, 0);
    lv_obj_set_style_border_color(btn, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 15, 0);
    lv_obj_set_style_pad_hor(btn, 14, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, NP_F_SM, 0);
    lv_obj_set_style_text_color(lbl, NP_C_ACCENT, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

static inline lv_obj_t *np_icon_btn(lv_obj_t *parent,
                                    const char *symbol,
                                    lv_color_t icon_color = NP_C_TEXT_DIM)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_set_style_bg_color(btn, NP_C_ITEM_BG, 0);
    lv_obj_set_style_border_color(btn, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *ic = lv_label_create(btn);
    lv_label_set_text(ic, symbol);
    lv_obj_set_style_text_font(ic, NP_F_ICON_SM, 0);
    lv_obj_set_style_text_color(ic, icon_color, 0);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);
    return btn;
}
