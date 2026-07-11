#pragma once

#include "lvgl.h"

namespace nova::ui_theme {

constexpr int kScreenWidth = 1024;
constexpr int kScreenHeight = 600;
constexpr int kRailWidth = 72;
constexpr int kTopbarHeight = 56;

inline lv_color_t color_bg() { return lv_color_hex(0x0D1117); }
inline lv_color_t color_panel() { return lv_color_hex(0x141A22); }
inline lv_color_t color_panel_alt() { return lv_color_hex(0x10151C); }
inline lv_color_t color_border() { return lv_color_hex(0x232B36); }
inline lv_color_t color_accent() { return lv_color_hex(0xE8A83C); }
inline lv_color_t color_text() { return lv_color_hex(0xE7ECF2); }
inline lv_color_t color_muted() { return lv_color_hex(0x7C8796); }
inline const lv_font_t* font_title() { return &lv_font_montserrat_14; }
inline const lv_font_t* font_body() { return &lv_font_montserrat_14; }
inline const lv_font_t* font_small() { return &lv_font_montserrat_14; }

}  // namespace nova::ui_theme
