// NovaPainel - ui/src/screens/home_screen.cpp
// Real LVGL widgets (Fase 4/5). NOT host-checkable: lvgl.h has no host shim
// (see tools/scripts/host_check.sh, which skips this file on purpose).
// Faithful port of docs/design/lvgl_export_reference/screens/screen_home.c
// (+ novapanel_theme.h for colors/fonts/dimensions) - see home_screen.hpp
// for which sections are real data vs. the reference's own placeholder
// content (Agenda/Cenas rápidas/Player).
#include "home_screen.hpp"

#include <cstdio>

#include "lvgl.h"
#include "nova_fonts.hpp"

namespace nova {

namespace {
// -- Palette, from docs/design/lvgl_export_reference/novapanel_theme.h --
constexpr uint32_t kColorCard      = 0x141721;  // NP_C_CARD
constexpr uint32_t kColorCard2     = 0x1B1E2D;  // NP_C_CARD2
constexpr uint32_t kColorBorder    = 0x1E2235;  // NP_C_BORDER
constexpr uint32_t kColorSep       = 0x181B28;  // NP_C_SEP
constexpr uint32_t kColorAccent    = 0xE8A83C;  // NP_C_ACCENT
constexpr uint32_t kColorAccentBg  = 0x1C1900;  // NP_C_ACCENT_BG
constexpr uint32_t kColorText      = 0xE8EAF2;  // NP_C_TEXT
constexpr uint32_t kColorTextDim   = 0x7A8298;  // NP_C_TEXT_DIM
constexpr uint32_t kColorTextMuted = 0x464E64;  // NP_C_TEXT_MUTED
constexpr uint32_t kColorTextMed   = 0xBCC0CE;  // NP_C_TEXT_MED
constexpr uint32_t kColorGreen     = 0x4ABB78;  // NP_C_GREEN
constexpr uint32_t kColorRed       = 0xD05252;  // NP_C_RED
constexpr uint32_t kColorDarkFg    = 0x090C12;  // NP_C_DARK_FG

constexpr int kLeftW   = 362;  // LEFT_W
constexpr int kColGap  = 12;   // COL_GAP
constexpr int kRCard   = 12;   // NP_R_CARD

const char* weekday_name(int wd) {
    static const char* kDays[] = {"Domingo", "Segunda", "Terça", "Quarta",
                                  "Quinta", "Sexta", "Sábado"};
    if (wd < 0 || wd > 6) return "?";
    return kDays[wd];
}

// Formats a non-negative integer with '.' as the thousands separator
// (pt-BR convention), e.g. 53142 -> "53.142". Falls back to a plain number
// if it wouldn't fit `out`.
void format_thousands(long value, char* out, size_t out_size) {
    char raw[32];
    const int len = std::snprintf(raw, sizeof(raw), "%ld", value);
    const int groups = (len - 1) / 3;
    if (len <= 0 || static_cast<size_t>(len + groups) >= out_size) {
        std::snprintf(out, out_size, "%ld", value);
        return;
    }
    int oi = len + groups;
    out[oi--] = '\0';
    int digits_since_sep = 0;
    for (int i = len - 1; i >= 0; --i) {
        out[oi--] = raw[i];
        if (++digits_since_sep == 3 && i != 0) {
            out[oi--] = '.';
            digits_since_sep = 0;
        }
    }
}

const char* condition_name(WeatherCondition c) {
    switch (c) {
        case WeatherCondition::Clear:        return "Céu limpo";
        case WeatherCondition::Cloudy:       return "Nublado";
        case WeatherCondition::Fog:          return "Nevoeiro";
        case WeatherCondition::Drizzle:      return "Garoa";
        case WeatherCondition::Rain:         return "Chuva";
        case WeatherCondition::Snow:         return "Neve";
        case WeatherCondition::Thunderstorm: return "Tempestade";
        case WeatherCondition::Unknown:      return "--";
    }
    return "--";
}

// FontAwesome 4 glyphs (same source/range as nova_font_*'s icon set, see
// nova_fonts.hpp) that LVGL doesn't expose as an LV_SYMBOL_* constant - raw
// UTF-8 bytes for the codepoint, same convention lv_symbol_def.h itself uses
// (e.g. LV_SYMBOL_HOME = "\xEF\x80\x95" for U+F015).
constexpr const char* kWeatherIconSun       = "\xEF\x86\x85";  // fa-sun-o, U+F185
constexpr const char* kWeatherIconCloud     = "\xEF\x83\x82";  // fa-cloud, U+F0C2
constexpr const char* kWeatherIconUmbrella  = "\xEF\x83\xA9";  // fa-umbrella, U+F0E9 (rain/drizzle)
constexpr const char* kWeatherIconBolt      = LV_SYMBOL_CHARGE; // fa-bolt, U+F0E7 (thunderstorm)
constexpr const char* kWeatherIconUnknown   = LV_SYMBOL_REFRESH; // placeholder until first fetch lands

const char* weather_icon_symbol(WeatherCondition c) {
    switch (c) {
        case WeatherCondition::Clear:        return kWeatherIconSun;
        case WeatherCondition::Cloudy:
        case WeatherCondition::Fog:          return kWeatherIconCloud;
        case WeatherCondition::Drizzle:
        case WeatherCondition::Rain:
        case WeatherCondition::Snow:         return kWeatherIconUmbrella;  // FA4 has no snowflake glyph, closest available

        case WeatherCondition::Thunderstorm: return kWeatherIconBolt;
        case WeatherCondition::Unknown:      return kWeatherIconUnknown;
    }
    return kWeatherIconUnknown;
}

// -- Theme helpers, mirroring novapanel_theme.h's np_* functions --
void clear_style(lv_obj_t* obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

lv_obj_t* make_card(lv_obj_t* parent) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_set_style_bg_color(o, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o, kRCard, 0);
    lv_obj_set_style_pad_all(o, 16, 0);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    return o;
}

lv_obj_t* make_label(lv_obj_t* parent, const char* text, const lv_font_t* font, uint32_t color) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

lv_obj_t* make_row(lv_obj_t* parent) {
    lv_obj_t* o = lv_obj_create(parent);
    clear_style(o);
    lv_obj_set_size(o, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return o;
}

lv_obj_t* make_col(lv_obj_t* parent) {
    lv_obj_t* o = lv_obj_create(parent);
    clear_style(o);
    lv_obj_set_size(o, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return o;
}

lv_obj_t* make_hsep(lv_obj_t* parent) {
    lv_obj_t* s = lv_obj_create(parent);
    lv_obj_set_size(s, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(s, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_set_style_radius(s, 0, 0);
    return s;
}

lv_obj_t* make_chip(lv_obj_t* parent, const char* text) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_bg_color(btn, lv_color_hex(kColorAccentBg), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2C2700), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 14, 0);
    lv_obj_set_style_pad_hor(btn, 12, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t* lbl = make_label(btn, text, &nova_font_14, kColorAccent);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    return btn;
}
}  // namespace

void HomeScreen::build(lv_obj_t* parent) {
    // Mounts inside MainShell::content() - that's where the page bg/padding
    // already comes from (ADR-0024), so this wrapper stays transparent with
    // no padding of its own, just the row layout + gap between cards.
    lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(parent, 0, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(parent, kColGap, 0);

    // ====== LEFT: clock + weather ======
    lv_obj_t* left = make_card(parent);
    lv_obj_set_size(left, kLeftW, LV_PCT(100));
    lv_obj_set_style_pad_all(left, 20, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* clk_row = lv_obj_create(left);
    clear_style(clk_row);
    lv_obj_set_size(clk_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(clk_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(clk_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(clk_row, 4, 0);

    clock_label_ = make_label(clk_row, "00:00", &nova_font_bold_100, kColorText); 
    seconds_label_ = make_label(clk_row, "00", &nova_font_28, 0x3A4252);
    lv_obj_set_style_pad_bottom(seconds_label_, 8, 0);

    date_label_ = make_label(left, "", &nova_font_14, kColorTextDim);
    lv_obj_set_style_margin_top(date_label_, 8, 0);

    // spacer pushes the weather card to the bottom of the left column
    lv_obj_t* spacer = lv_obj_create(left);
    clear_style(spacer);
    lv_obj_set_size(spacer, 1, 1);
    lv_obj_set_style_flex_grow(spacer, 1, 0);

    lv_obj_t* hs = make_hsep(left);
    lv_obj_set_style_margin_bottom(hs, 14, 0);

    // -- weather card (all real data, OpenMeteoProvider: temp, condition,
    //    wind, humidity, high/low, "sensação" and UV all come from the
    //    current+daily forecast response - see open_meteo_provider.cpp) --
    lv_obj_t* wx = lv_obj_create(left);
    lv_obj_set_size(wx, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wx, lv_color_hex(kColorCard2), 0);
    lv_obj_set_style_bg_opa(wx, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wx, 0, 0);
    lv_obj_set_style_radius(wx, 10, 0);
    lv_obj_set_style_pad_all(wx, 14, 0);
    lv_obj_set_scrollbar_mode(wx, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wx, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* wr = make_row(wx);
    lv_obj_set_flex_align(wr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Left column: temperatura → mínima/máxima → status (condição)
    lv_obj_t* info_col = make_col(wr);
    lv_obj_set_size(info_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(info_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    weather_temp_label_ = make_label(info_col, "--", &nova_font_48, kColorText);

    weather_highlow_label_ = make_label(info_col, "", &nova_font_16, kColorTextMuted);
    lv_obj_set_style_margin_top(weather_highlow_label_, 2, 0);

    weather_condition_label_ = make_label(info_col, "sem dados ainda", &nova_font_14, kColorTextDim);
    lv_obj_set_style_margin_top(weather_condition_label_, 4, 0);

    // Right column: ícone 2x estático (nova_font_28; transform_scale não reflui
    // o layout — col de 70px garante espaço visual sem overlap com info_col)
    lv_obj_t* icon_col = make_col(wr);
    lv_obj_set_size(icon_col, 70, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(icon_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    weather_icon_label_ = lv_label_create(icon_col);
    lv_obj_set_style_text_font(weather_icon_label_, &nova_font_28, 0);
    lv_obj_set_style_text_color(weather_icon_label_, lv_color_hex(kColorAccent), 0);
    lv_label_set_text(weather_icon_label_, kWeatherIconUnknown);
    lv_obj_set_style_transform_pivot_x(weather_icon_label_, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(weather_icon_label_, LV_PCT(50), 0);
    lv_obj_set_style_transform_scale_x(weather_icon_label_, LV_SCALE_NONE * 2, 0);
    lv_obj_set_style_transform_scale_y(weather_icon_label_, LV_SCALE_NONE * 2, 0);
    lv_obj_align(weather_icon_label_, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* wsp = lv_obj_create(wx);
    lv_obj_set_size(wsp, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(wsp, lv_color_hex(kColorSep), 0);
    lv_obj_set_style_bg_opa(wsp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wsp, 0, 0);
    lv_obj_set_style_radius(wsp, 0, 0);
    lv_obj_set_style_margin_top(wsp, 11, 0);
    lv_obj_set_style_margin_bottom(wsp, 11, 0);

    lv_obj_t* wst = make_row(wx);
    lv_obj_set_flex_align(wst, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* wind_col = make_col(wst);
    lv_obj_set_size(wind_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(wind_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    make_label(wind_col, "Vento", &nova_font_14, kColorTextMuted);
    weather_wind_label_ = make_label(wind_col, "--", &nova_font_14, kColorText);
    lv_obj_set_style_margin_top(weather_wind_label_, 3, 0);

    lv_obj_t* humidity_col = make_col(wst);
    lv_obj_set_size(humidity_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(humidity_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    make_label(humidity_col, "Umidade", &nova_font_14, kColorTextMuted);
    weather_humidity_label_ = make_label(humidity_col, "--", &nova_font_14, kColorText);
    lv_obj_set_style_margin_top(weather_humidity_label_, 3, 0);

    lv_obj_t* feels_col = make_col(wst);
    lv_obj_set_size(feels_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(feels_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    make_label(feels_col, "Sensação", &nova_font_14, kColorTextMuted);
    weather_feels_label_ = make_label(feels_col, "--", &nova_font_14, kColorText);
    lv_obj_set_style_margin_top(weather_feels_label_, 3, 0);

    lv_obj_t* uv_col = make_col(wst);
    lv_obj_set_size(uv_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(uv_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    make_label(uv_col, "UV", &nova_font_14, kColorTextMuted);
    weather_uv_label_ = make_label(uv_col, "--", &nova_font_14, kColorAccent);
    lv_obj_set_style_margin_top(weather_uv_label_, 3, 0);

    status_label_ = make_label(left, "", &nova_font_14, kColorTextMuted);
    lv_obj_set_style_margin_top(status_label_, 10, 0);

    // ====== RIGHT: agenda + scenes + (player | market) ======
    lv_obj_t* right = lv_obj_create(parent);
    lv_obj_set_style_flex_grow(right, 1, 0);
    lv_obj_set_height(right, LV_PCT(100));
    clear_style(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(right, kColGap, 0);

    // -- Agenda mini card (PLACEHOLDER: no CalendarService yet - sample
    //    content straight from the design reference, see home_screen.hpp) --
    lv_obj_t* ag = make_card(right);
    lv_obj_set_size(ag, LV_PCT(100), 0);
    lv_obj_set_style_flex_grow(ag, 2, 0);  // bigger than the player/market row below (user decision: drop Cenas Rapidas, grow Agenda)
    lv_obj_set_style_pad_all(ag, 16, 0);
    lv_obj_set_flex_flow(ag, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ag, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* agh = make_row(ag);
    lv_obj_set_flex_align(agh, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(agh, 9, 0);
    lv_obj_set_style_margin_bottom(agh, 10, 0);
    make_label(agh, LV_SYMBOL_LIST, &nova_font_14, kColorAccent);
    lv_obj_t* ag_t = make_label(agh, "Agenda de hoje", &nova_font_16, kColorText);
    lv_obj_set_style_flex_grow(ag_t, 1, 0);
    make_label(agh, "4 eventos", &nova_font_14, kColorTextMuted);
    make_chip(agh, "Ver tudo >");

    static const struct { const char* t; const char* title; const char* sub; uint32_t c; } kEvents[] = {
        {"15:30", "Reunião - NoiseBot team", "Trabalho - 45 min", 0xE8A83C},
        {"17:00", "Academia",                "Pessoal - 1h",      0x4ABB78},
        {"19:30", "Jantar com Marina",        "Pessoal - 2h",      0x4F7ECB},
        {"21:00", "Revisar firmware v1.4",    "Projeto - 30 min",  0xB77ABB},
    };
    for (const auto& ev : kEvents) {
        lv_obj_t* row = make_row(ag);
        lv_obj_set_style_pad_column(row, 11, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(kColorSep), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 8, 0);

        lv_obj_t* tm = make_label(row, ev.t, &nova_font_16, kColorTextMed);
        lv_obj_set_width(tm, 38);

        lv_obj_t* bar = lv_obj_create(row);
        lv_obj_set_size(bar, 3, 28);
        lv_obj_set_style_bg_color(bar, lv_color_hex(ev.c), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);

        lv_obj_t* info = make_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        lv_obj_t* ttl = make_label(info, ev.title, &nova_font_14, kColorText);
        lv_label_set_long_mode(ttl, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(ttl, 260);
        lv_obj_t* sub = make_label(info, ev.sub, &nova_font_14, 0x5A6478);
        lv_obj_set_style_margin_top(sub, 0, 0);
    }

    // -- Bottom row: player (PLACEHOLDER: no audio feature) + market mini (real) --
    // Quick scenes card removed (user decision) - Agenda above got the
    // freed space via its bigger flex_grow.
    lv_obj_t* br = lv_obj_create(right);
    clear_style(br);
    lv_obj_set_size(br, LV_PCT(100), 0);
    lv_obj_set_style_flex_grow(br, 1, 0);
    lv_obj_set_flex_flow(br, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(br, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(br, kColGap, 0);

    lv_obj_t* pl = make_card(br);
    lv_obj_set_style_flex_grow(pl, 1, 0);
    lv_obj_set_height(pl, LV_PCT(100));
    lv_obj_set_style_pad_all(pl, 13, 0);
    lv_obj_set_flex_flow(pl, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pl, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* ptop = make_row(pl);
    lv_obj_set_style_pad_column(ptop, 10, 0);

    lv_obj_t* cov = lv_obj_create(ptop);
    lv_obj_set_size(cov, 34, 34);
    lv_obj_set_style_bg_color(cov, lv_color_hex(kColorAccent), 0);
    lv_obj_set_style_radius(cov, 8, 0);
    lv_obj_set_style_border_width(cov, 0, 0);
    lv_obj_set_style_pad_all(cov, 0, 0);
    lv_obj_set_scrollbar_mode(cov, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* cov_ic = make_label(cov, LV_SYMBOL_AUDIO, &nova_font_14, kColorDarkFg);
    lv_obj_align(cov_ic, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* si = make_col(ptop);
    lv_obj_set_style_flex_grow(si, 1, 0);
    lv_obj_set_size(si, 0, LV_SIZE_CONTENT);
    make_label(si, "Labirinto", &nova_font_14, kColorText);
    make_label(si, "Tuyo - Pra Cima de Mim", &nova_font_14, 0x5A6478);

    lv_obj_t* pb = lv_bar_create(pl);
    lv_obj_set_size(pb, LV_PCT(100), 2);
    lv_bar_set_range(pb, 0, 100);
    lv_bar_set_value(pb, 17, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(pb, lv_color_hex(kColorBorder), LV_PART_MAIN);
    lv_obj_set_style_bg_color(pb, lv_color_hex(kColorAccent), LV_PART_INDICATOR);
    lv_obj_set_style_radius(pb, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(pb, 1, LV_PART_INDICATOR);

    lv_obj_t* ctrl = make_row(pl);
    lv_obj_set_flex_align(ctrl, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(ctrl, 14, 0);
    make_label(ctrl, LV_SYMBOL_PREV, &nova_font_14, kColorTextMuted);

    lv_obj_t* play = lv_button_create(ctrl);
    lv_obj_set_size(play, 30, 30);
    lv_obj_set_style_bg_color(play, lv_color_hex(kColorAccent), 0);
    lv_obj_set_style_radius(play, 15, 0);
    lv_obj_set_style_shadow_width(play, 0, 0);
    lv_obj_t* play_ic = make_label(play, LV_SYMBOL_PAUSE, &nova_font_14, kColorDarkFg);
    lv_obj_align(play_ic, LV_ALIGN_CENTER, 0, 0);

    make_label(ctrl, LV_SYMBOL_NEXT, &nova_font_14, kColorTextMuted);

    // -- market mini (real data: USD/BRL + BTC from CoinGeckoProvider - no
    //    Ibovespa row, there's no stock-index provider) --
    lv_obj_t* mk = make_card(br);
    lv_obj_set_style_flex_grow(mk, 1, 0);
    lv_obj_set_height(mk, LV_PCT(100));
    lv_obj_set_style_pad_all(mk, 13, 0);
    lv_obj_set_flex_flow(mk, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mk, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* mh = make_row(mk);
    lv_obj_set_style_pad_column(mh, 6, 0);
    lv_obj_set_style_margin_bottom(mh, 9, 0);
    make_label(mh, LV_SYMBOL_CHARGE, &nova_font_14, kColorAccent);
    make_label(mh, "Mercado", &nova_font_14, kColorText);

    auto make_market_row = [&](const char* name, lv_obj_t** price_label, lv_obj_t** change_label) {
        lv_obj_t* row = make_row(mk);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_bottom(row, 6, 0);
        lv_obj_t* name_label = make_label(row, name, &nova_font_14, 0x5A6478);
        lv_obj_set_style_flex_grow(name_label, 1, 0);
        *price_label = make_label(row, "--", &nova_font_16, kColorText);
        lv_obj_set_style_margin_right(*price_label, 6, 0);
        *change_label = make_label(row, "", &nova_font_16, kColorTextDim);
    };
    make_market_row("Dólar", &market_usd_label_, &market_usd_change_label_);
    make_market_row("Bitcoin", &market_btc_label_, &market_btc_change_label_);
}

void HomeScreen::render(const AppState& s, lv_obj_t* content_parent) {
    if (!built_) {
        build(content_parent);
        built_ = true;
    }

    const auto& c = s.clock;
    const auto& m = s.market;
    const auto& w = s.weather;
    const auto& sys = s.system;

    char buf[64];

    std::snprintf(buf, sizeof(buf), "%02d:%02d", c.hour, c.minute);
    lv_label_set_text(clock_label_, buf);

    std::snprintf(buf, sizeof(buf), "%02d", c.second);
    lv_label_set_text(seconds_label_, buf);

    std::snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d%s",
                  weekday_name(c.weekday), c.day, c.month, c.year,
                  c.synced ? "" : " (não sincronizado)");
    lv_label_set_text(date_label_, buf);

    std::snprintf(buf, sizeof(buf), "board=%d display=%d touch=%d net=%d sd=%d cache=%d",
                  sys.board_ready, sys.display_ready, sys.touch_ready,
                  sys.network_ready, sys.sd_ready, sys.cache_ready);
    lv_label_set_text(status_label_, buf);

    // Dolar (ForexProvider) and BTC (CoinGeckoProvider) are fetched by
    // separate services now - each gets its own valid/source check instead
    // of sharing m.valid (one can be fresh while the other is still loading
    // or failing).
    if (m.usd_brl_valid) {
        const char* src = (m.usd_brl_source == DataSource::Cache) ? " (cache)"
                        : (m.usd_brl_source == DataSource::Mock)  ? " (mock)"
                        : "";
        std::snprintf(buf, sizeof(buf), "R$ %.2f%s", m.usd_brl, src);
        lv_label_set_text(market_usd_label_, buf);
        lv_label_set_text(market_usd_change_label_, "");
    } else {
        lv_label_set_text(market_usd_label_, "--");
        lv_label_set_text(market_usd_change_label_, "");
    }

    if (m.valid) {
        const char* btc_src = (m.source == DataSource::Cache) ? " (cache)"
                             : (m.source == DataSource::Mock)  ? " (mock)"
                             : "";
        char btc_price[24];
        format_thousands(static_cast<long>(m.btc_usd + 0.5), btc_price, sizeof(btc_price));
        std::snprintf(buf, sizeof(buf), "US$ %s%s", btc_price, btc_src);
        lv_label_set_text(market_btc_label_, buf);

        std::snprintf(buf, sizeof(buf), "%s %.1f%%",
                      m.btc_change_24h >= 0.0 ? LV_SYMBOL_UP : LV_SYMBOL_DOWN,
                      m.btc_change_24h);
        lv_label_set_text(market_btc_change_label_, buf);
        lv_obj_set_style_text_color(
            market_btc_change_label_,
            lv_color_hex(m.btc_change_24h >= 0.0 ? kColorGreen : kColorRed), 0);
    } else {
        lv_label_set_text(market_btc_label_, "--");
        lv_label_set_text(market_btc_change_label_, "sem dados ainda");
    }

    if (w.valid) {
        std::snprintf(buf, sizeof(buf), "%.0f°C", w.temperature_c);
        lv_label_set_text(weather_temp_label_, buf);

        lv_label_set_text(weather_icon_label_, weather_icon_symbol(w.condition));
        {
            const char* w_src = (w.source == DataSource::Cache) ? " (cache)"
                              : (w.source == DataSource::Mock)  ? " (mock)"
                              : "";
            std::snprintf(buf, sizeof(buf), "%s%s", condition_name(w.condition), w_src);
            lv_label_set_text(weather_condition_label_, buf);
        }

        std::snprintf(buf, sizeof(buf), "%.0f km/h", w.wind_speed_kmh);
        lv_label_set_text(weather_wind_label_, buf);

        std::snprintf(buf, sizeof(buf), "%d%%", w.humidity_pct);
        lv_label_set_text(weather_humidity_label_, buf);

        std::snprintf(buf, sizeof(buf), "%s %.0f°   %s %.0f°",
                      LV_SYMBOL_UP, w.temp_max_c, LV_SYMBOL_DOWN, w.temp_min_c);
        lv_label_set_text(weather_highlow_label_, buf);

        std::snprintf(buf, sizeof(buf), "%.0f°", w.feels_like_c);
        lv_label_set_text(weather_feels_label_, buf);

        std::snprintf(buf, sizeof(buf), "%.0f", w.uv_index);
        lv_label_set_text(weather_uv_label_, buf);
    } else {
        lv_label_set_text(weather_temp_label_, "--");
        lv_label_set_text(weather_condition_label_, "sem dados ainda");
        lv_label_set_text(weather_wind_label_, "--");
        lv_label_set_text(weather_humidity_label_, "--");
        lv_label_set_text(weather_highlow_label_, "");
        lv_label_set_text(weather_feels_label_, "--");
        lv_label_set_text(weather_uv_label_, "--");
    }
}

}  // namespace nova
