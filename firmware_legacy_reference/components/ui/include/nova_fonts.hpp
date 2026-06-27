// NovaPainel - ui/nova_fonts.hpp
// Custom Montserrat + full FontAwesome fonts (firmware/components/ui/fonts/),
// generated with lv_font_conv from Montserrat-Regular.ttf (range 32-127,
// 160-255 - ASCII + Latin-1 Supplement for accented Portuguese: ã, ç, é, í,
// õ, ú, etc., per ADR-0026) plus fontawesome-webfont.ttf (range 57344-63487 /
// 0xE000-0xF7FF, same range LVGL's LV_SYMBOL_* constants point into, but with
// the full icon set instead of LVGL's own reduced bundled subset). Named
// nova_font_* (not lv_font_montserrat_*) to avoid clashing with LVGL's own
// bundled fonts of the same name when CONFIG_LV_FONT_MONTSERRAT_* is enabled.
// Sizes match the ones enabled in sdkconfig, minus 10 (declared by neither
// LVGL nor used anywhere in this codebase): 12, 14, 16, 20, 24, 28, 32, 48.
#pragma once

#include "lvgl.h"

LV_FONT_DECLARE(nova_font_14)
LV_FONT_DECLARE(nova_font_16)
LV_FONT_DECLARE(nova_font_20)
LV_FONT_DECLARE(nova_font_24)
LV_FONT_DECLARE(nova_font_28)
LV_FONT_DECLARE(nova_font_32)
LV_FONT_DECLARE(nova_font_48)
LV_FONT_DECLARE(nova_font_64)
LV_FONT_DECLARE(nova_font_100)
LV_FONT_DECLARE(nova_font_bold_100)
