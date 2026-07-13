#pragma once

#include "lvgl.h"
#include "novapanel_fonts.hpp"

namespace nova::ui_theme {

constexpr int kScreenWidth = 1024;
constexpr int kScreenHeight = 600;
constexpr int kRailWidth = 62;
constexpr int kTopbarHeight = 60;
constexpr int kDotsHeight = 16;
constexpr int kPad = 14;
constexpr int kGap = 12;
constexpr int kCardRadius = 12;
constexpr int kButtonRadius = 9;

inline lv_color_t color_bg() { return lv_color_hex(0x0D0F18); }
inline lv_color_t color_rail_bg() { return lv_color_hex(0x0B0D16); }
inline lv_color_t color_panel() { return lv_color_hex(0x141721); }
inline lv_color_t color_panel_alt() { return lv_color_hex(0x1B1E2D); }
inline lv_color_t color_border() { return lv_color_hex(0x1E2235); }
inline lv_color_t color_separator() { return lv_color_hex(0x181B28); }
inline lv_color_t color_accent() { return lv_color_hex(0xE8A83C); }
inline lv_color_t color_accent_bg() { return lv_color_hex(0x1C1900); }
inline lv_color_t color_accent_border() { return lv_color_hex(0x2C2700); }
inline lv_color_t color_text() { return lv_color_hex(0xE8EAF2); }
inline lv_color_t color_text_medium() { return lv_color_hex(0xBCC0CE); }
inline lv_color_t color_muted() { return lv_color_hex(0x7A8298); }
inline lv_color_t color_disabled() { return lv_color_hex(0x464E64); }
inline lv_color_t color_dark_fg() { return lv_color_hex(0x090C12); }
inline lv_color_t color_green() { return lv_color_hex(0x4ABB78); }
inline lv_color_t color_green_bg() { return lv_color_hex(0x0F1D15); }
inline lv_color_t color_green_border() { return lv_color_hex(0x1A3828); }
inline lv_color_t color_red() { return lv_color_hex(0xD05252); }
inline lv_color_t color_blue() { return lv_color_hex(0x4F7ECB); }
inline lv_color_t color_dot_inactive() { return lv_color_hex(0x252A3C); }

inline const lv_font_t* font_xs() { return &nova_font_14; }
inline const lv_font_t* font_small() { return &nova_font_14; }
inline const lv_font_t* font_body() { return &nova_font_16; }
inline const lv_font_t* font_large() { return &nova_font_20; }
inline const lv_font_t* font_title() { return &nova_font_bold_24; }
inline const lv_font_t* font_title_large() { return &nova_font_bold_28; }
inline const lv_font_t* font_icon_xs() { return &lv_font_material_16; }
inline const lv_font_t* font_icon() { return &lv_font_material_18; }
inline const lv_font_t* font_icon_large() { return &lv_font_material_24; }
inline const lv_font_t* font_hero() { return &nova_font_48; }

inline constexpr const char* icon_home() { return "\xEE\xA2\x8A"; }
inline constexpr const char* icon_list() { return "\xEE\xA6\xB9"; }
inline constexpr const char* icon_wifi() { return "\xEE\x98\xBE"; }
inline constexpr const char* icon_wifi_lock() { return "\xEE\xBC\x9A"; }
inline constexpr const char* icon_wifi_off() { return "\xEE\x99\x88"; }
inline constexpr const char* icon_bell() { return "\xEE\x9F\xB4"; }
inline constexpr const char* icon_settings() { return "\xEE\xA2\xB8"; }
inline constexpr const char* icon_refresh() { return "\xEE\x97\x95"; }
inline constexpr const char* icon_check() { return "\xEE\x97\x8A"; }
inline constexpr const char* icon_arrow_left() { return "\xEE\x8C\x94"; }
inline constexpr const char* icon_arrow_right() { return "\xEE\x8C\x95"; }
inline constexpr const char* icon_warning() { return "\xEE\x80\x82"; }
inline constexpr const char* icon_clock() { return "\xEE\xA2\xAE"; }

inline void clear_style(lv_obj_t* obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

inline lv_obj_t* label(lv_obj_t* parent,
                       const char* text,
                       const lv_font_t* font,
                       lv_color_t color) {
    lv_obj_t* out = lv_label_create(parent);
    lv_label_set_text(out, text);
    lv_obj_set_style_text_font(out, font, 0);
    lv_obj_set_style_text_color(out, color, 0);
    return out;
}

inline lv_obj_t* card(lv_obj_t* parent) {
    lv_obj_t* out = lv_obj_create(parent);
    lv_obj_set_style_bg_color(out, color_panel(), 0);
    lv_obj_set_style_bg_opa(out, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(out, color_border(), 0);
    lv_obj_set_style_border_width(out, 1, 0);
    lv_obj_set_style_radius(out, kCardRadius, 0);
    lv_obj_set_style_pad_all(out, 16, 0);
    lv_obj_set_scrollbar_mode(out, LV_SCROLLBAR_MODE_OFF);
    return out;
}

inline lv_obj_t* row(lv_obj_t* parent) {
    lv_obj_t* out = lv_obj_create(parent);
    clear_style(out);
    lv_obj_set_size(out, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(out, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(out, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return out;
}

inline lv_obj_t* col(lv_obj_t* parent) {
    lv_obj_t* out = lv_obj_create(parent);
    clear_style(out);
    lv_obj_set_size(out, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(out, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(out, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return out;
}

inline lv_obj_t* hsep(lv_obj_t* parent) {
    lv_obj_t* out = lv_obj_create(parent);
    lv_obj_set_size(out, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(out, color_border(), 0);
    lv_obj_set_style_bg_opa(out, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(out, 0, 0);
    lv_obj_set_style_pad_all(out, 0, 0);
    lv_obj_set_style_radius(out, 0, 0);
    return out;
}

inline lv_obj_t* chip(lv_obj_t* parent, const char* text) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(btn, color_accent_bg(), 0);
    lv_obj_set_style_border_color(btn, color_accent_border(), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 15, 0);
    lv_obj_set_style_pad_hor(btn, 14, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t* lbl = label(btn, text, font_small(), color_accent());
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

inline lv_obj_t* icon_button(lv_obj_t* parent,
                             const char* symbol,
                             lv_color_t icon_color = color_disabled()) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_set_style_bg_color(btn, color_panel(), 0);
    lv_obj_set_style_border_color(btn, color_border(), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, kButtonRadius, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t* icon = label(btn, symbol, font_icon(), icon_color);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

}  // namespace nova::ui_theme
