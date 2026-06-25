// NovaPainel - ui/src/screens/main_shell.cpp
// Real LVGL widgets (Fase 4/5). NOT host-checkable: lvgl.h has no host shim
// (see tools/scripts/host_check.sh, which skips this file on purpose).
// Faithful port of docs/design/lvgl_export_reference/novapanel.c's
// np_build_rail()/np_build_topbar()/np_build_dots() + novapanel_theme.h.
#include "main_shell.hpp"

#include <cstring>

#include "lvgl.h"
#include "nova_fonts.hpp"

namespace nova {

namespace {
// -- Palette, from docs/design/lvgl_export_reference/novapanel_theme.h --
constexpr uint32_t kColorBg        = 0x0D0F18;  // NP_C_BG
constexpr uint32_t kColorRailBg    = 0x0B0D16;  // NP_C_RAIL_BG
constexpr uint32_t kColorBorder    = 0x1E2235;  // NP_C_BORDER
constexpr uint32_t kColorAccent    = 0xE8A83C;  // NP_C_ACCENT
constexpr uint32_t kColorAccentBg  = 0x1C1900;  // NP_C_ACCENT_BG
constexpr uint32_t kColorText      = 0xE8EAF2;  // NP_C_TEXT
constexpr uint32_t kColorTextMuted = 0x464E64;  // NP_C_TEXT_MUTED
constexpr uint32_t kColorGreen     = 0x4ABB78;  // NP_C_GREEN
constexpr uint32_t kColorGreenBd   = 0x1A3828;  // NP_C_GREEN_BD
constexpr uint32_t kColorDarkFg    = 0x090C12;  // NP_C_DARK_FG
constexpr uint32_t kColorDotInactive = 0x252A3C;
constexpr uint32_t kColorSep       = 0x181B28;

constexpr int kW         = 1024;  // NP_W
constexpr int kH         = 600;   // NP_H
constexpr int kRailW     = 62;    // NP_RAIL_W
constexpr int kTopbarH   = 60;    // NP_TOPBAR_H
// The rail is a closed-by-default overlay here (deviation from the
// reference, see main_shell.hpp) - topbar/content use the FULL width
// instead of leaving kRailW reserved for an always-visible rail.
constexpr int kContentW  = kW;
constexpr int kContentH  = kH - kTopbarH;
constexpr int kDotsH     = 16;    // NP_DOTS_H
constexpr int kPad       = 14;    // NP_PAD

constexpr int kNavCount = 8;
constexpr const char* kNavSymbols[kNavCount] = {
    LV_SYMBOL_HOME, LV_SYMBOL_LIST, LV_SYMBOL_CHARGE, LV_SYMBOL_IMAGE,
    LV_SYMBOL_BELL, LV_SYMBOL_TINT, LV_SYMBOL_REFRESH,
    LV_SYMBOL_WARNING,  // index 7 - System diagnostics (Fase 7), the only other navigable icon besides Inicio
};

lv_obj_t* make_icon_btn(lv_obj_t* parent, const char* symbol) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x141721), 0);  // NP_C_ITEM_BG
    lv_obj_set_style_border_color(btn, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 9, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t* ic = lv_label_create(btn);
    lv_label_set_text(ic, symbol);
    lv_obj_set_style_text_color(ic, lv_color_hex(kColorTextMuted), 0);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);
    return btn;
}
}  // namespace

