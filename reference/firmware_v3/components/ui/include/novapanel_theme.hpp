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

#define NP_F_XS    (&nova_font_14)
#define NP_F_SM    (&nova_font_14)
#define NP_F_MD    (&nova_font_14)
#define NP_F_LG    (&nova_font_16)
#define NP_F_XS_BOLD (&nova_font_bold_14)
#define NP_F_SM_BOLD (&nova_font_bold_16)
#define NP_F_XL    (&nova_font_20)
#define NP_F_2XL   (&nova_font_24)
#define NP_F_3XL   (&nova_font_28)
#define NP_F_4XL   (&nova_font_32)
#define NP_F_TITLE_SM (&nova_font_bold_20)
#define NP_F_TITLE_MD (&nova_font_bold_24)
#define NP_F_TITLE_LG (&nova_font_bold_28)
#define NP_F_ICON_SM (&lv_font_material_18)
#define NP_F_ICON_XS (&lv_font_material_16)
#define NP_F_ICON    (&lv_font_material_18)
#define NP_F_ICON_LG (&lv_font_material_24)
#define NP_F_ICON_WEATHER (&lv_font_material_24)
#define NP_F_ICON_WEATHER_LG (&lv_font_material_48)

#define NP_F_HERO  (&nova_font_48)
#define NP_F_TEMP   (&nova_font_extrabold_64)
#define NP_F_CLOCK  (&nova_font_extrabold_92)

#define NP_I_HOME      "\xEE\xA2\x8A"
#define NP_I_AGENDA    "\xEE\xA4\xB5"
#define NP_I_LIST      "\xEE\xA6\xB9"
#define NP_I_CALENDAR  "\xEE\xA2\xB5"
#define NP_I_CHART     "\xEE\xAF\x85"
#define NP_I_DEVICE    "\xEE\x8A\x83"
#define NP_I_BELL      "\xEE\x9F\xB4"
#define NP_I_REFRESH   "\xEE\x97\x95"
#define NP_I_SETTINGS  "\xEE\xA2\xB8"
#define NP_I_FOCUS     "\xEE\xBF\x80"
#define NP_I_INFO      "\xEE\xA2\x8E"
#define NP_I_WIFI      "\xEE\x98\xBE"
#define NP_I_MUSIC     "\xEE\x90\x85"
#define NP_I_PAUSE     "\xEE\x86\xA2"
#define NP_I_PLAY      "\xEE\x87\x84"
#define NP_I_IMAGE     "\xEE\x8F\xB4"
#define NP_I_SUN       "\xEE\xA0\x9A"
#define NP_I_CLOSE     "\xEE\x97\x8D"
#define NP_I_CHECK     "\xEE\x97\x8A"
#define NP_I_VALUE_UP  "\xEE\x97\x87"
#define NP_I_VALUE_DOWN "\xEE\x97\x85"
#define NP_I_ADD       "\xEE\x85\x85"
#define NP_I_DELETE    "\xEE\xA1\xB2"
#define NP_I_ACCOUNT   "\xEE\xA1\x93"
#define NP_I_HELP      "\xEE\xA2\x87"
#define NP_I_WARNING   "\xEE\x80\x82"
#define NP_I_AIR       "\xEE\xBF\x98"
#define NP_I_MIC       "\xEE\x80\xA9"
#define NP_I_MIC_OFF   "\xEE\x80\xAB"
#define NP_I_STOP      "\xEE\xBD\xB1"
#define NP_I_VOLUME_DOWN "\xEE\x81\x8D"
#define NP_I_VOLUME_UP   "\xEE\x81\x90"
#define NP_I_VOLUME_MUTE "\xEE\x81\x8E"
#define NP_I_VOLUME_OFF  "\xEE\x81\x8F"
#define NP_I_NO_SOUND    "\xEE\x9C\x90"
#define NP_I_ARROW_LEFT  "\xEE\x8C\x94"
#define NP_I_ARROW_RIGHT "\xEE\x8C\x95"
#define NP_I_ARROW_UP    "\xEE\x8C\x96"
#define NP_I_ARROW_DOWN  "\xEE\x8C\x93"
#define NP_I_SYSTEM_ALERT "\xEF\x85\x83"
#define NP_I_RESET_SETTINGS "\xEF\x91\xBF"
#define NP_I_COFFEE     "\xEE\x95\x81"
#define NP_I_MODE_NIGHT "\xEF\x80\xB6"
#define NP_I_LIGHTBULB  "\xEF\x8F\xA3"
#define NP_I_LIGHT_OFF  "\xEE\xA6\xB8"
#define NP_I_LIGHT_ON   "\xEE\x83\xB0"
#define NP_I_BLUETOOTH  "\xEE\x86\xA7"
#define NP_I_BLUETOOTH_OFF "\xEE\x86\xA9"
#define NP_I_SYNC       "\xEE\x98\xA7"
#define NP_I_SEARCH     "\xEE\xA2\xB6"
#define NP_I_CACHED     "\xEE\xA1\xAA"
#define NP_I_CALCULATE  "\xEE\xA9\x9F"
#define NP_I_BATTERY_1  "\xEF\x89\x97"
#define NP_I_BATTERY_2  "\xEF\x89\x96"
#define NP_I_BATTERY_3  "\xEF\x89\x95"
#define NP_I_BATTERY_4  "\xEF\x89\x94"
#define NP_I_BATTERY_5  "\xEF\x89\x93"
#define NP_I_BATTERY_6  "\xEF\x89\x92"
#define NP_I_BATTERY_FULL "\xEF\x89\x8F"
#define NP_I_BATTERY_ALERT "\xEF\x89\x91"
#define NP_I_BATTERY_BOLT  "\xEF\x89\x90"
#define NP_I_WIFI_1     "\xEE\x93\x8A"
#define NP_I_WIFI_2     "\xEE\x93\x99"
#define NP_I_WIFI_OFF   "\xEE\x99\x88"
#define NP_I_WIFI_ALERT "\xEE\xBC\x9B"
#define NP_I_WIFI_LOCK  "\xEE\xBC\x9A"

