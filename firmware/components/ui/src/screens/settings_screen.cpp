// NovaPainel - ui/src/screens/settings_screen.cpp
// Faithful port of docs/design/lvgl_export_reference/screens/screen_config.c.
// NOT host-checkable: lvgl.h has no host shim (see host_check.sh skip list).
// ADR-0017: on_submit_ enqueues work to app_main's action_queue — safe from
// LVGL event callbacks.
#include "settings_screen.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>

#include "lvgl.h"
#include "nova_fonts.hpp"

namespace nova {

namespace {
// ── Palette (novapanel_theme.h) ──────────────────────────────────────────
constexpr uint32_t kBg         = 0x0D0F18;
constexpr uint32_t kCard       = 0x141721;
constexpr uint32_t kCard2      = 0x1B1E2D;
constexpr uint32_t kBorder     = 0x1E2235;
constexpr uint32_t kSep        = 0x1A1D2C;
constexpr uint32_t kAccent     = 0xE8A83C;
constexpr uint32_t kAccentBg   = 0x1C1900;
constexpr uint32_t kAccentBd   = 0x2C2700;
constexpr uint32_t kText       = 0xE8EAF2;
constexpr uint32_t kTextDim    = 0x7A8298;
constexpr uint32_t kTextMuted  = 0x464E64;
constexpr uint32_t kSubtext    = 0x5A6478;
constexpr uint32_t kGreen      = 0x4ABB78;
constexpr uint32_t kGreenBg    = 0x0F1D15;
constexpr uint32_t kGreenBd    = 0x1A3828;
constexpr uint32_t kDarkFg     = 0x090C12;
constexpr uint32_t kSelected   = 0x252A3C;

constexpr int kRCard = 12;
constexpr int kRBtn  = 9;

struct TzEntry { const char* id; const char* display; const char* offset; };
constexpr TzEntry kTzList[SettingsScreen::kTzCount] = {
    {"America/Sao_Paulo",  "America/S\xC3\xA3o Paulo", "UTC-3 \xC2\xB7 NTP autom\xC3\xA1tico"},
    {"America/Manaus",     "America/Manaus",            "UTC-4 \xC2\xB7 NTP autom\xC3\xA1tico"},
    {"America/Rio_Branco", "America/Rio Branco",        "UTC-5 \xC2\xB7 NTP autom\xC3\xA1tico"},
    {"America/Noronha",    "America/Noronha",           "UTC-2 \xC2\xB7 NTP autom\xC3\xA1tico"},
    {"UTC",                "UTC / GMT",                  "UTC+0 \xC2\xB7 NTP autom\xC3\xA1tico"},
};

constexpr const char* kSysKeys[SettingsScreen::kSysRows] = {
    "Dispositivo", "Firmware", "Chip",
    "RAM livre", "Flash livre", "\xC3\xBaltima reinic.", "Reboots",
};
constexpr uint32_t kSysColors[SettingsScreen::kSysRows] = {
    kText, kText, kText,
    kGreen, kGreen, kText, kAccent,
};

// ── Widget helpers ────────────────────────────────────────────────────────
static lv_obj_t* mk_lbl(lv_obj_t* p, const char* t,
                         const lv_font_t* f, uint32_t c)
{
    lv_obj_t* l = lv_label_create(p);
    lv_label_set_text(l, t);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(c), 0);
    return l;
}

static void clear_style(lv_obj_t* o) {
    lv_obj_set_style_bg_opa(o,       LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o,      0, 0);
    lv_obj_set_style_radius(o,       0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_scrollbar_mode(o,     LV_SCROLLBAR_MODE_OFF);
}

static lv_obj_t* mk_row(lv_obj_t* p) {
    lv_obj_t* r = lv_obj_create(p);
    lv_obj_set_size(r, LV_PCT(100), LV_SIZE_CONTENT);
    clear_style(r);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return r;
}

static lv_obj_t* mk_col(lv_obj_t* p) {
    lv_obj_t* c = lv_obj_create(p);
    lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    clear_style(c);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    return c;
}

static lv_obj_t* mk_card(lv_obj_t* p) {
    lv_obj_t* c = lv_obj_create(p);
    lv_obj_set_style_bg_color(c,     lv_color_hex(kCard), 0);
    lv_obj_set_style_bg_opa(c,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(kBorder), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c,       kRCard, 0);
    lv_obj_set_style_shadow_width(c, 0, 0);
    lv_obj_set_style_pad_all(c,      0, 0);
    lv_obj_set_scrollbar_mode(c,     LV_SCROLLBAR_MODE_OFF);
    return c;
}

static void mk_hsep(lv_obj_t* p) {
    lv_obj_t* s = lv_obj_create(p);
    lv_obj_set_size(s, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(s,     lv_color_hex(kSep), 0);
    lv_obj_set_style_bg_opa(s,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s,      0, 0);
    lv_obj_set_style_radius(s,       0, 0);
    lv_obj_set_style_shadow_width(s, 0, 0);
}

// One of the 3 main column cards
static lv_obj_t* mk_col_card(lv_obj_t* grid) {
    lv_obj_t* c = mk_card(grid);
    lv_obj_set_style_flex_grow(c, 1, 0);
    lv_obj_set_height(c, LV_PCT(100));
    lv_obj_set_style_pad_all(c, 16, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_AUTO);
    return c;
}

// "⚡ Rede" style header for a column
static void mk_col_hdr(lv_obj_t* col, const char* icon, const char* title) {
    lv_obj_t* h = mk_row(col);
    lv_obj_set_style_pad_column(h, 7, 0);
    lv_obj_set_style_margin_bottom(h, 14, 0);
    mk_lbl(h, icon,  &nova_font_20, kAccent);
    mk_lbl(h, title, &nova_font_14, kAccent);
}

// "WI-FI" / "BLUETOOTH" section labels
static void mk_sec_lbl(lv_obj_t* col, const char* text) {
    mk_lbl(col, text, &nova_font_14, kTextMuted);
}

// Inner card (Wi-Fi, timezone)
static lv_obj_t* mk_inner_card(lv_obj_t* col) {
    lv_obj_t* c = lv_obj_create(col);
    lv_obj_set_size(c, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(c,     lv_color_hex(kCard2), 0);
    lv_obj_set_style_bg_opa(c,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c,       9, 0);
    lv_obj_set_style_pad_all(c,      11, 0);
    lv_obj_set_style_margin_top(c,   7, 0);
    lv_obj_set_style_shadow_width(c, 0, 0);
    lv_obj_set_scrollbar_mode(c,     LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    return c;
}

// BT / night-mode toggle row: [title col + switch]
static lv_obj_t* mk_toggle_row(lv_obj_t* col,
                                const char* title, const char* subtitle,
                                bool checked,
                                lv_event_cb_t cb, void* ud)
{
    lv_obj_t* row = mk_row(col);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* info = mk_col(row);
    mk_lbl(info, title,    &nova_font_14, kText);
    mk_lbl(info, subtitle, &nova_font_14, kSubtext);

    lv_obj_t* sw = lv_switch_create(row);
    lv_obj_set_size(sw, 44, 24);
    lv_obj_set_style_bg_color(sw, lv_color_hex(kBorder), 0);
    lv_obj_set_style_bg_color(sw, lv_color_hex(kAccent), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(kText),   LV_PART_KNOB);
    lv_obj_set_style_radius(sw,   12, 0);
    lv_obj_set_style_radius(sw,   10, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sw,  2,  LV_PART_KNOB);
    lv_obj_set_style_shadow_width(sw, 0, 0);
    if (checked) lv_obj_add_state(sw, LV_STATE_CHECKED);
    if (cb) lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, ud);
    return sw;
}

// Slider row: title + accent value on one row, then the slider
static lv_obj_t* mk_slider_row(lv_obj_t* col, const char* title,
                                int32_t val, lv_obj_t** out_lbl,
                                lv_event_cb_t cb, void* ud)
{
    lv_obj_t* row = mk_row(col);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(row, 4, 0);
    mk_lbl(row, title, &nova_font_14, kTextDim);
    char buf[8]; snprintf(buf, sizeof(buf), "%d%%", (int)val);
    lv_obj_t* vl = mk_lbl(row, buf, &nova_font_20, kAccent);
    if (out_lbl) *out_lbl = vl;

    lv_obj_t* sl = lv_slider_create(col);
    lv_obj_set_size(sl, LV_PCT(100), 6);
    lv_obj_set_style_margin_bottom(sl, 14, 0);
    lv_slider_set_range(sl, 0, 100);
    lv_slider_set_value(sl, val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, lv_color_hex(kBorder), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, lv_color_hex(kAccent), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, lv_color_hex(kAccent), LV_PART_KNOB);
    lv_obj_set_style_radius(sl,   3,  LV_PART_MAIN);
    lv_obj_set_style_radius(sl,   3,  LV_PART_INDICATOR);
    lv_obj_set_style_radius(sl,   6,  LV_PART_KNOB);
    lv_obj_set_style_pad_all(sl,  4,  LV_PART_KNOB);
    lv_obj_set_style_shadow_width(sl, 0, LV_PART_KNOB);
    if (cb) lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, ud);
    return sl;
}

// "Editar" chip in the profile bar
static lv_obj_t* mk_chip(lv_obj_t* p, const char* text) {
    lv_obj_t* c = lv_obj_create(p);
    lv_obj_set_size(c, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_bg_color(c,     lv_color_hex(kAccentBg), 0);
    lv_obj_set_style_bg_opa(c,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(kAccentBd), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c,       14, 0);
    lv_obj_set_style_pad_hor(c,      10, 0);
    lv_obj_set_style_pad_ver(c,      0, 0);
    lv_obj_set_style_shadow_width(c, 0, 0);
    lv_obj_set_scrollbar_mode(c,     LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* l = mk_lbl(c, text, &nova_font_14, kAccent);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 0);
    return c;
}

}  // namespace

// ── Static event callbacks ────────────────────────────────────────────────
void SettingsScreen::on_close_clicked(lv_event_t* e) {
    static_cast<SettingsScreen*>(lv_event_get_user_data(e))
        ->on_navigate_(ScreenId::Home);
}
void SettingsScreen::on_wifi_reconfigure_clicked(lv_event_t* e) {
    static_cast<SettingsScreen*>(lv_event_get_user_data(e))
        ->on_navigate_(ScreenId::Setup);
}
void SettingsScreen::on_tz_card_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->selected_tz_ = (self->selected_tz_ + 1) % kTzCount;
    self->update_tz_card();
    self->submit_tz_format();
}
void SettingsScreen::on_fmt24_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    if (!self->selected_24h_) {
        self->selected_24h_ = true;
        self->update_fmt_pills();
        self->submit_tz_format();
    }
}
void SettingsScreen::on_fmt12_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    if (self->selected_24h_) {
        self->selected_24h_ = false;
        self->update_fmt_pills();
        self->submit_tz_format();
    }
}
void SettingsScreen::on_night_toggle_changed(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->night_mode_ = lv_obj_has_state(static_cast<lv_obj_t*>(lv_event_get_target(e)), LV_STATE_CHECKED);
}