void MainShell::build() {
    root_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(root_, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);

    // ====== topbar (60px) - x/width adapt when the rail opens/closes, see
    // open_rail()/close_rail() (push layout, not overlay). No menu icon
    // here on purpose - the edge tab (built at the end of this function)
    // is the only affordance to open/close the rail.
    topbar_ = lv_obj_create(root_);
    lv_obj_t* topbar = topbar_;
    lv_obj_set_size(topbar, kContentW, kTopbarH);
    lv_obj_set_pos(topbar, 0, 0);
    lv_obj_add_flag(topbar, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_color(topbar, lv_color_hex(kColorRailBg), 0);
    lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(topbar, 0, 0);
    lv_obj_set_style_radius(topbar, 0, 0);
    lv_obj_set_style_pad_hor(topbar, 18, 0);
    lv_obj_set_style_pad_ver(topbar, 0, 0);
    lv_obj_set_flex_flow(topbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(topbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(topbar, 10, 0);
    lv_obj_set_scrollbar_mode(topbar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* tb = lv_obj_create(topbar);
    lv_obj_set_height(tb, kTopbarH);
    lv_obj_set_style_bg_opa(tb, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tb, 0, 0);
    lv_obj_set_style_pad_all(tb, 0, 0);
    lv_obj_set_style_flex_grow(tb, 1, 0);
    lv_obj_set_flex_flow(tb, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tb, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(tb, LV_SCROLLBAR_MODE_OFF);

    // title = the user's display name (set by the onboarding wizard) instead
    // of a static "Inicio" - the date used to live in a label right below
    // this, but HomeScreen's own clock card already shows the full date, so
    // that second copy was just redundant (user decision, this session).
    title_label_ = lv_label_create(tb);
    lv_obj_set_style_text_font(title_label_, &nova_font_16, 0);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(kColorText), 0);

    // Wi-Fi status icon (real: reflects AppState::onboarding.wifi_status ==
    // Connected, the same STA+IP signal SetupService/MarketService/
    // WeatherService use - NOT SystemStatus::network_ready, which is only
    // the ESP-Hosted/SDIO transport set once at boot, not live Wi-Fi state).
    // Replaces the old "Online"/"Offline" text chip with a single icon, like
    // the bell/tint/settings buttons next to it.
    wifi_icon_btn_ = lv_obj_create(topbar);
    lv_obj_set_size(wifi_icon_btn_, 40, 40);
    lv_obj_set_style_border_width(wifi_icon_btn_, 1, 0);
    lv_obj_set_style_radius(wifi_icon_btn_, 9, 0);
    lv_obj_set_style_shadow_width(wifi_icon_btn_, 0, 0);
    lv_obj_set_scrollbar_mode(wifi_icon_btn_, LV_SCROLLBAR_MODE_OFF);
    wifi_icon_ = lv_label_create(wifi_icon_btn_);
    lv_label_set_text(wifi_icon_, LV_SYMBOL_WIFI);
    lv_obj_align(wifi_icon_, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* bell = make_icon_btn(topbar, LV_SYMBOL_BELL);  // inert - no Notificacoes screen yet
    (void)bell;

    lv_obj_t* vsep = lv_obj_create(topbar);
    lv_obj_set_size(vsep, 1, 18);
    lv_obj_set_style_bg_color(vsep, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_bg_opa(vsep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vsep, 0, 0);
    lv_obj_set_style_radius(vsep, 0, 0);

    make_icon_btn(topbar, LV_SYMBOL_TINT);  // inert - no dedicated Clima screen yet
    settings_btn_ = make_icon_btn(topbar, LV_SYMBOL_SETTINGS);
    lv_obj_add_event_cb(settings_btn_, &MainShell::on_settings_clicked, LV_EVENT_CLICKED, this);

    // ====== column: content + dots - x/width adapt with the rail, same as
    // topbar_ above (push layout). ======
    col_ = lv_obj_create(root_);
    lv_obj_t* col = col_;
    lv_obj_set_size(col, kContentW, kContentH);
    lv_obj_set_pos(col, 0, kTopbarH);
    lv_obj_add_flag(col, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_scrollbar_mode(col, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    content_ = lv_obj_create(col);
    lv_obj_set_size(content_, LV_PCT(100), kContentH - kDotsH);
    lv_obj_add_flag(content_, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_style_pad_all(content_, kPad, 0);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    // Not scrollable (not just scrollbar-hidden) - if HomeScreen's content
    // ever overflows this fixed box, it gets clipped instead of becoming
    // touch-draggable, which used to push the dot strip below the fold
    // (user had to scroll the page to see it - it's a fixed sibling, not
    // meant to move).
    lv_obj_clear_flag(content_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

    // dot strip - only the first (Inicio) dot is ever active, see header comment
    lv_obj_t* dots = lv_obj_create(col);
    lv_obj_set_size(dots, LV_PCT(100), kDotsH);
    lv_obj_set_style_bg_opa(dots, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots, 0, 0);
    lv_obj_set_style_pad_all(dots, 0, 0);
    lv_obj_set_style_radius(dots, 0, 0);
    lv_obj_set_scrollbar_mode(dots, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots, 5, 0);

    for (int i = 0; i < kNavCount; ++i) {
        const bool active = (i == 0);
        lv_obj_t* d = lv_obj_create(dots);
        lv_obj_set_size(d, active ? 18 : 5, 5);
        lv_obj_set_style_bg_color(d, lv_color_hex(active ? kColorAccent : kColorDotInactive), 0);
        lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(d, 0, 0);
        lv_obj_set_style_radius(d, 3, 0);
    }

    // ====== rail (overlay, closed by default - see header comment) ======
    // Built LAST so it draws on top of topbar/content while open (LVGL
    // stacks children in creation order).
    rail_ = lv_obj_create(root_);
    lv_obj_set_size(rail_, kRailW, kH);
    lv_obj_set_pos(rail_, 0, 0);
    lv_obj_set_style_bg_color(rail_, lv_color_hex(kColorRailBg), 0);
    lv_obj_set_style_bg_opa(rail_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(rail_, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(rail_, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(rail_, 1, 0);
    lv_obj_set_style_radius(rail_, 0, 0);
    lv_obj_set_style_pad_top(rail_, 12, 0);
    lv_obj_set_style_pad_bottom(rail_, 12, 0);
    lv_obj_set_style_pad_hor(rail_, 0, 0);
    lv_obj_set_flex_flow(rail_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(rail_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(rail_, 2, 0);
    lv_obj_set_scrollbar_mode(rail_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(rail_, LV_OBJ_FLAG_HIDDEN);  // closed by default

    lv_obj_t* logo = lv_obj_create(rail_);
    lv_obj_set_size(logo, 36, 36);
    lv_obj_set_style_bg_color(logo, lv_color_hex(kColorAccent), 0);
    lv_obj_set_style_radius(logo, 9, 0);
    lv_obj_set_style_border_width(logo, 0, 0);
    lv_obj_set_style_pad_all(logo, 0, 0);
    lv_obj_set_style_margin_bottom(logo, 6, 0);
    lv_obj_set_scrollbar_mode(logo, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* logo_l = lv_label_create(logo);
    lv_label_set_text(logo_l, "NP");
    lv_obj_set_style_text_font(logo_l, &nova_font_14, 0);
    lv_obj_set_style_text_color(logo_l, lv_color_hex(kColorDarkFg), 0);
    lv_obj_set_style_text_letter_space(logo_l, 1, 0);
    lv_obj_align(logo_l, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* sep = lv_obj_create(rail_);
    lv_obj_set_size(sep, 38, 1);
    lv_obj_set_style_bg_color(sep, lv_color_hex(kColorSep), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_radius(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    lv_obj_set_style_margin_bottom(sep, 6, 0);

    for (int i = 0; i < kNavCount; ++i) {
        lv_obj_t* btn = lv_button_create(rail_);
        lv_obj_set_size(btn, 44, 44);
        lv_obj_set_style_bg_color(btn, lv_color_hex(kColorRailBg), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(kColorAccentBg), LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, lv_color_hex(kColorBorder), LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 1, LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);

        lv_obj_t* ic = lv_label_create(btn);
        lv_label_set_text(ic, kNavSymbols[i]);
        lv_obj_set_style_text_font(ic, &nova_font_20, 0);
        lv_obj_set_style_text_color(ic, lv_color_hex(kColorTextMuted), 0);
        lv_obj_set_style_text_color(ic, lv_color_hex(kColorAccent), LV_STATE_CHECKED);
        lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);

        if (i == 0) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);  // Inicio - only real screen
        } else if (i == 7) {
            lv_obj_add_event_cb(btn, &MainShell::on_system_icon_clicked, LV_EVENT_CLICKED, this);  // Sistema (Fase 7)
        }
        // Icons 1-6 are intentionally inert (no event_cb) until their
        // screens exist (Agenda/Mercado dedicado/Casa/Alarmes/Clima
        // dedicado/Timer) - see header comment.
    }

    // ====== edge tab - the only affordance to open/close the rail. A
    // small persistent strip with a >/< arrow, always at the rail's
    // current edge, so the user can SEE that swiping is possible instead
    // of guessing. Built last (after the rail) so it draws on top of it.
    edge_tab_ = lv_obj_create(root_);
    lv_obj_set_size(edge_tab_, 20, 64);
    lv_obj_set_style_bg_color(edge_tab_, lv_color_hex(kColorRailBg), 0);
    lv_obj_set_style_bg_opa(edge_tab_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(edge_tab_, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(edge_tab_, 1, 0);
    lv_obj_set_style_radius(edge_tab_, 8, 0);
    lv_obj_set_style_pad_all(edge_tab_, 0, 0);
    lv_obj_set_scrollbar_mode(edge_tab_, LV_SCROLLBAR_MODE_OFF);
    edge_tab_icon_ = lv_label_create(edge_tab_);
    lv_label_set_text(edge_tab_icon_, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(edge_tab_icon_, &nova_font_14, 0);
    lv_obj_set_style_text_color(edge_tab_icon_, lv_color_hex(kColorTextMuted), 0);
    lv_obj_align(edge_tab_icon_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(edge_tab_, &MainShell::on_menu_toggle_clicked, LV_EVENT_CLICKED, this);

    // Swipe right-to-open / left-to-close, from anywhere (gestures bubble
    // up from topbar/col/content_ via LV_OBJ_FLAG_GESTURE_BUBBLE above).
    lv_obj_add_event_cb(root_, &MainShell::on_gesture, LV_EVENT_GESTURE, this);

    update_edge_tab();
    built_ = true;
}

void MainShell::open_rail() {
    rail_open_ = true;
    lv_obj_clear_flag(rail_, LV_OBJ_FLAG_HIDDEN);
    // Push layout (deviation from the reference's overlay-free permanent
    // rail, see header comment): topbar/content shrink and shift right
    // by kRailW instead of the rail covering them.
    lv_obj_set_pos(topbar_, kRailW, 0);
    lv_obj_set_width(topbar_, kContentW - kRailW);
    lv_obj_set_pos(col_, kRailW, kTopbarH);
    lv_obj_set_width(col_, kContentW - kRailW);
    update_edge_tab();
}

void MainShell::close_rail() {
    rail_open_ = false;
    lv_obj_add_flag(rail_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(topbar_, 0, 0);
    lv_obj_set_width(topbar_, kContentW);
    lv_obj_set_pos(col_, 0, kTopbarH);
    lv_obj_set_width(col_, kContentW);
    update_edge_tab();
}

void MainShell::update_edge_tab() {
    const int x = rail_open_ ? kRailW : 0;
    lv_obj_set_pos(edge_tab_, x, (kH - 64) / 2);
    lv_label_set_text(edge_tab_icon_, rail_open_ ? LV_SYMBOL_LEFT : LV_SYMBOL_RIGHT);
}

void MainShell::on_menu_toggle_clicked(lv_event_t* e) {
    auto* self = static_cast<MainShell*>(lv_event_get_user_data(e));
    self->rail_open_ ? self->close_rail() : self->open_rail();
}

void MainShell::on_system_icon_clicked(lv_event_t* e) {
    auto* self = static_cast<MainShell*>(lv_event_get_user_data(e));
    if (self->on_navigate_) self->on_navigate_(ScreenId::System);
}

void MainShell::on_settings_clicked(lv_event_t* e) {
    auto* self = static_cast<MainShell*>(lv_event_get_user_data(e));
    if (self->on_navigate_) self->on_navigate_(ScreenId::Settings);
}

void MainShell::on_gesture(lv_event_t* e) {
    auto* self = static_cast<MainShell*>(lv_event_get_user_data(e));
    lv_indev_t* indev = lv_indev_active();
    if (!indev) return;
    const lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_RIGHT) self->open_rail();
    else if (dir == LV_DIR_LEFT) self->close_rail();
}

void MainShell::render(const AppState& state) {
    if (!built_) {
        build();
    }
    if (lv_screen_active() != root_) {
        lv_screen_load(root_);
    }

    const char* title = state.preferences.display_name.empty()
                        ? "NovaPainel" : state.preferences.display_name.c_str();
    if (std::strcmp(lv_label_get_text(title_label_), title) != 0)
        lv_label_set_text(title_label_, title);

    const bool online = state.onboarding.wifi_status == WifiConnectStatus::Connected;
    if (online != last_online_) {
        lv_obj_set_style_bg_color(wifi_icon_btn_, lv_color_hex(online ? 0x0E1F14 : 0x141721), 0);
        lv_obj_set_style_border_color(wifi_icon_btn_, lv_color_hex(online ? kColorGreenBd : kColorBorder), 0);
        lv_obj_set_style_text_color(wifi_icon_, lv_color_hex(online ? kColorGreen : kColorTextMuted), 0);
        last_online_ = online;
    }
}

}  // namespace nova