#define NP_I_WEATHER_HAIL "\xEF\x99\xBF"
#define NP_I_WEATHER_SNOW "\xEE\x8B\x8D"
#define NP_I_WEATHER_CLEAR "\xEF\x85\x97"
#define NP_I_WEATHER_CLOUDY "\xEE\x8A\xBD"
#define NP_I_WEATHER_FOG "\xEE\xA0\x98"
#define NP_I_WEATHER_HUMIDITY_HIGH "\xEF\x85\xA3"
#define NP_I_WEATHER_HUMIDITY_MID "\xEF\x85\xA4"
#define NP_I_WEATHER_HUMIDITY_LOW "\xEF\x85\xA5"
#define NP_I_WEATHER_PARTLY_CLOUDY_DAY "\xEF\x85\xB2"
#define NP_I_WEATHER_PARTLY_CLOUDY_NIGHT "\xEF\x85\xB4"
#define NP_I_WEATHER_RAIN "\xEF\x85\xB6"
#define NP_I_WEATHER_SUNNY "\xEE\xA0\x9A"
#define NP_I_WEATHER_TEMP_DOWN "\xEF\x8D\xBA"
#define NP_I_WEATHER_TEMP_UP "\xEF\x8D\xB9"
#define NP_I_WEATHER_THUNDER "\xEE\xAF\x9B"
#define NP_I_WEATHER_RAIN_HEAVY "\xEF\x98\x9F"
#define NP_I_WEATHER_RAIN_LIGHT "\xEF\x98\x9E"
#define NP_I_WEATHER_RAIN_SNOW "\xEF\x98\x9D"

#define NP_I_CHEV_L    NP_I_ARROW_LEFT
#define NP_I_CHEV_R    NP_I_ARROW_RIGHT

#define NP_I_MENU_HOME      "\xEE\xA2\x8A"
#define NP_I_MENU_AGENDA    "\xEE\xA4\xB5"
#define NP_I_MENU_MARKET    "\xEE\xBE\x92"
#define NP_I_MENU_HOME_DEV  "\xEE\x8A\x83"
#define NP_I_MENU_ALARMS    "\xEE\x9F\xB4"
#define NP_I_MENU_WEATHER   "\xEF\x85\xB2"
#define NP_I_MENU_POMODORO  "\xEE\xBF\x80"

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

static inline void np_icon_zoom(lv_obj_t *obj, uint16_t pct)
{
    (void)obj;
    (void)pct;
}



// ── Helper: toggle switch ────────────────────────────────────
static inline lv_obj_t *np_toggle(lv_obj_t *parent, bool checked)
{
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 44, 24);
    lv_obj_set_style_bg_color(sw, NP_C_ITEM_BG2,        LV_PART_MAIN);
    lv_obj_set_style_border_color(sw, NP_C_BORDER,       LV_PART_MAIN);
    lv_obj_set_style_border_width(sw, 1,                 LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x2C3820),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, NP_C_TEXT_DIM,         LV_PART_KNOB);
    lv_obj_set_style_bg_color(sw, NP_C_ACCENT,
                              LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_pad_all(sw, 2, LV_PART_KNOB);
    if (checked) lv_obj_add_state(sw, LV_STATE_CHECKED);
    return sw;
}

// ── Helper: horizontal separator ────────────────────────────
static inline lv_obj_t *np_hsep(lv_obj_t *parent)
{
    lv_obj_t *s = lv_obj_create(parent);
    lv_obj_set_size(s, lv_pct(100), 1);
    lv_obj_set_style_bg_color(s,     NP_C_BORDER, 0);
    lv_obj_set_style_bg_opa(s,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s,      0, 0);
    lv_obj_set_style_radius(s,       0, 0);
    return s;
}

// ── Helper: row flex container ───────────────────────────────
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

// ── Helper: column flex container ────────────────────────────
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

// ── Helper: accent chip button ───────────────────────────────
static inline lv_obj_t *np_chip(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(btn,     NP_C_ACCENT_BG, 0);
    lv_obj_set_style_border_color(btn, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn,       15, 0);
    lv_obj_set_style_pad_hor(btn,      14, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl,  NP_F_SM,     0);
    lv_obj_set_style_text_color(lbl, NP_C_ACCENT, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

// ── Helper: icon button — uses material_16 for topbar icons ──
static inline lv_obj_t *np_icon_btn(lv_obj_t *parent,
                                    const char *symbol,
                                    lv_color_t icon_color = NP_C_TEXT_DIM)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_set_style_bg_color(btn,     NP_C_ITEM_BG, 0);
    lv_obj_set_style_border_color(btn, NP_C_BORDER,  0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *ic = lv_label_create(btn);
    lv_label_set_text(ic, symbol);
    lv_obj_set_style_text_font(ic, NP_F_ICON_SM, 0);
    lv_obj_set_style_text_color(ic, icon_color, 0);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);
    return btn;
}
