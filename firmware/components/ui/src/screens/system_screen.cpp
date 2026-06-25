// NovaPainel - ui/src/screens/system_screen.cpp
// Real LVGL widgets (Fase 7). NOT host-checkable: lvgl.h has no host shim
// (see tools/scripts/host_check.sh, which skips this file on purpose).
// No design reference exists for this screen - plain text MVP (user
// decision), see system_screen.hpp.
#include "system_screen.hpp"

#include <cstdio>

#include "lvgl.h"
#include "nova_fonts.hpp"

namespace nova {

namespace {
constexpr uint32_t kColorBg        = 0x0D0F18;  // NP_C_BG
constexpr uint32_t kColorCard      = 0x141721;  // NP_C_CARD
constexpr uint32_t kColorBorder    = 0x1E2235;  // NP_C_BORDER
constexpr uint32_t kColorText      = 0xE8EAF2;  // NP_C_TEXT
constexpr uint32_t kColorTextDim   = 0x7A8298;  // NP_C_TEXT_DIM
constexpr uint32_t kColorTextMuted = 0x464E64;  // NP_C_TEXT_MUTED

lv_obj_t* make_row_label(lv_obj_t* parent, const char* caption) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_ver(row, 6, 0);
    lv_obj_set_style_pad_hor(row, 0, 0);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* cap = lv_label_create(row);
    lv_label_set_text(cap, caption);
    lv_obj_set_style_text_font(cap, &nova_font_14, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(kColorTextDim), 0);

    lv_obj_t* val = lv_label_create(row);
    lv_label_set_text(val, "--");
    lv_obj_set_style_text_font(val, &nova_font_14, 0);
    lv_obj_set_style_text_color(val, lv_color_hex(kColorText), 0);
    return val;
}

void format_age(uint32_t now_ms, uint32_t last_update_ms, bool valid, char* out, size_t out_size) {
    if (!valid) {
        std::snprintf(out, out_size, "sem dados ainda");
        return;
    }
    const uint32_t age_s = (now_ms >= last_update_ms) ? (now_ms - last_update_ms) / 1000 : 0;
    std::snprintf(out, out_size, "há %lus", static_cast<unsigned long>(age_s));
}
}  // namespace

void SystemScreen::build() {
    root_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(root_, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 24, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* title = lv_label_create(root_);
    lv_label_set_text(title, "Sistema");
    lv_obj_set_style_text_font(title, &nova_font_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(kColorText), 0);
    lv_obj_set_style_margin_bottom(title, 16, 0);

    lv_obj_t* card = lv_obj_create(root_);
    lv_obj_set_size(card, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    reset_reason_label_ = make_row_label(card, "Motivo do último reset");
    reboot_count_label_ = make_row_label(card, "Reinicializações");
    hardware_label_     = make_row_label(card, "Hardware");
    weather_age_label_  = make_row_label(card, "Clima");
    btc_age_label_      = make_row_label(card, "Bitcoin");
    usd_brl_age_label_  = make_row_label(card, "Dólar");

    lv_obj_t* back = lv_button_create(root_);
    lv_obj_set_size(back, LV_SIZE_CONTENT, 38);
    lv_obj_set_style_margin_top(back, 16, 0);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x141721), 0);
    lv_obj_set_style_border_color(back, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(back, 1, 0);
    lv_obj_set_style_radius(back, 9, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    lv_obj_set_style_pad_hor(back, 16, 0);
    lv_obj_t* back_l = lv_label_create(back);
    lv_label_set_text(back_l, "<- Voltar");  // intentionally plain "<-" not LV_SYMBOL_LEFT, see wizard_screen.cpp's own "<- Voltar" precedent
    lv_obj_set_style_text_font(back_l, &nova_font_14, 0);
    lv_obj_set_style_text_color(back_l, lv_color_hex(kColorTextMuted), 0);
    lv_obj_align(back_l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(back, &SystemScreen::on_back_clicked, LV_EVENT_CLICKED, this);

    built_ = true;
}

void SystemScreen::render(const AppState& state, uint32_t now_ms) {
    if (!built_) {
        build();
    }
    if (lv_screen_active() != root_) {
        lv_screen_load(root_);
    }

    const auto& sys = state.system;
    const auto& w = state.weather;
    const auto& m = state.market;
    char buf[64];

    lv_label_set_text(reset_reason_label_, sys.reset_reason);

    std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(sys.reboot_count));
    lv_label_set_text(reboot_count_label_, buf);

    std::snprintf(buf, sizeof(buf), "board=%d display=%d touch=%d rede=%d sd=%d cache=%d",
                  sys.board_ready, sys.display_ready, sys.touch_ready,
                  sys.network_ready, sys.sd_ready, sys.cache_ready);
    lv_label_set_text(hardware_label_, buf);

    format_age(now_ms, w.last_update_ms, w.valid, buf, sizeof(buf));
    lv_label_set_text(weather_age_label_, buf);

    format_age(now_ms, m.last_update_ms, m.valid, buf, sizeof(buf));
    lv_label_set_text(btc_age_label_, buf);

    format_age(now_ms, m.usd_brl_last_update_ms, m.usd_brl_valid, buf, sizeof(buf));
    lv_label_set_text(usd_brl_age_label_, buf);
}

void SystemScreen::on_back_clicked(lv_event_t* e) {
    auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
    if (self->on_back_) self->on_back_();
}

}  // namespace nova