// Slider callbacks each update their own value label
void SettingsScreen::on_brilho_changed(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->brilho_ = (int)lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(e)));
    char buf[8]; snprintf(buf, sizeof(buf), "%d%%", self->brilho_);
    lv_label_set_text(self->brilho_val_lbl_, buf);
}
void SettingsScreen::on_vsys_changed(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->vol_sys_ = (int)lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(e)));
    char buf[8]; snprintf(buf, sizeof(buf), "%d%%", self->vol_sys_);
    lv_label_set_text(self->vsys_val_lbl_, buf);
}
void SettingsScreen::on_vmus_changed(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->vol_mus_ = (int)lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(e)));
    char buf[8]; snprintf(buf, sizeof(buf), "%d%%", self->vol_mus_);
    lv_label_set_text(self->vmus_val_lbl_, buf);
}
void SettingsScreen::on_valm_changed(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->vol_alm_ = (int)lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(e)));
    char buf[8]; snprintf(buf, sizeof(buf), "%d%%", self->vol_alm_);
    lv_label_set_text(self->valm_val_lbl_, buf);
}
void SettingsScreen::on_restart_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    if (self->on_restart_) self->on_restart_();
}

// ── Internal state helpers ────────────────────────────────────────────────
void SettingsScreen::update_tz_card() {
    lv_label_set_text(tz_name_lbl_, kTzList[selected_tz_].display);
    lv_label_set_text(tz_info_lbl_, kTzList[selected_tz_].offset);
}

