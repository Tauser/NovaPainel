// NovaPainel - ui/src/screens/home_screen.cpp
// Real LVGL widgets (Fase 4). NOT host-checkable: lvgl.h has no host shim
// (see tools/scripts/host_check.sh, which skips this file on purpose).
// Colors/fonts/layout language follow docs/design/lvgl_export_reference/
// (novapanel_theme.h), scoped down to fields AppState actually has.
#include "home_screen.hpp"

#include <cstdio>

#include "lvgl.h"

namespace nova {

namespace {
// -- Palette, lifted from docs/design/lvgl_export_reference/novapanel_theme.h
constexpr uint32_t kColorBg        = 0x0D0F18;
constexpr uint32_t kColorCard      = 0x141721;
constexpr uint32_t kColorBorder    = 0x1E2235;
constexpr uint32_t kColorText      = 0xE8EAF2;
constexpr uint32_t kColorTextDim   = 0x7A8298;
constexpr uint32_t kColorTextMuted = 0x464E64;
constexpr uint32_t kColorAccent    = 0xE8A83C;
constexpr uint32_t kColorGreen     = 0x4ABB78;
constexpr uint32_t kColorRed       = 0xD05252;

const char* weekday_name(int wd) {
    static const char* kDays[] = {"Domingo", "Segunda", "Terca", "Quarta",
                                  "Quinta", "Sexta", "Sabado"};
    if (wd < 0 || wd > 6) return "?";
    return kDays[wd];
}

lv_obj_t* make_card(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_style_bg_color(card, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    return card;
}

lv_obj_t* make_label(lv_obj_t* parent, const lv_font_t* font, uint32_t color) {
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}
}  // namespace

void HomeScreen::build() {
    lv_obj_t* root = lv_screen_active();
    lv_obj_set_style_bg_color(root, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 16, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(root, 16, 0);

    // -- Left card: clock --
    lv_obj_t* clock_card = make_card(root);
    lv_obj_set_size(clock_card, 360, LV_PCT(100));
    lv_obj_set_flex_flow(clock_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(clock_card, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* clock_row = lv_obj_create(clock_card);
    lv_obj_set_style_bg_opa(clock_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(clock_row, 0, 0);
    lv_obj_set_style_pad_all(clock_row, 0, 0);
    lv_obj_set_size(clock_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(clock_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(clock_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(clock_row, 4, 0);

    clock_label_ = make_label(clock_row, &lv_font_montserrat_48, kColorText);
    seconds_label_ = make_label(clock_row, &lv_font_montserrat_28, 0x3A4252);
    lv_obj_set_style_pad_bottom(seconds_label_, 8, 0);

    date_label_ = make_label(clock_card, &lv_font_montserrat_12, kColorTextDim);
    lv_obj_set_style_margin_top(date_label_, 8, 0);

    status_label_ = make_label(clock_card, &lv_font_montserrat_10, kColorTextMuted);
    lv_obj_set_style_margin_top(status_label_, 16, 0);

    // -- Right card: market --
    lv_obj_t* market_card = make_card(root);
    lv_obj_set_style_flex_grow(market_card, 1, 0);
    lv_obj_set_height(market_card, LV_PCT(100));
    lv_obj_set_flex_flow(market_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(market_card, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* market_title = make_label(market_card, &lv_font_montserrat_14, kColorAccent);
    lv_label_set_text(market_title, "Mercado");

    lv_obj_t* btc_row = lv_obj_create(market_card);
    lv_obj_set_style_bg_opa(btc_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btc_row, 0, 0);
    lv_obj_set_style_pad_all(btc_row, 0, 0);
    lv_obj_set_size(btc_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(btc_row, 12, 0);
    lv_obj_set_flex_flow(btc_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btc_row, 8, 0);

    btc_label_ = make_label(btc_row, &lv_font_montserrat_24, kColorText);
    btc_change_label_ = make_label(btc_row, &lv_font_montserrat_14, kColorGreen);
    lv_obj_set_style_pad_bottom(btc_change_label_, 4, 0);

    usd_brl_label_ = make_label(market_card, &lv_font_montserrat_14, kColorTextDim);
    lv_obj_set_style_margin_top(usd_brl_label_, 6, 0);

    built_ = true;
}

void HomeScreen::render(const AppState& s) {
    if (!built_) {
        build();
    }

    const auto& c = s.clock;
    const auto& m = s.market;
    const auto& sys = s.system;

    char buf[64];

    std::snprintf(buf, sizeof(buf), "%02d:%02d", c.hour, c.minute);
    lv_label_set_text(clock_label_, buf);

    std::snprintf(buf, sizeof(buf), "%02d", c.second);
    lv_label_set_text(seconds_label_, buf);

    std::snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d%s",
                  weekday_name(c.weekday), c.day, c.month, c.year,
                  c.synced ? "" : " (nao sincronizado)");
    lv_label_set_text(date_label_, buf);

    std::snprintf(buf, sizeof(buf), "board=%d display=%d touch=%d net=%d sd=%d cache=%d",
                  sys.board_ready, sys.display_ready, sys.touch_ready,
                  sys.network_ready, sys.sd_ready, sys.cache_ready);
    lv_label_set_text(status_label_, buf);

    if (m.valid) {
        const char* src = (m.source == DataSource::Cache) ? " (cache)"
                        : (m.source == DataSource::Mock)  ? " (mock)"
                        : "";
        std::snprintf(buf, sizeof(buf), "BTC $%.0f%s", m.btc_usd, src);
        lv_label_set_text(btc_label_, buf);

        std::snprintf(buf, sizeof(buf), "%+.1f%%", m.btc_change_24h);
        lv_label_set_text(btc_change_label_, buf);
        lv_obj_set_style_text_color(
            btc_change_label_,
            lv_color_hex(m.btc_change_24h >= 0.0 ? kColorGreen : kColorRed), 0);

        std::snprintf(buf, sizeof(buf), "USD/BRL R$ %.2f", m.usd_brl);
        lv_label_set_text(usd_brl_label_, buf);
    } else {
        lv_label_set_text(btc_label_, "--");
        lv_label_set_text(btc_change_label_, "");
        lv_label_set_text(usd_brl_label_, "sem dados ainda");
    }
}

}  // namespace nova
