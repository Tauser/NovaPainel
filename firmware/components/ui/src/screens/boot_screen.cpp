// NovaPainel - ui/src/screens/boot_screen.cpp
// Real LVGL widgets (Fase 4/5). NOT host-checkable: lvgl.h has no host shim
// (see tools/scripts/host_check.sh, which skips this file on purpose).
// Faithful port of docs/design/lvgl_export_reference/screens/screen_boot.c.
#include "boot_screen.hpp"

#include "lvgl.h"
#include "nova_fonts.hpp"

namespace nova {

namespace {
constexpr uint32_t kColorBg        = 0x060810;  // matches screen_boot.c's own bg override
constexpr uint32_t kColorAccent    = 0xE8A83C;  // NP_C_ACCENT
constexpr uint32_t kColorText      = 0xE8EAF2;  // NP_C_TEXT
constexpr uint32_t kColorTextMuted = 0x464E64;  // NP_C_TEXT_MUTED
constexpr uint32_t kColorBorder    = 0x1E2235;  // NP_C_BORDER
constexpr uint32_t kColorDarkFg    = 0x090C12;  // NP_C_DARK_FG

const char* status_message_for(uint8_t pct) {
    if (pct < 30) return "Inicializando hardware...";
    if (pct < 60) return "Carregando sistema...";
    if (pct < 90) return "Verificando sensores...";
    return "Pronto!";
}
}  // namespace

void BootScreen::build() {
    // Own LVGL screen object (not lv_screen_active()) - see home_screen.cpp.
    root_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(root_, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* col = lv_obj_create(root_);
    lv_obj_set_size(col, 300, LV_SIZE_CONTENT);
    lv_obj_align(col, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_scrollbar_mode(col, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // logo badge
    lv_obj_t* badge = lv_obj_create(col);
    lv_obj_set_size(badge, 82, 82);
    lv_obj_set_style_bg_color(badge, lv_color_hex(kColorAccent), 0);
    lv_obj_set_style_radius(badge, 20, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_set_style_margin_bottom(badge, 22, 0);
    lv_obj_set_scrollbar_mode(badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* badge_l = lv_label_create(badge);
    lv_label_set_text(badge_l, "NP");
    lv_obj_set_style_text_font(badge_l, &nova_font_28, 0);
    lv_obj_set_style_text_color(badge_l, lv_color_hex(kColorDarkFg), 0);
    lv_obj_set_style_text_letter_space(badge_l, 2, 0);
    lv_obj_align(badge_l, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* nm = lv_label_create(col);
    lv_label_set_text(nm, "NovaPanel");
    lv_obj_set_style_text_font(nm, &nova_font_28, 0);
    lv_obj_set_style_text_color(nm, lv_color_hex(kColorText), 0);
    lv_obj_set_style_margin_bottom(nm, 6, 0);

    lv_obj_t* ver = lv_label_create(col);
    lv_label_set_text(ver, "Dashboard Smart");
    lv_obj_set_style_text_font(ver, &nova_font_14, 0);
    lv_obj_set_style_text_color(ver, lv_color_hex(kColorTextMuted), 0);
    lv_obj_set_style_text_letter_space(ver, 2, 0);
    lv_obj_set_style_margin_bottom(ver, 54, 0);

    lv_obj_t* track = lv_obj_create(col);
    lv_obj_set_size(track, 240, 3);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(track, 0, 0);
    lv_obj_set_style_radius(track, 2, 0);
    lv_obj_set_style_pad_all(track, 0, 0);
    lv_obj_set_style_margin_bottom(track, 16, 0);
    lv_obj_set_scrollbar_mode(track, LV_SCROLLBAR_MODE_OFF);

    progress_bar_ = lv_bar_create(track);
    lv_obj_set_size(progress_bar_, 240, 3);
    lv_obj_align(progress_bar_, LV_ALIGN_LEFT_MID, 0, 0);
    lv_bar_set_range(progress_bar_, 0, 100);
    lv_bar_set_value(progress_bar_, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(progress_bar_, lv_color_hex(0x1A1D2C), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar_, lv_color_hex(kColorAccent), LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar_, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar_, 2, LV_PART_INDICATOR);

    status_label_ = lv_label_create(col);
    lv_obj_set_style_text_font(status_label_, &nova_font_14, 0);
    lv_obj_set_style_text_color(status_label_, lv_color_hex(kColorTextMuted), 0);
    lv_obj_set_style_text_letter_space(status_label_, 1, 0);

    // skip button (bottom-right)
    lv_obj_t* skip = lv_button_create(root_);
    lv_obj_set_size(skip, LV_SIZE_CONTENT, 38);
    lv_obj_align(skip, LV_ALIGN_BOTTOM_RIGHT, -24, -24);
    lv_obj_set_style_bg_color(skip, lv_color_hex(0x0D0F18), 0);
    lv_obj_set_style_border_color(skip, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(skip, 1, 0);
    lv_obj_set_style_radius(skip, 9, 0);
    lv_obj_set_style_shadow_width(skip, 0, 0);
    lv_obj_set_style_pad_hor(skip, 16, 0);
    lv_obj_t* skip_l = lv_label_create(skip);
    lv_label_set_text(skip_l, "Pular ->");
    lv_obj_set_style_text_font(skip_l, &nova_font_14, 0);
    lv_obj_set_style_text_color(skip_l, lv_color_hex(kColorTextMuted), 0);
    lv_obj_align(skip_l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(skip, &BootScreen::on_skip_clicked, LV_EVENT_CLICKED, this);

    start_tick_ = lv_tick_get();
    built_ = true;
}

void BootScreen::render(const AppState& /*state*/) {
    if (!built_) {
        build();
    }
    if (lv_screen_active() != root_) {
        lv_screen_load(root_);
    }

    const uint32_t elapsed = lv_tick_elaps(start_tick_);
    const uint32_t pct = duration_ms_ > 0
        ? (elapsed >= duration_ms_ ? 100u : (elapsed * 100u) / duration_ms_)
        : 100u;

    lv_bar_set_value(progress_bar_, static_cast<int32_t>(pct), LV_ANIM_ON);
    lv_label_set_text(status_label_, status_message_for(static_cast<uint8_t>(pct)));
}

void BootScreen::on_skip_clicked(lv_event_t* e) {
    auto* self = static_cast<BootScreen*>(lv_event_get_user_data(e));
    if (self->on_skip_) self->on_skip_();
}

}  // namespace nova