void SettingsScreen::update_fmt_pills() {
    for (int i = 0; i < 2; i++) {
        bool sel = (i == 0) == selected_24h_;
        lv_obj_set_style_bg_color(fmt_pills_[i],
            lv_color_hex(sel ? kSelected : kCard2), 0);
        lv_obj_t* lbl = lv_obj_get_child(fmt_pills_[i], 0);
        if (lbl)
            lv_obj_set_style_text_color(lbl,
                lv_color_hex(sel ? kText : kTextMuted), 0);
    }
}

void SettingsScreen::submit_tz_format() {
    OnboardingSubmission sub{};
    sub.step            = OnboardingStep::TimezoneAndFormat;
    sub.timezone        = kTzList[selected_tz_].id;
    sub.time_format_24h = selected_24h_;
    on_submit_(sub);
}

// ── build() ───────────────────────────────────────────────────────────────
void SettingsScreen::build() {
    root_ = lv_obj_create(nullptr);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root_, lv_color_hex(kBg), 0);
    lv_obj_set_style_bg_opa(root_,   LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_hor(root_,  16, 0);
    lv_obj_set_style_pad_top(root_,  12, 0);
    lv_obj_set_style_pad_bottom(root_, 12, 0);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root_, 11, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);

    // ── Top bar: title + close button ─────────────────────────────────────
    lv_obj_t* topbar = mk_row(root_);
    lv_obj_set_flex_align(topbar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(topbar, 2, 0);
    mk_lbl(topbar, "Configura\xC3\xA7\xC3\xB5""es", &nova_font_20, kText);

    lv_obj_t* close_btn = lv_obj_create(topbar);
    lv_obj_set_size(close_btn, 36, 36);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(kCard2), 0);
    lv_obj_set_style_bg_opa(close_btn,   LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_set_style_radius(close_btn,   18, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_pad_all(close_btn, 0, 0);
    lv_obj_set_scrollbar_mode(close_btn, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(close_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t* x_lbl = mk_lbl(close_btn, LV_SYMBOL_CLOSE, &nova_font_14, kTextDim);
    lv_obj_align(x_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(close_btn, &SettingsScreen::on_close_clicked,
                        LV_EVENT_CLICKED, this);

    // ── Profile bar ───────────────────────────────────────────────────────
    lv_obj_t* prof = mk_card(root_);
    lv_obj_set_size(prof, LV_PCT(100), 68);
    lv_obj_set_style_pad_hor(prof, 18, 0);
    lv_obj_set_style_pad_ver(prof, 0, 0);
    lv_obj_set_flex_flow(prof, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(prof, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(prof, 14, 0);

    lv_obj_t* av = lv_obj_create(prof);
    lv_obj_set_size(av, 42, 42);
    lv_obj_set_style_bg_color(av,     lv_color_hex(kAccent), 0);
    lv_obj_set_style_bg_opa(av,       LV_OPA_COVER, 0);
    lv_obj_set_style_radius(av,       21, 0);
    lv_obj_set_style_border_width(av, 0, 0);
    lv_obj_set_style_pad_all(av,      0, 0);
    lv_obj_set_style_shadow_width(av, 0, 0);
    lv_obj_set_scrollbar_mode(av,     LV_SCROLLBAR_MODE_OFF);
    av_initials_lbl_ = mk_lbl(av, "RL", &nova_font_20, kDarkFg);
    lv_obj_align(av_initials_lbl_, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* pi = mk_col(prof);
    lv_obj_set_style_flex_grow(pi, 1, 0);
    lv_obj_set_size(pi, 0, LV_SIZE_CONTENT);
    profile_name_lbl_ = mk_lbl(pi, "\xe2\x80\x94", &nova_font_20, kText);
    mk_lbl(pi, "Perfil padr\xC3\xA3o \xC2\xB7 NovaPanel ESP32-P4",
           &nova_font_14, kSubtext);

    mk_chip(prof, "Editar");

    // ── 3-column grid ─────────────────────────────────────────────────────
    lv_obj_t* grid = lv_obj_create(root_);
    lv_obj_set_size(grid, LV_PCT(100), 0);
    lv_obj_set_style_flex_grow(grid, 1, 0);
    clear_style(grid);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(grid, 11, 0);

    // ─────────────────────────────────────────────────────────────────────
    // COL 1: Rede
    // ─────────────────────────────────────────────────────────────────────
    lv_obj_t* net = mk_col_card(grid);
    mk_col_hdr(net, LV_SYMBOL_WIFI, "Rede");

    // Wi-Fi section
    mk_sec_lbl(net, "WI-FI");

    lv_obj_t* wc = mk_inner_card(net);
    wifi_ssid_lbl_    = mk_lbl(wc, "\xe2\x80\x94", &nova_font_14, kText);
    wifi_details_lbl_ = mk_lbl(wc, "Verificando...", &nova_font_14, kSubtext);

    lv_obj_t* wm = mk_lbl(net, "Gerenciar redes Wi-Fi " LV_SYMBOL_RIGHT,
                           &nova_font_14, kAccent);
    lv_obj_set_style_margin_top(wm, 7, 0);
    lv_obj_set_style_margin_bottom(wm, 14, 0);
    lv_obj_add_flag(wm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(wm, &SettingsScreen::on_wifi_reconfigure_clicked,
                        LV_EVENT_CLICKED, this);

    mk_hsep(net);

    // Bluetooth section
    lv_obj_t* bth = mk_row(net);
    lv_obj_set_style_margin_top(bth,    14, 0);
    lv_obj_set_style_margin_bottom(bth, 4, 0);
    mk_lbl(bth, "BLUETOOTH", &nova_font_14, kTextMuted);

    mk_toggle_row(net, "Bluetooth", "Desativado", false, nullptr, nullptr);
    lv_obj_set_style_margin_bottom(lv_obj_get_child(net, -1), 14, 0);

    mk_hsep(net);

    // Timezone section
    lv_obj_t* tzh = mk_row(net);
    lv_obj_set_style_margin_top(tzh,    14, 0);
    lv_obj_set_style_margin_bottom(tzh, 8, 0);
    mk_lbl(tzh, "FUSO HOR\xC3\x81RIO", &nova_font_14, kTextMuted);

    lv_obj_t* tz = lv_obj_create(net);
    lv_obj_set_size(tz, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tz,     lv_color_hex(kCard2), 0);
    lv_obj_set_style_bg_opa(tz,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tz, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_border_width(tz, 1, 0);
    lv_obj_set_style_radius(tz,       9, 0);
    lv_obj_set_style_pad_all(tz,      10, 0);
    lv_obj_set_style_shadow_width(tz, 0, 0);
    lv_obj_set_flex_flow(tz, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tz, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(tz, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(tz, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tz, &SettingsScreen::on_tz_card_clicked,
                        LV_EVENT_CLICKED, this);

    lv_obj_t* tz_info = mk_col(tz);
    lv_obj_set_size(tz_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    tz_name_lbl_ = mk_lbl(tz_info, kTzList[0].display, &nova_font_14, kText);
    tz_info_lbl_ = mk_lbl(tz_info, kTzList[0].offset,  &nova_font_14, kSubtext);
    mk_lbl(tz, LV_SYMBOL_RIGHT, &nova_font_14, kTextMuted);

    // ─────────────────────────────────────────────────────────────────────
    // COL 2: Display & Som
    // ─────────────────────────────────────────────────────────────────────
    lv_obj_t* disp = mk_col_card(grid);
    mk_col_hdr(disp, LV_SYMBOL_TINT, "Display & Som");

    mk_sec_lbl(disp, "BRILHO");
    mk_slider_row(disp, "Brilho da tela", brilho_,
                  &brilho_val_lbl_, &SettingsScreen::on_brilho_changed, this);

    mk_sec_lbl(disp, "VOLUME");
    mk_slider_row(disp, "Sistema", vol_sys_,
                  &vsys_val_lbl_, &SettingsScreen::on_vsys_changed, this);
    mk_slider_row(disp, "M\xC3\xBAsica", vol_mus_,
                  &vmus_val_lbl_, &SettingsScreen::on_vmus_changed, this);
    mk_slider_row(disp, "Alarme", vol_alm_,
                  &valm_val_lbl_, &SettingsScreen::on_valm_changed, this);

    mk_hsep(disp);

    // Night mode toggle
    lv_obj_t* nm_row = mk_row(disp);
    lv_obj_set_flex_align(nm_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(nm_row,    14, 0);
    lv_obj_set_style_margin_bottom(nm_row, 12, 0);
    lv_obj_t* nm_info = mk_col(nm_row);
    lv_obj_set_size(nm_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    mk_lbl(nm_info, "Modo noturno",                &nova_font_14, kText);
    mk_lbl(nm_info, "Reduz brilho automaticamente", &nova_font_14, kSubtext);
    night_sw_ = lv_switch_create(nm_row);
    lv_obj_set_size(night_sw_, 44, 24);
    lv_obj_set_style_bg_color(night_sw_, lv_color_hex(kBorder), 0);
    lv_obj_set_style_bg_color(night_sw_, lv_color_hex(kAccent), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(night_sw_, lv_color_hex(kText),   LV_PART_KNOB);
    lv_obj_set_style_radius(night_sw_,   12, 0);
    lv_obj_set_style_radius(night_sw_,   10, LV_PART_KNOB);
    lv_obj_set_style_pad_all(night_sw_,  2,  LV_PART_KNOB);
    lv_obj_set_style_shadow_width(night_sw_, 0, 0);
    lv_obj_add_event_cb(night_sw_, &SettingsScreen::on_night_toggle_changed,
                        LV_EVENT_VALUE_CHANGED, this);

    // Time format row: "Formato de hora" + 24h/12h pills
    lv_obj_t* hf = mk_row(disp);
    lv_obj_set_flex_align(hf, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    mk_lbl(hf, "Formato de hora", &nova_font_14, kText);

    lv_obj_t* fmt_wrap = lv_obj_create(hf);
    lv_obj_set_size(fmt_wrap, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_bg_color(fmt_wrap,     lv_color_hex(kCard2), 0);
    lv_obj_set_style_bg_opa(fmt_wrap,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(fmt_wrap, lv_color_hex(kBorder), 0);
    lv_obj_set_style_border_width(fmt_wrap, 1, 0);
    lv_obj_set_style_radius(fmt_wrap,       7, 0);
    lv_obj_set_style_pad_all(fmt_wrap,      2, 0);
    lv_obj_set_style_shadow_width(fmt_wrap, 0, 0);
    lv_obj_set_flex_flow(fmt_wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollbar_mode(fmt_wrap, LV_SCROLLBAR_MODE_OFF);

    static lv_event_cb_t fmt_cbs[2] = {
        &SettingsScreen::on_fmt24_clicked,
        &SettingsScreen::on_fmt12_clicked,
    };
    static const char* fmt_labels[2] = { "24h", "12h" };
    for (int i = 0; i < 2; i++) {
        bool sel = (i == 0) == selected_24h_;
        lv_obj_t* pill = lv_obj_create(fmt_wrap);
        lv_obj_set_size(pill, LV_SIZE_CONTENT, LV_PCT(100));
        lv_obj_set_style_bg_color(pill,
            lv_color_hex(sel ? kSelected : kCard2), 0);
        lv_obj_set_style_bg_opa(pill,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(pill, 0, 0);
        lv_obj_set_style_radius(pill,       5, 0);
        lv_obj_set_style_pad_hor(pill,      10, 0);
        lv_obj_set_style_shadow_width(pill, 0, 0);
        lv_obj_set_scrollbar_mode(pill,     LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(pill, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t* pl = mk_lbl(pill, fmt_labels[i], &nova_font_14,
                               sel ? kText : kTextMuted);
        lv_obj_align(pl, LV_ALIGN_CENTER, 0, 0);
        fmt_pills_[i] = pill;
        lv_obj_add_event_cb(pill, fmt_cbs[i], LV_EVENT_CLICKED, this);
    }

    // ─────────────────────────────────────────────────────────────────────
    // COL 3: Sistema
    // ─────────────────────────────────────────────────────────────────────
    lv_obj_t* sys = mk_col_card(grid);
    mk_col_hdr(sys, LV_SYMBOL_SETTINGS, "Sistema");

    for (int i = 0; i < kSysRows; i++) {
        lv_obj_t* sr = mk_row(sys);
        lv_obj_set_flex_align(sr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_side(sr,   LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(sr,  lv_color_hex(kSep), 0);
        lv_obj_set_style_border_width(sr,  1, 0);
        lv_obj_set_style_pad_ver(sr,       9, 0);
        mk_lbl(sr, kSysKeys[i], &nova_font_14, kTextDim);
        sys_vals_[i] = mk_lbl(sr, "\xe2\x80\x94", &nova_font_14, kSysColors[i]);
    }

    // "Atualizar firmware" button (placeholder – no OTA yet)
    lv_obj_t* upd = lv_button_create(sys);
    lv_obj_set_size(upd, LV_PCT(100), 40);
    lv_obj_set_style_bg_color(upd,     lv_color_hex(kGreenBg), 0);
    lv_obj_set_style_border_color(upd, lv_color_hex(kGreenBd), 0);
    lv_obj_set_style_border_width(upd, 1, 0);
    lv_obj_set_style_radius(upd,       kRBtn, 0);
    lv_obj_set_style_shadow_width(upd, 0, 0);
    lv_obj_set_style_margin_top(upd,   12, 0);
    lv_obj_set_style_margin_bottom(upd, 7, 0);
    lv_obj_set_flex_flow(upd, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(upd, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(upd, 8, 0);
    mk_lbl(upd, LV_SYMBOL_DOWNLOAD,         &nova_font_14, kGreen);
    mk_lbl(upd, "Atualizar firmware v1.4",  &nova_font_14, kGreen);

    // "Reiniciar dispositivo" button
    lv_obj_t* rst = lv_button_create(sys);
    lv_obj_set_size(rst, LV_PCT(100), 40);
    lv_obj_set_style_bg_color(rst,     lv_color_hex(kCard2), 0);
    lv_obj_set_style_border_color(rst, lv_color_hex(kBorder), 0);
    lv_obj_set_style_border_width(rst, 1, 0);
    lv_obj_set_style_radius(rst,       kRBtn, 0);
    lv_obj_set_style_shadow_width(rst, 0, 0);
    lv_obj_set_flex_flow(rst, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rst, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(rst, 8, 0);
    mk_lbl(rst, LV_SYMBOL_REFRESH,       &nova_font_14, kTextDim);
    mk_lbl(rst, "Reiniciar dispositivo", &nova_font_14, kTextDim);
    lv_obj_add_event_cb(rst, &SettingsScreen::on_restart_clicked,
                        LV_EVENT_CLICKED, this);
}

// ── prefill: one-time init from saved preferences ─────────────────────────
void SettingsScreen::prefill(const AppState& state) {
    // Timezone: find matching index
    const auto& tz = state.preferences.timezone;
    for (int i = 0; i < kTzCount; i++) {
        if (tz == kTzList[i].id) { selected_tz_ = i; break; }
    }
    selected_24h_ = state.preferences.time_format_24h;
    update_tz_card();
    update_fmt_pills();

    // Profile initials + name
    const auto& name = state.preferences.display_name;
    if (!name.empty()) {
        lv_label_set_text(profile_name_lbl_, name.c_str());
        char ini[3] = { (char)toupper((unsigned char)name[0]), '\0', '\0' };
        for (size_t i = 1; i < name.size(); i++) {
            if (name[i - 1] == ' ' && i + 1 < name.size()) {
                ini[1] = (char)toupper((unsigned char)name[i]);
                break;
            }
        }
        lv_label_set_text(av_initials_lbl_, ini);
    }

    prefilled_ = true;
}

// ── update_live: called every render while screen is active ───────────────
void SettingsScreen::update_live(const AppState& state) {
    const bool online =
        state.onboarding.wifi_status == WifiConnectStatus::Connected;

    // Wi-Fi card
    const char* ssid = state.onboarding.pending_submission.wifi_ssid.empty()
                       ? (online ? "Conectado" : "\xe2\x80\x94")
                       : state.onboarding.pending_submission.wifi_ssid.c_str();
    if (std::strcmp(lv_label_get_text(wifi_ssid_lbl_), ssid) != 0)
        lv_label_set_text(wifi_ssid_lbl_, ssid);

    const char* detail = online
        ? "Conectado \xC2\xB7 NTP ativo"
        : (state.onboarding.wifi_status == WifiConnectStatus::Connecting
           ? "Conectando..." : "Sem conex\xC3\xA3o");
    if (std::strcmp(lv_label_get_text(wifi_details_lbl_), detail) != 0)
        lv_label_set_text(wifi_details_lbl_, detail);

    // System info
    const auto& sys = state.system;
    char buf[32];

    auto set_val = [&](int idx, const char* text) {
        if (std::strcmp(lv_label_get_text(sys_vals_[idx]), text) != 0)
            lv_label_set_text(sys_vals_[idx], text);
    };
    set_val(0, "NovaPanel v2");
    set_val(1, "v1.0.0");
    set_val(2, "ESP32-P4");
    set_val(3, sys.board_ready ? "312 KB" : "\xe2\x80\x94");
    set_val(4, sys.cache_ready ? "1.4 MB" : "\xe2\x80\x94");
    set_val(5, sys.reset_reason ? sys.reset_reason : "?");
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)sys.reboot_count);
    set_val(6, buf);
}

// ── render ────────────────────────────────────────────────────────────────
void SettingsScreen::render(const AppState& state) {
    if (!built_) { build(); built_ = true; }
    if (!prefilled_) prefill(state);
    lv_screen_load(root_);
    update_live(state);
}

}  // namespace nova
