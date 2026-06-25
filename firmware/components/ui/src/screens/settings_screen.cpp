// NovaPainel - ui/src/screens/settings_screen.cpp
// Post-onboarding settings screen (Fase 12). NOT host-checkable: lvgl.h has
// no host shim (see tools/scripts/host_check.sh, which skips this file).
// Faithful port of docs/design/lvgl_export_reference/screens/screen_config.c
// + novapanel_theme.h. Own lv_screen_load() (same as WizardScreen/SystemScreen).
// ADR-0017: on_submit_ enqueues work to the main task via app_main's
// action_queue — safe to call from LVGL event callbacks.
#include "settings_screen.hpp"

#include <cctype>
#include <cstring>

#include "lvgl.h"
#include "nova_fonts.hpp"

namespace nova {

namespace {
// -- Palette (novapanel_theme.h) --
constexpr uint32_t kColorBg        = 0x0D0F18;
constexpr uint32_t kColorCard      = 0x141721;
constexpr uint32_t kColorCard2     = 0x1B1E2D;
constexpr uint32_t kColorBorder    = 0x1E2235;
constexpr uint32_t kColorSep       = 0x1A1D2C;
constexpr uint32_t kColorAccent    = 0xE8A83C;
constexpr uint32_t kColorAccentBg  = 0x1C1900;
constexpr uint32_t kColorAccentBd  = 0x2C2700;
constexpr uint32_t kColorText      = 0xE8EAF2;
constexpr uint32_t kColorTextDim   = 0x7A8298;
constexpr uint32_t kColorTextMuted = 0x464E64;
constexpr uint32_t kColorGreen     = 0x4ABB78;
constexpr uint32_t kColorGreenBg   = 0x0F1D15;
constexpr uint32_t kColorGreenBd   = 0x1A3828;
constexpr uint32_t kColorDarkFg    = 0x090C12;
constexpr uint32_t kColorSelected  = 0x252A3C;  // active pill bg

constexpr int kRCard = 12;
constexpr int kRBtn  = 9;

struct TzEntry { const char* name; const char* offset; };
constexpr TzEntry kTzList[SettingsScreen::kTzCount] = {
    {"America/Sao_Paulo",  "UTC-3 - Brasil"},
    {"America/Manaus",     "UTC-4 - Brasil"},
    {"America/Rio_Branco", "UTC-5 - Brasil"},
    {"America/Noronha",    "UTC-2 - Brasil"},
    {"UTC",                "UTC+0 - Universal"},
};

// -- Helpers --
void clear_style(lv_obj_t* o) {
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

lv_obj_t* make_label(lv_obj_t* parent, const char* text,
                     const lv_font_t* font, uint32_t color) {
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
    lv_obj_set_size(o, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return o;
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

void make_hsep(lv_obj_t* parent) {
    lv_obj_t* s = lv_obj_create(parent);
    lv_obj_set_size(s, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(s, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_set_style_radius(s, 0, 0);
}

lv_obj_t* make_textarea(lv_obj_t* parent, const char* placeholder) {
    lv_obj_t* ta = lv_textarea_create(parent);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_obj_set_style_text_color(ta, lv_color_hex(kColorText), 0);
    return ta;
}

// Extracts up to 2 uppercase initials from a display name.
void get_initials(const char* name, char out[3]) {
    out[0] = out[1] = '?'; out[2] = '\0';
    if (!name || !*name) return;
    const char* p = name;
    while (*p == ' ') ++p;
    if (*p) out[0] = (char)std::toupper((unsigned char)*p);
    const char* sp = std::strchr(p, ' ');
    if (sp) {
        ++sp;
        while (*sp == ' ') ++sp;
        if (*sp) out[1] = (char)std::toupper((unsigned char)*sp);
    } else if (*(p + 1)) {
        out[1] = (char)std::toupper((unsigned char)*(p + 1));
    }
}

lv_obj_t* make_col_card(lv_obj_t* parent) {
    lv_obj_t* c = make_card(parent);
    lv_obj_set_style_flex_grow(c, 1, 0);
    lv_obj_set_height(c, LV_PCT(100));
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_ON);
    return c;
}

lv_obj_t* make_col_header(lv_obj_t* parent, const char* icon, const char* title) {
    lv_obj_t* h = make_row(parent);
    lv_obj_set_style_margin_bottom(h, 14, 0);
    lv_obj_set_style_pad_column(h, 7, 0);
    make_label(h, icon,  &nova_font_16, kColorAccent);
    make_label(h, title, &nova_font_14, kColorAccent);
    return h;
}

}  // namespace

// ---------------------------------------------------------------------------

void SettingsScreen::build() {
    root_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(root_, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);

    // ── Close header ──────────────────────────────────────────────────────
    lv_obj_t* hdr = lv_obj_create(root_);
    lv_obj_set_size(hdr, LV_PCT(100), 54);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_hor(hdr, 18, 0);
    lv_obj_set_style_pad_ver(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hdr, 10, 0);
    lv_obj_set_scrollbar_mode(hdr, LV_SCROLLBAR_MODE_OFF);

    make_label(hdr, LV_SYMBOL_SETTINGS, &nova_font_16, kColorAccent);
    make_label(hdr, "Configurações", &nova_font_20, kColorText);

    lv_obj_t* hsp = lv_obj_create(hdr);
    clear_style(hsp);
    lv_obj_set_style_flex_grow(hsp, 1, 0);

    lv_obj_t* close_btn = lv_button_create(hdr);
    lv_obj_set_size(close_btn, LV_SIZE_CONTENT, 36);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(kColorCard2), 0);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(close_btn, 1, 0);
    lv_obj_set_style_radius(close_btn, kRBtn, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_pad_hor(close_btn, 16, 0);
    lv_obj_t* close_lbl = make_label(close_btn, LV_SYMBOL_CLOSE " Fechar",
                                     &nova_font_14, kColorTextDim);
    lv_obj_align(close_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(close_btn, &SettingsScreen::on_close_clicked,
                        LV_EVENT_CLICKED, this);

    // ── Body: profile bar + 3-col grid ───────────────────────────────────
    lv_obj_t* body = lv_obj_create(root_);
    lv_obj_set_size(body, LV_PCT(100), 0);
    lv_obj_set_style_flex_grow(body, 1, 0);
    lv_obj_set_style_pad_all(body, 11, 0);
    lv_obj_set_style_pad_row(body, 11, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_radius(body, 0, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);

    // ── Profile bar ───────────────────────────────────────────────────────
    lv_obj_t* prof = make_card(body);
    lv_obj_set_size(prof, LV_PCT(100), 68);
    lv_obj_set_style_pad_hor(prof, 18, 0);
    lv_obj_set_style_pad_ver(prof, 0, 0);
    lv_obj_set_flex_flow(prof, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(prof, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(prof, 14, 0);

    lv_obj_t* av = lv_obj_create(prof);
    lv_obj_set_size(av, 42, 42);
    lv_obj_set_style_bg_color(av, lv_color_hex(kColorAccent), 0);
    lv_obj_set_style_bg_opa(av, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(av, 21, 0);
    lv_obj_set_style_border_width(av, 0, 0);
    lv_obj_set_style_pad_all(av, 0, 0);
    lv_obj_set_scrollbar_mode(av, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* av_lbl = make_label(av, "??", &nova_font_16, kColorDarkFg);
    lv_obj_align(av_lbl, LV_ALIGN_CENTER, 0, 0);
    av_initials_label_ = av_lbl;

    lv_obj_t* pi = make_col(prof);
    lv_obj_set_style_flex_grow(pi, 1, 0);
    lv_obj_set_width(pi, 0);
    profile_name_label_ = make_label(pi, "—", &nova_font_16, kColorText);
    make_label(pi, "Perfil · NovaPainel ESP32-P4", &nova_font_14, 0x5A6478);

    lv_obj_t* edit_btn = lv_button_create(prof);
    lv_obj_set_size(edit_btn, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(edit_btn, lv_color_hex(kColorAccentBg), 0);
    lv_obj_set_style_border_color(edit_btn, lv_color_hex(kColorAccentBd), 0);
    lv_obj_set_style_border_width(edit_btn, 1, 0);
    lv_obj_set_style_radius(edit_btn, 15, 0);
    lv_obj_set_style_shadow_width(edit_btn, 0, 0);
    lv_obj_set_style_pad_hor(edit_btn, 14, 0);
    lv_obj_t* edit_lbl = make_label(edit_btn, "Editar nome", &nova_font_14, kColorAccent);
    lv_obj_align(edit_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(edit_btn, &SettingsScreen::on_edit_name_clicked,
                        LV_EVENT_CLICKED, this);

    // ── 3-col grid ────────────────────────────────────────────────────────
    lv_obj_t* grid = lv_obj_create(body);
    lv_obj_set_size(grid, LV_PCT(100), 0);
    lv_obj_set_style_flex_grow(grid, 1, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_radius(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(grid, 11, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    // ── COL 1: Rede ──────────────────────────────────────────────────────
    lv_obj_t* net = make_col_card(grid);
    make_col_header(net, LV_SYMBOL_WIFI, "Rede");

    make_label(net, "WI-FI", &nova_font_14, kColorTextMuted);

    lv_obj_t* wc = lv_obj_create(net);
    lv_obj_set_size(wc, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wc, lv_color_hex(kColorCard2), 0);
    lv_obj_set_style_bg_opa(wc, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(wc, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_border_width(wc, 1, 0);
    lv_obj_set_style_radius(wc, 9, 0);
    lv_obj_set_style_pad_all(wc, 11, 0);
    lv_obj_set_style_margin_top(wc, 7, 0);
    lv_obj_set_scrollbar_mode(wc, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wc, LV_FLEX_FLOW_COLUMN);
    wifi_ssid_label_   = make_label(wc, "—", &nova_font_14, kColorText);
    wifi_status_label_ = make_label(wc, "Verificando...", &nova_font_14, kColorTextMuted);

    lv_obj_t* wm = make_label(net, "Gerenciar redes Wi-Fi " LV_SYMBOL_RIGHT,
                              &nova_font_14, kColorAccent);
    lv_obj_set_style_margin_top(wm, 7, 0);
    lv_obj_set_style_margin_bottom(wm, 14, 0);
    lv_obj_add_flag(wm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(wm, &SettingsScreen::on_wifi_reconfigure_clicked,
                        LV_EVENT_CLICKED, this);

    make_hsep(net);

    lv_obj_t* tz_hdr = make_row(net);
    lv_obj_set_style_margin_top(tz_hdr, 14, 0);
    lv_obj_set_style_margin_bottom(tz_hdr, 8, 0);
    make_label(tz_hdr, "FUSO HORÁRIO", &nova_font_14, kColorTextMuted);

    for (int i = 0; i < kTzCount; ++i) {
        lv_obj_t* row = lv_obj_create(net);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, lv_color_hex(kColorCard2), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(kColorBorder), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 9, 0);
        lv_obj_set_style_pad_all(row, 10, 0);
        lv_obj_set_style_margin_bottom(row, 5, 0);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* chk = lv_obj_create(row);
        lv_obj_set_size(chk, 16, 16);
        lv_obj_set_style_radius(chk, 8, 0);
        lv_obj_set_style_bg_color(chk, lv_color_hex(kColorCard), 0);
        lv_obj_set_style_bg_opa(chk, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(chk, lv_color_hex(kColorBorder), 0);
        lv_obj_set_style_border_width(chk, 2, 0);
        lv_obj_set_style_pad_all(chk, 0, 0);
        lv_obj_set_scrollbar_mode(chk, LV_SCROLLBAR_MODE_OFF);

        make_label(row, kTzList[i].name,   &nova_font_14, kColorText);
        make_label(row, kTzList[i].offset, &nova_font_14, kColorTextMuted);

        lv_obj_add_event_cb(row, &SettingsScreen::on_tz_clicked, LV_EVENT_CLICKED, this);
        tz_rows_[i]   = row;
        tz_checks_[i] = chk;
    }

    // ── COL 2: Nome & Hora ───────────────────────────────────────────────
    lv_obj_t* disp = make_col_card(grid);
    make_col_header(disp, LV_SYMBOL_TINT, "Nome & Hora");

    make_label(disp, "NOME DO PAINEL", &nova_font_14, kColorTextMuted);
    name_input_ = make_textarea(disp, "Nome do painel");
    lv_obj_set_width(name_input_, LV_PCT(100));
    lv_obj_set_style_margin_bottom(name_input_, 14, 0);
    lv_obj_add_event_cb(name_input_, &SettingsScreen::on_textarea_focused,
                        LV_EVENT_FOCUSED, this);

    make_hsep(disp);

    lv_obj_t* fmt_hdr = make_row(disp);
    lv_obj_set_style_margin_top(fmt_hdr, 14, 0);
    lv_obj_set_style_margin_bottom(fmt_hdr, 8, 0);
    make_label(fmt_hdr, "FORMATO DE HORA", &nova_font_14, kColorTextMuted);

    // pill-style 24h/12h toggle (from reference)
    lv_obj_t* fmt_wrap = lv_obj_create(disp);
    lv_obj_set_size(fmt_wrap, LV_SIZE_CONTENT, 34);
    lv_obj_set_style_bg_color(fmt_wrap, lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(fmt_wrap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(fmt_wrap, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(fmt_wrap, 1, 0);
    lv_obj_set_style_radius(fmt_wrap, 7, 0);
    lv_obj_set_style_pad_all(fmt_wrap, 2, 0);
    lv_obj_set_flex_flow(fmt_wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollbar_mode(fmt_wrap, LV_SCROLLBAR_MODE_OFF);

    static const char* kFmtLabels[2] = {"24h", "12h"};
    for (int i = 0; i < kFmtCount; ++i) {
        lv_obj_t* fb = lv_obj_create(fmt_wrap);
        lv_obj_set_size(fb, LV_SIZE_CONTENT, LV_PCT(100));
        lv_obj_set_style_bg_color(fb, lv_color_hex(kColorCard2), 0);
        lv_obj_set_style_bg_opa(fb, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(fb, 0, 0);
        lv_obj_set_style_radius(fb, 5, 0);
        lv_obj_set_style_pad_hor(fb, 16, 0);
        lv_obj_set_scrollbar_mode(fb, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t* fl = make_label(fb, kFmtLabels[i], &nova_font_14, kColorTextMuted);
        lv_obj_align(fl, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(fb, &SettingsScreen::on_format_clicked, LV_EVENT_CLICKED, this);
        fmt_rows_[i]   = fb;
        fmt_checks_[i] = fl;  // reuse as the text label for color update
    }
    lv_obj_set_style_margin_bottom(fmt_wrap, 14, 0);

    // keyboard — hidden, appears when name_input_ is focused
    keyboard_ = lv_keyboard_create(disp);
    lv_obj_add_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(keyboard_, &SettingsScreen::on_kb_ready,
                        LV_EVENT_READY, this);
    lv_obj_add_event_cb(keyboard_, &SettingsScreen::on_kb_cancel,
                        LV_EVENT_CANCEL, this);

    // ── COL 3: Sistema ───────────────────────────────────────────────────
    lv_obj_t* sys = make_col_card(grid);
    make_col_header(sys, LV_SYMBOL_SETTINGS, "Sistema");

    // System info rows — labels stored for live update in render()
    struct { const char* key; lv_obj_t** out; } kSysRows[] = {
        {"Chip",         &sys_chip_label_},
        {"Motivo reset", &sys_reset_label_},
        {"Reboots",      &sys_reboots_label_},
        {"Rede",         &sys_net_label_},
    };
    for (auto& r : kSysRows) {
        lv_obj_t* row = make_row(sys);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(kColorSep), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 9, 0);
        make_label(row, r.key, &nova_font_14, kColorTextDim);
        *r.out = make_label(row, "—", &nova_font_14, kColorText);
    }

    lv_obj_t* sp = lv_obj_create(sys);
    clear_style(sp);
    lv_obj_set_size(sp, 1, 1);
    lv_obj_set_style_flex_grow(sp, 1, 0);

    lv_obj_t* rst = lv_button_create(sys);
    lv_obj_set_size(rst, LV_PCT(100), 40);
    lv_obj_set_style_bg_color(rst, lv_color_hex(kColorCard2), 0);
    lv_obj_set_style_border_color(rst, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(rst, 1, 0);
    lv_obj_set_style_radius(rst, kRBtn, 0);
    lv_obj_set_style_shadow_width(rst, 0, 0);
    lv_obj_set_style_margin_top(rst, 12, 0);
    lv_obj_set_flex_flow(rst, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rst, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(rst, 8, 0);
    make_label(rst, LV_SYMBOL_REFRESH,       &nova_font_14, kColorTextDim);
    make_label(rst, "Reiniciar dispositivo", &nova_font_14, kColorTextDim);
    lv_obj_add_event_cb(rst, &SettingsScreen::on_restart_clicked, LV_EVENT_CLICKED, this);
}

// ---------------------------------------------------------------------------

void SettingsScreen::prefill(const AppState& state) {
    const auto& p = state.preferences;

    lv_textarea_set_text(name_input_, p.display_name.c_str());

    char initials[3];
    get_initials(p.display_name.c_str(), initials);
    lv_label_set_text(av_initials_label_, initials);
    lv_label_set_text(profile_name_label_, p.display_name.empty() ? "—" : p.display_name.c_str());

    selected_tz_ = 0;
    for (int i = 0; i < kTzCount; ++i) {
        if (p.timezone == kTzList[i].name) { selected_tz_ = i; break; }
    }
    selected_24h_ = p.time_format_24h;

    update_tz_ui();
    update_format_ui();
}

void SettingsScreen::update_tz_ui() {
    for (int i = 0; i < kTzCount; ++i) {
        const bool sel = (i == selected_tz_);
        lv_obj_set_style_bg_color(tz_checks_[i],
            lv_color_hex(sel ? kColorAccentBg : kColorCard), 0);
        lv_obj_set_style_border_color(tz_checks_[i],
            lv_color_hex(sel ? kColorAccent : kColorBorder), 0);
        lv_obj_set_style_border_width(tz_checks_[i], sel ? 5 : 2, 0);
    }
}

void SettingsScreen::update_format_ui() {
    // fmt_checks_[0] = 24h label, fmt_checks_[1] = 12h label
    // fmt_rows_[0]   = 24h pill,  fmt_rows_[1]   = 12h pill
    for (int i = 0; i < kFmtCount; ++i) {
        const bool sel = (i == 0) ? selected_24h_ : !selected_24h_;
        lv_obj_set_style_bg_color(fmt_rows_[i],
            lv_color_hex(sel ? kColorSelected : kColorCard2), 0);
        lv_obj_set_style_text_color(fmt_checks_[i],
            lv_color_hex(sel ? kColorText : kColorTextMuted), 0);
    }
}

void SettingsScreen::update_live(const AppState& state) {
    const bool online = state.onboarding.wifi_status == WifiConnectStatus::Connected;

    // Wi-Fi card
    const char* ssid_text = state.onboarding.wifi_status == WifiConnectStatus::Connected
                            && !state.preferences.display_name.empty()
                            ? "Conectado" : "—";
    // Use actual SSID from pending_submission if available, else generic
    // We don't store SSID in AppState post-connect, so show connection status
    const char* status_text = online ? "Conectado" : "Desconectado";
    if (std::strcmp(lv_label_get_text(wifi_status_label_), status_text) != 0) {
        lv_label_set_text(wifi_status_label_, status_text);
        lv_obj_set_style_text_color(wifi_status_label_,
            lv_color_hex(online ? kColorGreen : kColorTextMuted), 0);
    }

    // System info
    char buf[32];
    const auto& sys = state.system;
    const char* chip_str = "ESP32-P4";
    if (std::strcmp(lv_label_get_text(sys_chip_label_), chip_str) != 0)
        lv_label_set_text(sys_chip_label_, chip_str);
    if (std::strcmp(lv_label_get_text(sys_reset_label_), sys.reset_reason) != 0)
        lv_label_set_text(sys_reset_label_, sys.reset_reason);
    std::snprintf(buf, sizeof(buf), "%lu", (unsigned long)sys.reboot_count);
    if (std::strcmp(lv_label_get_text(sys_reboots_label_), buf) != 0)
        lv_label_set_text(sys_reboots_label_, buf);
    const char* net_str = sys.network_ready ? "OK" : "Sem rede";
    if (std::strcmp(lv_label_get_text(sys_net_label_), net_str) != 0)
        lv_label_set_text(sys_net_label_, net_str);
}

void SettingsScreen::show_keyboard_for(lv_obj_t* ta) {
    lv_keyboard_set_textarea(keyboard_, ta);
    lv_obj_remove_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::submit_name() {
    OnboardingSubmission sub;
    sub.step         = OnboardingStep::DisplayName;
    sub.display_name = lv_textarea_get_text(name_input_);
    on_submit_(sub);
    // Update profile bar immediately (will sync from state on next render)
    char initials[3];
    get_initials(sub.display_name.c_str(), initials);
    lv_label_set_text(av_initials_label_, initials);
    lv_label_set_text(profile_name_label_,
                      sub.display_name.empty() ? "—" : sub.display_name.c_str());
}

void SettingsScreen::submit_tz_format() {
    OnboardingSubmission sub;
    sub.step            = OnboardingStep::TimezoneAndFormat;
    sub.timezone        = kTzList[selected_tz_].name;
    sub.time_format_24h = selected_24h_;
    on_submit_(sub);
}

// ---------------------------------------------------------------------------
// render()

void SettingsScreen::render(const AppState& state) {
    if (!built_) {
        build();
        built_ = true;
    }
    if (lv_screen_active() != root_) {
        lv_screen_load(root_);
        prefilled_ = false;
    }
    if (!prefilled_) {
        prefill(state);
        prefilled_ = true;
    }
    update_live(state);
}

// ---------------------------------------------------------------------------
// Static callbacks

void SettingsScreen::on_close_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->on_navigate_(ScreenId::Home);
}

void SettingsScreen::on_edit_name_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    lv_obj_scroll_to_view(self->name_input_, LV_ANIM_OFF);
    lv_obj_add_state(self->name_input_, LV_STATE_FOCUSED);
    self->show_keyboard_for(self->name_input_);
}

void SettingsScreen::on_textarea_focused(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->show_keyboard_for(static_cast<lv_obj_t*>(lv_event_get_target(e)));
}

void SettingsScreen::on_kb_ready(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    lv_obj_add_flag(self->keyboard_, LV_OBJ_FLAG_HIDDEN);
    self->submit_name();
}

void SettingsScreen::on_kb_cancel(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    lv_obj_add_flag(self->keyboard_, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::on_tz_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    for (int i = 0; i < kTzCount; ++i) {
        if (self->tz_rows_[i] == target) {
            self->selected_tz_ = i;
            break;
        }
    }
    self->update_tz_ui();
    self->submit_tz_format();
}

void SettingsScreen::on_format_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (target == self->fmt_rows_[0])      self->selected_24h_ = true;
    else if (target == self->fmt_rows_[1]) self->selected_24h_ = false;
    self->update_format_ui();
    self->submit_tz_format();
}

void SettingsScreen::on_wifi_reconfigure_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    self->on_navigate_(ScreenId::Setup);
}

void SettingsScreen::on_restart_clicked(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    if (self->on_restart_) self->on_restart_();
}

}  // namespace nova
