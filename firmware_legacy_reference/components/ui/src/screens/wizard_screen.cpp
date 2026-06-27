// NovaPainel - ui/src/screens/wizard_screen.cpp
// Real LVGL widgets (Fase 5). NOT host-checkable: lvgl.h has no host shim
// (see tools/scripts/host_check.sh, which skips this file on purpose).
// Faithful port of docs/design/lvgl_export_reference/screens/screen_setup.c
// (v2) + novapanel_theme.h. See wizard_screen.hpp for the architecture note
// on local vs. submitted state.
#include "wizard_screen.hpp"

#include "lvgl.h"
#include "nova_fonts.hpp"

namespace nova {

namespace {
// -- Palette, from docs/design/lvgl_export_reference/novapanel_theme.h --
constexpr uint32_t kColorBg        = 0x0D0F18;  // NP_C_BG
constexpr uint32_t kColorCard      = 0x141721;  // NP_C_CARD
constexpr uint32_t kColorCard2     = 0x1B1E2D;  // NP_C_CARD2
constexpr uint32_t kColorBorder    = 0x1E2235;  // NP_C_BORDER
constexpr uint32_t kColorAccent    = 0xE8A83C;  // NP_C_ACCENT
constexpr uint32_t kColorAccentBg  = 0x1C1900;  // NP_C_ACCENT_BG
constexpr uint32_t kColorAccentBd  = 0x2C2700;  // NP_C_ACCENT_BD
constexpr uint32_t kColorText      = 0xE8EAF2;  // NP_C_TEXT
constexpr uint32_t kColorTextDim   = 0x7A8298;  // NP_C_TEXT_DIM
constexpr uint32_t kColorTextMuted = 0x464E64;  // NP_C_TEXT_MUTED
constexpr uint32_t kColorGreen     = 0x4ABB78;  // NP_C_GREEN
constexpr uint32_t kColorGreenBg   = 0x0F1D15;  // NP_C_GREEN_BG
constexpr uint32_t kColorRed       = 0xD05252;  // NP_C_RED
constexpr uint32_t kColorDarkFg    = 0x090C12;  // NP_C_DARK_FG

constexpr int kRBtn = 9;  // NP_R_BTN

// Curated MVP list (no full IANA TZ database - see app_state.hpp comment on
// UserPreferences::timezone). Brazil-first since the product targets BRL/PT.
struct TzEntry { const char* name; const char* offset; };
constexpr TzEntry kTimezoneList[WizardScreen::kTzCount] = {
    {"America/Sao_Paulo",  "UTC-3 - Brasil"},
    {"America/Manaus",     "UTC-4 - Brasil"},
    {"America/Rio_Branco", "UTC-5 - Brasil"},
    {"America/Noronha",    "UTC-2 - Brasil"},
    {"UTC",                "UTC+0 - Universal"},
};

int step_index(OnboardingStep step) {
    switch (step) {
        case OnboardingStep::DisplayName:        return 0;
        case OnboardingStep::Wifi:                return 1;
        case OnboardingStep::TimezoneAndFormat:   return 2;
        case OnboardingStep::Confirmation:        return 3;
        case OnboardingStep::Done:                return 3;
    }
    return 0;
}
OnboardingStep step_at(int idx) {
    switch (idx) {
        case 0: return OnboardingStep::DisplayName;
        case 1: return OnboardingStep::Wifi;
        case 2: return OnboardingStep::TimezoneAndFormat;
        default: return OnboardingStep::Confirmation;
    }
}

constexpr const char* kStepTitles[4]   = {"Perfil", "Wi-Fi", "Fuso Horário", "Tudo certo!"};
constexpr const char* kStepCaptions[4] = {"Passo 1 de 3", "Passo 2 de 3", "Passo 3 de 3", ""};
constexpr uint8_t     kStepPct[4]      = {12, 40, 70, 100};

// -- Theme helpers, mirroring novapanel_theme.h's np_* functions --
void clear_style(lv_obj_t* obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

lv_obj_t* make_label(lv_obj_t* parent, const char* text, const lv_font_t* font, uint32_t color) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
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

lv_obj_t* make_confirm_row(lv_obj_t* parent, const char* key, const char* value, uint32_t value_color) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_all(row, 12, 0);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(row, 8, 0);
    make_label(row, key, &nova_font_14, kColorTextDim);
    return make_label(row, value, &nova_font_14, value_color);
}

lv_obj_t* make_textarea(lv_obj_t* parent, const char* placeholder, bool password) {
    lv_obj_t* ta = lv_textarea_create(parent);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_textarea_set_password_mode(ta, password);
    lv_obj_set_style_text_color(ta, lv_color_hex(kColorText), 0);
    return ta;
}
}  // namespace

void WizardScreen::build() {
    // Own LVGL screen object (not lv_screen_active()) - see home_screen.cpp.
    root_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(root_, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);

    // -- progress bar --
    lv_obj_t* prog_track = lv_obj_create(root_);
    lv_obj_set_size(prog_track, LV_PCT(100), 3);
    lv_obj_set_style_bg_color(prog_track, lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(prog_track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(prog_track, 0, 0);
    lv_obj_set_style_radius(prog_track, 0, 0);
    lv_obj_set_style_pad_all(prog_track, 0, 0);
    lv_obj_set_scrollbar_mode(prog_track, LV_SCROLLBAR_MODE_OFF);

    progress_bar_ = lv_bar_create(prog_track);
    lv_obj_set_size(progress_bar_, LV_PCT(100), 3);
    lv_bar_set_range(progress_bar_, 0, 100);
    lv_bar_set_value(progress_bar_, kStepPct[0], LV_ANIM_OFF);
    lv_obj_set_style_bg_color(progress_bar_, lv_color_hex(0x1A1D2C), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar_, lv_color_hex(kColorAccent), LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar_, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar_, 0, LV_PART_INDICATOR);

    // -- step header --
    lv_obj_t* hdr = lv_obj_create(root_);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 16, 0);
    lv_obj_set_style_pad_bottom(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hdr, 10, 0);
    lv_obj_set_scrollbar_mode(hdr, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* num_badge = lv_obj_create(hdr);
    lv_obj_set_size(num_badge, 30, 30);
    lv_obj_set_style_bg_color(num_badge, lv_color_hex(kColorAccentBg), 0);
    lv_obj_set_style_border_color(num_badge, lv_color_hex(kColorAccentBd), 0);
    lv_obj_set_style_border_width(num_badge, 1, 0);
    lv_obj_set_style_radius(num_badge, 15, 0);
    lv_obj_set_style_pad_all(num_badge, 0, 0);
    lv_obj_set_scrollbar_mode(num_badge, LV_SCROLLBAR_MODE_OFF);
    step_num_label_ = make_label(num_badge, "1", &nova_font_14, kColorAccent);
    lv_obj_align(step_num_label_, LV_ALIGN_CENTER, 0, 0);

    step_title_label_ = make_label(hdr, kStepTitles[0], &nova_font_16, kColorAccent);

    lv_obj_t* hdr_sp = lv_obj_create(hdr);
    clear_style(hdr_sp);
    lv_obj_set_style_flex_grow(hdr_sp, 1, 0);

    step_caption_label_ = make_label(hdr, kStepCaptions[0], &nova_font_14, kColorTextMuted);

    // -- body (flex:1), holds the 4 step panels --
    lv_obj_t* body = lv_obj_create(root_);
    lv_obj_set_size(body, LV_PCT(100), 0);
    lv_obj_set_style_flex_grow(body, 1, 0);
    clear_style(body);

    build_name_panel(body);
    build_wifi_panel(body);
    build_tz_panel(body);
    build_confirmation_panel(body);

    // -- footer nav --
    lv_obj_t* footer = lv_obj_create(root_);
    lv_obj_set_size(footer, LV_PCT(100), 62);
    lv_obj_set_style_bg_color(footer, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 14, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, 12, 0);
    lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);

    back_btn_ = lv_button_create(footer);
    lv_obj_set_size(back_btn_, LV_SIZE_CONTENT, 46);
    lv_obj_set_style_bg_color(back_btn_, lv_color_hex(kColorCard2), 0);
    lv_obj_set_style_border_color(back_btn_, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(back_btn_, 1, 0);
    lv_obj_set_style_radius(back_btn_, kRBtn, 0);
    lv_obj_set_style_shadow_width(back_btn_, 0, 0);
    lv_obj_set_style_pad_hor(back_btn_, 22, 0);
    lv_obj_t* back_l = make_label(back_btn_, "<- Voltar", &nova_font_14, kColorTextDim);
    lv_obj_align(back_l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(back_btn_, &WizardScreen::on_back_clicked, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* footer_sp = lv_obj_create(footer);
    clear_style(footer_sp);
    lv_obj_set_style_flex_grow(footer_sp, 1, 0);

    status_label_ = make_label(footer, "", &nova_font_14, kColorTextDim);
    lv_obj_set_style_margin_right(status_label_, 12, 0);

    next_btn_ = lv_button_create(footer);
    lv_obj_set_size(next_btn_, LV_SIZE_CONTENT, 46);
    lv_obj_set_style_bg_color(next_btn_, lv_color_hex(kColorAccent), 0);
    lv_obj_set_style_radius(next_btn_, kRBtn, 0);
    lv_obj_set_style_shadow_width(next_btn_, 0, 0);
    lv_obj_set_style_pad_hor(next_btn_, 24, 0);
    next_label_ = make_label(next_btn_, "Continuar ->", &nova_font_14, kColorDarkFg);
    lv_obj_align(next_label_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(next_btn_, &WizardScreen::on_next_clicked, LV_EVENT_CLICKED, this);

    built_ = true;
    refresh_chrome();
}

void WizardScreen::build_name_panel(lv_obj_t* body) {
    panel_name_ = lv_obj_create(body);
    lv_obj_set_size(panel_name_, LV_PCT(100), LV_PCT(100));
    clear_style(panel_name_);
    lv_obj_set_flex_flow(panel_name_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel_name_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(panel_name_, 80, 0);

    lv_obj_t* avatar = lv_obj_create(panel_name_);
    lv_obj_set_size(avatar, 76, 76);
    lv_obj_set_style_bg_color(avatar, lv_color_hex(kColorAccentBg), 0);
    lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(avatar, lv_color_hex(kColorAccent), 0);
    lv_obj_set_style_border_width(avatar, 2, 0);
    lv_obj_set_style_radius(avatar, 38, 0);
    lv_obj_set_style_pad_all(avatar, 0, 0);
    lv_obj_set_style_margin_bottom(avatar, 22, 0);
    lv_obj_set_scrollbar_mode(avatar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* av_l = make_label(avatar, "?", &nova_font_28, kColorAccent);
    lv_obj_align(av_l, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* q = make_label(panel_name_, "Como quer ser chamado?", &nova_font_24, kColorText);
    lv_obj_set_style_text_align(q, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(q, 10, 0);

    lv_obj_t* sub = make_label(panel_name_,
        "Seu nome aparecerá nas saudações\ne no perfil do dispositivo.",
        &nova_font_14, kColorTextDim);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(sub, 30, 0);

    name_input_ = make_textarea(panel_name_, "Seu nome", false);
    lv_obj_set_width(name_input_, 340);
    lv_obj_set_style_bg_color(name_input_, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_border_color(name_input_, lv_color_hex(kColorAccentBd), 0);
    lv_obj_set_style_border_width(name_input_, 1, 0);
    lv_obj_set_style_radius(name_input_, 12, 0);
    lv_obj_set_style_text_font(name_input_, &nova_font_20, 0);
    lv_obj_set_style_text_align(name_input_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_event_cb(name_input_, &WizardScreen::on_textarea_focused, LV_EVENT_FOCUSED, this);
}

void WizardScreen::build_wifi_panel(lv_obj_t* body) {
    panel_wifi_ = lv_obj_create(body);
    lv_obj_set_size(panel_wifi_, LV_PCT(100), LV_PCT(100));
    clear_style(panel_wifi_);
    lv_obj_set_flex_flow(panel_wifi_, LV_FLEX_FLOW_ROW);
    lv_obj_add_flag(panel_wifi_, LV_OBJ_FLAG_HIDDEN);

    // -- network list --
    lv_obj_t* wlist = lv_obj_create(panel_wifi_);
    lv_obj_set_style_flex_grow(wlist, 1, 0);
    lv_obj_set_height(wlist, LV_PCT(100));
    lv_obj_set_style_bg_opa(wlist, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wlist, 0, 0);
    lv_obj_set_style_pad_all(wlist, 16, 0);
    lv_obj_set_flex_flow(wlist, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wlist, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(wlist, 7, 0);
    lv_obj_set_scrollbar_mode(wlist, LV_SCROLLBAR_MODE_ON);

    lv_obj_t* wl_hdr = make_label(wlist, "Selecione a rede Wi-Fi do seu ambiente:",
                                  &nova_font_14, kColorTextDim);
    lv_obj_set_style_margin_bottom(wl_hdr, 7, 0);

    wifi_list_ = lv_obj_create(wlist);
    clear_style(wifi_list_);
    lv_obj_set_size(wifi_list_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(wifi_list_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(wifi_list_, 7, 0);

    wifi_status_text_ = make_label(wifi_list_, "Toque em 'Atualizar lista' para buscar redes.",
                                   &nova_font_14, kColorTextDim);

    // All kMaxWifiRows rows are pre-built ONCE, right here, hidden until
    // refresh_wifi_network_list() (called from render(), off the touch
    // path) maps real scan results onto them. Selecting one only recolors
    // these existing rows (update_wifi_selection_ui()) - no
    // create/destroy ever happens from a touch event callback. See
    // ADR-0025: rebuilding ~24 rows synchronously inside a click handler
    // overflowed taskLVGL's stack on real hardware.
    for (int i = 0; i < kMaxWifiRows; ++i) {
        lv_obj_t* row = lv_obj_create(wifi_list_);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(kColorCard), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_set_style_pad_all(row, 13, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 12, 0);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t* ic_box = lv_obj_create(row);
        lv_obj_set_size(ic_box, 38, 38);
        lv_obj_set_style_bg_color(ic_box, lv_color_hex(kColorCard2), 0);
        lv_obj_set_style_border_width(ic_box, 0, 0);
        lv_obj_set_style_radius(ic_box, 9, 0);
        lv_obj_set_style_pad_all(ic_box, 0, 0);
        lv_obj_set_scrollbar_mode(ic_box, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t* ic = make_label(ic_box, LV_SYMBOL_WIFI, &nova_font_16, kColorTextDim);
        lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t* info = make_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        make_label(info, "", &nova_font_14, 0xC4C8D6);          // ssid - set in refresh_wifi_network_list()
        make_label(info, "", &nova_font_14, kColorTextMuted);   // "Protegida"/"Aberta"

        lv_obj_t* chk = lv_obj_create(row);
        lv_obj_set_size(chk, 22, 22);
        lv_obj_set_style_bg_color(chk, lv_color_hex(kColorAccent), 0);
        lv_obj_set_style_border_width(chk, 0, 0);
        lv_obj_set_style_radius(chk, 11, 0);
        lv_obj_set_style_pad_all(chk, 0, 0);
        lv_obj_set_scrollbar_mode(chk, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t* chk_ic = make_label(chk, LV_SYMBOL_OK, &nova_font_14, kColorDarkFg);
        lv_obj_align(chk_ic, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(row, &WizardScreen::on_wifi_network_clicked, LV_EVENT_CLICKED, this);
        wifi_rows_[i] = row;
        wifi_row_checks_[i] = chk;
    }

    lv_obj_t* rescan_l = make_label(wlist, "Atualizar lista", &nova_font_14, kColorAccent);
    lv_obj_set_style_margin_top(rescan_l, 4, 0);
    lv_obj_add_flag(rescan_l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(rescan_l, &WizardScreen::on_wifi_rescan, LV_EVENT_CLICKED, this);

    // -- password side panel --
    lv_obj_t* wpanel = lv_obj_create(panel_wifi_);
    lv_obj_set_size(wpanel, 258, LV_PCT(100));
    lv_obj_set_style_bg_color(wpanel, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(wpanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(wpanel, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(wpanel, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(wpanel, 1, 0);
    lv_obj_set_style_radius(wpanel, 0, 0);
    lv_obj_set_style_pad_all(wpanel, 24, 0);
    lv_obj_set_flex_flow(wpanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wpanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(wpanel, LV_SCROLLBAR_MODE_OFF);

    make_label(wpanel, "REDE SELECIONADA", &nova_font_14, kColorTextMuted);
    wifi_ssid_label_ = make_label(wpanel, "Nenhuma", &nova_font_16, kColorText);
    lv_obj_set_style_margin_top(wifi_ssid_label_, 8, 0);
    lv_obj_set_style_margin_bottom(wifi_ssid_label_, 20, 0);

    make_label(wpanel, "Senha", &nova_font_14, kColorTextDim);

    password_input_ = make_textarea(wpanel, "Senha da rede", true);
    lv_obj_set_width(password_input_, LV_PCT(100));
    lv_obj_set_style_bg_color(password_input_, lv_color_hex(kColorCard2), 0);
    lv_obj_set_style_border_color(password_input_, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(password_input_, 1, 0);
    lv_obj_set_style_radius(password_input_, 9, 0);
    lv_obj_set_style_text_font(password_input_, &nova_font_16, 0);
    lv_obj_set_style_margin_top(password_input_, 8, 0);
    lv_obj_set_style_margin_bottom(password_input_, 14, 0);
    lv_obj_add_event_cb(password_input_, &WizardScreen::on_textarea_focused, LV_EVENT_FOCUSED, this);

    make_label(wpanel, "Deixe em branco para\nredes abertas.", &nova_font_14, kColorTextMuted);
}

void WizardScreen::build_tz_panel(lv_obj_t* body) {
    panel_tz_ = lv_obj_create(body);
    lv_obj_set_size(panel_tz_, LV_PCT(100), LV_PCT(100));
    clear_style(panel_tz_);
    lv_obj_set_flex_flow(panel_tz_, LV_FLEX_FLOW_ROW);
    lv_obj_add_flag(panel_tz_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* tzlist = lv_obj_create(panel_tz_);
    lv_obj_set_style_flex_grow(tzlist, 1, 0);
    lv_obj_set_height(tzlist, LV_PCT(100));
    lv_obj_set_style_bg_opa(tzlist, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tzlist, 0, 0);
    lv_obj_set_style_pad_all(tzlist, 16, 0);
    lv_obj_set_flex_flow(tzlist, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(tzlist, 6, 0);
    lv_obj_set_scrollbar_mode(tzlist, LV_SCROLLBAR_MODE_ON);

    lv_obj_t* tz_hdr = make_label(tzlist, "Escolha seu fuso horário:", &nova_font_14, kColorTextDim);
    lv_obj_set_style_margin_bottom(tz_hdr, 8, 0);

    for (int i = 0; i < kTzCount; ++i) {
        lv_obj_t* row = lv_obj_create(tzlist);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 9, 0);
        lv_obj_set_style_pad_all(row, 12, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t* ti = make_col(row);
        lv_obj_set_style_flex_grow(ti, 1, 0);
        lv_obj_set_size(ti, 0, LV_SIZE_CONTENT);
        make_label(ti, kTimezoneList[i].name, &nova_font_14, 0xC4C8D6);
        lv_obj_t* toff = make_label(ti, kTimezoneList[i].offset, &nova_font_14, kColorTextMuted);
        lv_obj_set_style_margin_top(toff, 2, 0);

        lv_obj_t* chk = lv_obj_create(row);
        lv_obj_set_size(chk, 20, 20);
        lv_obj_set_style_bg_color(chk, lv_color_hex(kColorAccent), 0);
        lv_obj_set_style_border_width(chk, 0, 0);
        lv_obj_set_style_radius(chk, 10, 0);
        lv_obj_set_style_pad_all(chk, 0, 0);
        lv_obj_set_scrollbar_mode(chk, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t* chk_i = make_label(chk, LV_SYMBOL_OK, &nova_font_14, kColorDarkFg);
        lv_obj_align(chk_i, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(row, &WizardScreen::on_tz_row_clicked, LV_EVENT_CLICKED, this);
        tz_rows_[i] = row;
        tz_checks_[i] = chk;
    }

    // -- format side panel --
    lv_obj_t* fpanel = lv_obj_create(panel_tz_);
    lv_obj_set_size(fpanel, 258, LV_PCT(100));
    lv_obj_set_style_bg_color(fpanel, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(fpanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(fpanel, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(fpanel, lv_color_hex(kColorBorder), 0);
    lv_obj_set_style_border_width(fpanel, 1, 0);
    lv_obj_set_style_radius(fpanel, 0, 0);
    lv_obj_set_style_pad_all(fpanel, 24, 0);
    lv_obj_set_flex_flow(fpanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(fpanel, LV_SCROLLBAR_MODE_OFF);

    make_label(fpanel, "Formato de hora", &nova_font_14, kColorText);
    lv_obj_t* fmt_col = lv_obj_create(fpanel);
    clear_style(fmt_col);
    lv_obj_set_size(fmt_col, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(fmt_col, 12, 0);
    lv_obj_set_style_margin_bottom(fmt_col, 20, 0);
    lv_obj_set_flex_flow(fmt_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(fmt_col, 8, 0);

    static const char* kFmtNames[2] = {"24 horas", "12 horas"};
    static const char* kFmtExamples[2] = {"Ex: 14:30", "Ex: 2:30 PM"};
    for (int i = 0; i < kFormatCount; ++i) {
        lv_obj_t* fb = lv_obj_create(fmt_col);
        lv_obj_set_size(fb, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(fb, 1, 0);
        lv_obj_set_style_radius(fb, 9, 0);
        lv_obj_set_style_pad_all(fb, 12, 0);
        lv_obj_set_flex_flow(fb, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scrollbar_mode(fb, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t* fl = make_label(fb, kFmtNames[i], &nova_font_16, kColorText);
        lv_obj_set_style_margin_bottom(fl, 3, 0);
        make_label(fb, kFmtExamples[i], &nova_font_14, kColorTextMuted);
        lv_obj_add_event_cb(fb, &WizardScreen::on_format_clicked, LV_EVENT_CLICKED, this);
        format_boxes_[i] = fb;
        format_checks_[i] = fl;  // reuse: fl is the name label, recolored to show selection
    }

    make_hsep(fpanel);
    lv_obj_t* tz_sel_caption = make_label(fpanel, "FUSO SELECIONADO", &nova_font_14, kColorTextMuted);
    lv_obj_set_style_margin_top(tz_sel_caption, 16, 0);
    lv_obj_set_style_margin_bottom(tz_sel_caption, 8, 0);
    tz_selected_label_ = make_label(fpanel, kTimezoneList[0].name, &nova_font_14, kColorAccent);
}

void WizardScreen::build_confirmation_panel(lv_obj_t* body) {
    panel_confirm_ = lv_obj_create(body);
    lv_obj_set_size(panel_confirm_, LV_PCT(100), LV_PCT(100));
    clear_style(panel_confirm_);
    lv_obj_set_flex_flow(panel_confirm_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel_confirm_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(panel_confirm_, 60, 0);
    lv_obj_add_flag(panel_confirm_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* ok_badge = lv_obj_create(panel_confirm_);
    lv_obj_set_size(ok_badge, 72, 72);
    lv_obj_set_style_bg_color(ok_badge, lv_color_hex(kColorGreenBg), 0);
    lv_obj_set_style_border_color(ok_badge, lv_color_hex(kColorGreen), 0);
    lv_obj_set_style_border_width(ok_badge, 2, 0);
    lv_obj_set_style_radius(ok_badge, 36, 0);
    lv_obj_set_style_pad_all(ok_badge, 0, 0);
    lv_obj_set_style_margin_bottom(ok_badge, 14, 0);
    lv_obj_set_scrollbar_mode(ok_badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* ok_ic = make_label(ok_badge, LV_SYMBOL_OK, &nova_font_28, kColorGreen);
    lv_obj_align(ok_ic, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* done_title = make_label(panel_confirm_, "Tudo pronto!", &nova_font_28, kColorText);
    lv_obj_set_style_margin_bottom(done_title, 8, 0);

    lv_obj_t* done_sub = make_label(panel_confirm_,
        "Seu NovaPanel está configurado. Você\npode alterar dados depois em Configurações.",
        &nova_font_14, kColorTextDim);
    lv_obj_set_style_text_align(done_sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(done_sub, 20, 0);

    lv_obj_t* cr_wrap = lv_obj_create(panel_confirm_);
    lv_obj_set_size(cr_wrap, 360, LV_SIZE_CONTENT);
    clear_style(cr_wrap);
    lv_obj_set_flex_flow(cr_wrap, LV_FLEX_FLOW_COLUMN);

    confirm_name_label_ = make_confirm_row(cr_wrap, "Nome", "-", kColorText);
    confirm_wifi_label_ = make_confirm_row(cr_wrap, "Wi-Fi", "-", kColorGreen);
    confirm_tz_label_   = make_confirm_row(cr_wrap, "Fuso", "-", kColorText);
    confirm_fmt_label_  = make_confirm_row(cr_wrap, "Hora", "24 horas", kColorText);
}

void WizardScreen::show_keyboard_for(lv_obj_t* textarea) {
    if (!keyboard_) {
        keyboard_ = lv_keyboard_create(root_);
        lv_obj_add_event_cb(keyboard_, &WizardScreen::on_kb_ready, LV_EVENT_READY, this);
        // The keyboard's own "X" key sends LV_EVENT_CANCEL - LVGL does NOT
        // hide the widget for you on that event, the app has to (found on
        // real hardware: the X key did nothing, keyboard stayed stuck open).
        lv_obj_add_event_cb(keyboard_, &WizardScreen::on_kb_cancel, LV_EVENT_CANCEL, this);
    }
    lv_obj_clear_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);  // on_kb_cancel may have hidden it before
    lv_keyboard_set_textarea(keyboard_, textarea);
}

void WizardScreen::go_to_step(OnboardingStep step) {
    current_step_ = step;
    // Deliberately NOT auto-scanning here: the scan only ever runs when the
    // user taps "Atualizar lista" (on_wifi_rescan) - once fetched, results
    // stay cached/shown for the rest of the wizard session, even if the
    // user goes back and forward through this step again. Same policy
    // should apply later to a Settings screen that edits Wi-Fi.
    refresh_chrome();
}

void WizardScreen::refresh_chrome() {
    const int idx = step_index(current_step_);

    lv_bar_set_value(progress_bar_, kStepPct[idx], LV_ANIM_ON);

    char num[4];
    if (idx < 3) {
        num[0] = static_cast<char>('1' + idx);
        num[1] = '\0';
        lv_label_set_text(step_num_label_, num);
    } else {
        lv_label_set_text(step_num_label_, LV_SYMBOL_OK);
    }
    lv_label_set_text(step_title_label_, kStepTitles[idx]);
    lv_label_set_text(step_caption_label_, kStepCaptions[idx]);

    if (idx > 0) lv_obj_clear_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(next_label_, idx < 3 ? "Continuar ->" : "Iniciar NovaPanel ->");
    lv_obj_set_style_bg_color(next_btn_, lv_color_hex(idx == 3 ? kColorGreen : kColorAccent), 0);

    lv_obj_add_flag(panel_name_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(panel_wifi_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(panel_tz_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(panel_confirm_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* panels[4] = {panel_name_, panel_wifi_, panel_tz_, panel_confirm_};
    lv_obj_clear_flag(panels[idx], LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(status_label_, "");

    if (current_step_ == OnboardingStep::Confirmation) populate_confirmation();
}

void WizardScreen::refresh_wifi_network_list() {
    if (!wifi_list_) return;

    const bool show_status = wifi_scan_status_cache_ == WifiScanStatus::Idle ||
                             wifi_scan_status_cache_ == WifiScanStatus::Scanning ||
                             wifi_scan_status_cache_ == WifiScanStatus::Failed ||
                             wifi_networks_cache_.empty();
    if (show_status) {
        const char* msg = "Toque em 'Atualizar lista' para buscar redes.";
        if (wifi_scan_status_cache_ == WifiScanStatus::Scanning) msg = "Buscando redes...";
        else if (wifi_scan_status_cache_ == WifiScanStatus::Failed) msg = "Falha ao buscar redes.";
        else if (wifi_scan_status_cache_ == WifiScanStatus::Done && wifi_networks_cache_.empty())
            msg = "Nenhuma rede encontrada.";
        lv_label_set_text(wifi_status_text_, msg);
        lv_obj_clear_flag(wifi_status_text_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(wifi_status_text_, LV_OBJ_FLAG_HIDDEN);
    }

    wifi_row_count_ = static_cast<int>(
        wifi_networks_cache_.size() < static_cast<size_t>(kMaxWifiRows)
            ? wifi_networks_cache_.size() : static_cast<size_t>(kMaxWifiRows));

    for (int i = 0; i < kMaxWifiRows; ++i) {
        if (i >= wifi_row_count_) {
            lv_obj_add_flag(wifi_rows_[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const auto& net = wifi_networks_cache_[i];
        lv_obj_t* info = lv_obj_get_child(wifi_rows_[i], 1);
        lv_label_set_text(static_cast<lv_obj_t*>(lv_obj_get_child(info, 0)), net.ssid.c_str());
        lv_label_set_text(static_cast<lv_obj_t*>(lv_obj_get_child(info, 1)),
                          net.secured ? "Protegida" : "Aberta");
        lv_obj_clear_flag(wifi_rows_[i], LV_OBJ_FLAG_HIDDEN);
    }
    update_wifi_selection_ui();
}

void WizardScreen::update_wifi_selection_ui() {
    for (int i = 0; i < wifi_row_count_; ++i) {
        const bool sel = (wifi_networks_cache_[i].ssid == selected_ssid_);
        lv_obj_t* row = wifi_rows_[i];
        lv_obj_set_style_bg_color(row, lv_color_hex(sel ? kColorAccentBg : kColorCard), 0);
        lv_obj_set_style_border_color(row, lv_color_hex(sel ? kColorAccentBd : kColorBorder), 0);
        sel ? lv_obj_clear_flag(wifi_row_checks_[i], LV_OBJ_FLAG_HIDDEN)
            : lv_obj_add_flag(wifi_row_checks_[i], LV_OBJ_FLAG_HIDDEN);

        // Recolor the wifi icon + ssid text too (pointer traversal only,
        // no object creation - safe from the touch path, see header comment).
        lv_obj_t* ic_box = lv_obj_get_child(row, 0);
        lv_obj_t* ic = lv_obj_get_child(ic_box, 0);
        lv_obj_set_style_text_color(ic, lv_color_hex(sel ? kColorAccent : kColorTextDim), 0);
        lv_obj_t* info = lv_obj_get_child(row, 1);
        lv_obj_t* ssid_label = lv_obj_get_child(info, 0);
        lv_obj_set_style_text_color(ssid_label, lv_color_hex(sel ? kColorText : 0xC4C8D6), 0);
    }
}

void WizardScreen::populate_confirmation() {
    lv_label_set_text(confirm_name_label_, name_input_ && lv_textarea_get_text(name_input_)[0]
                                                ? lv_textarea_get_text(name_input_) : "-");
    lv_label_set_text(confirm_wifi_label_, selected_ssid_.empty() ? "-" : selected_ssid_.c_str());
    lv_label_set_text(confirm_tz_label_, kTimezoneList[selected_tz_index_].name);
    lv_label_set_text(confirm_fmt_label_, selected_format_24h_ ? "24 horas" : "12 horas");
}

void WizardScreen::render(const AppState& state) {
    if (!built_) build();
    if (lv_screen_active() != root_) {
        lv_screen_load(root_);
    }

    const auto& onboarding = state.onboarding;
    if (current_step_ != OnboardingStep::Wifi) return;

    if (onboarding.wifi_scan_status != shown_wifi_scan_status_ ||
        onboarding.wifi_networks.size() != shown_wifi_network_count_) {
        shown_wifi_scan_status_ = onboarding.wifi_scan_status;
        shown_wifi_network_count_ = onboarding.wifi_networks.size();
        wifi_scan_status_cache_ = onboarding.wifi_scan_status;
        wifi_networks_cache_ = onboarding.wifi_networks;
        refresh_wifi_network_list();
    }

    if (onboarding.wifi_status != shown_wifi_status_) {
        shown_wifi_status_ = onboarding.wifi_status;
        switch (onboarding.wifi_status) {
            case WifiConnectStatus::Connecting:
                lv_label_set_text(status_label_, "Conectando...");
                lv_obj_set_style_text_color(status_label_, lv_color_hex(kColorTextDim), 0);
                break;
            case WifiConnectStatus::Connected:
                lv_label_set_text(status_label_, "Conectado!");
                lv_obj_set_style_text_color(status_label_, lv_color_hex(kColorGreen), 0);
                go_to_step(OnboardingStep::TimezoneAndFormat);
                break;
            case WifiConnectStatus::Failed:
                lv_label_set_text(status_label_, "Falha ao conectar. Verifique a senha.");
                lv_obj_set_style_text_color(status_label_, lv_color_hex(kColorRed), 0);
                break;
            case WifiConnectStatus::Idle:
                lv_label_set_text(status_label_, "");
                break;
        }
    }
}

void WizardScreen::submit_current_step() {
    OnboardingSubmission s{};
    s.step = current_step_;

    switch (current_step_) {
        case OnboardingStep::DisplayName:
            s.display_name = name_input_ ? lv_textarea_get_text(name_input_) : "";
            if (on_submit_) on_submit_(s);
            go_to_step(OnboardingStep::Wifi);
            break;
        case OnboardingStep::Wifi:
            s.wifi_ssid = selected_ssid_;
            s.wifi_password = password_input_ ? lv_textarea_get_text(password_input_) : "";
            if (on_submit_) on_submit_(s);
            // Doesn't advance here - render() advances once really Connected.
            break;
        case OnboardingStep::TimezoneAndFormat:
            s.timezone = kTimezoneList[selected_tz_index_].name;
            s.time_format_24h = selected_format_24h_;
            if (on_submit_) on_submit_(s);
            go_to_step(OnboardingStep::Confirmation);
            break;
        case OnboardingStep::Confirmation:
            if (on_submit_) on_submit_(s);
            // app_main switches the screen to Home once AppState reflects Done.
            break;
        case OnboardingStep::Done:
            break;
    }
}

void WizardScreen::on_back_clicked(lv_event_t* e) {
    auto* self = static_cast<WizardScreen*>(lv_event_get_user_data(e));
    const int idx = step_index(self->current_step_);
    if (idx > 0) {
        self->current_step_ = step_at(idx - 1);
        self->refresh_chrome();
    }
}

void WizardScreen::on_next_clicked(lv_event_t* e) {
    static_cast<WizardScreen*>(lv_event_get_user_data(e))->submit_current_step();
}

void WizardScreen::on_wifi_network_clicked(lv_event_t* e) {
    auto* self = static_cast<WizardScreen*>(lv_event_get_user_data(e));
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    // Identify which pre-built row was tapped by pointer, then read the
    // ssid straight from the cache (no label text parsing needed) - O(n)
    // pointer compares + a style-only update, no object creation. See
    // ADR-0025: this used to call refresh_wifi_network_list() (full
    // clean+rebuild) here and overflowed taskLVGL's stack.
    for (int i = 0; i < self->wifi_row_count_; ++i) {
        if (self->wifi_rows_[i] != target) continue;
        self->selected_ssid_ = self->wifi_networks_cache_[i].ssid;
        lv_label_set_text(self->wifi_ssid_label_, self->selected_ssid_.c_str());
        self->update_wifi_selection_ui();
        break;
    }
}

void WizardScreen::on_wifi_rescan(lv_event_t* e) {
    auto* self = static_cast<WizardScreen*>(lv_event_get_user_data(e));
    self->wifi_scan_status_cache_ = WifiScanStatus::Idle;
    self->refresh_wifi_network_list();
    if (self->on_request_scan_) self->on_request_scan_();
}

void WizardScreen::on_tz_row_clicked(lv_event_t* e) {
    auto* self = static_cast<WizardScreen*>(lv_event_get_user_data(e));
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    for (int i = 0; i < kTzCount; ++i) {
        if (self->tz_rows_[i] == target) {
            self->selected_tz_index_ = i;
            self->update_tz_selection_ui();
            return;
        }
    }
}

void WizardScreen::update_tz_selection_ui() {
    for (int i = 0; i < kTzCount; ++i) {
        const bool sel = (i == selected_tz_index_);
        lv_obj_set_style_bg_color(tz_rows_[i], lv_color_hex(sel ? kColorAccentBg : kColorCard), 0);
        lv_obj_set_style_border_color(tz_rows_[i], lv_color_hex(sel ? kColorAccentBd : kColorBorder), 0);
        sel ? lv_obj_clear_flag(tz_checks_[i], LV_OBJ_FLAG_HIDDEN)
            : lv_obj_add_flag(tz_checks_[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(tz_selected_label_, kTimezoneList[selected_tz_index_].name);
}

void WizardScreen::on_format_clicked(lv_event_t* e) {
    auto* self = static_cast<WizardScreen*>(lv_event_get_user_data(e));
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    for (int i = 0; i < kFormatCount; ++i) {
        if (self->format_boxes_[i] == target) {
            self->selected_format_24h_ = (i == 0);
            self->update_format_selection_ui();
            return;
        }
    }
}

void WizardScreen::update_format_selection_ui() {
    for (int i = 0; i < kFormatCount; ++i) {
        const bool sel = (i == 0) == selected_format_24h_;
        lv_obj_set_style_bg_color(format_boxes_[i], lv_color_hex(sel ? kColorAccentBg : kColorCard2), 0);
        lv_obj_set_style_border_color(format_boxes_[i], lv_color_hex(sel ? kColorAccentBd : kColorBorder), 0);
        lv_obj_set_style_text_color(format_checks_[i], lv_color_hex(sel ? kColorAccent : kColorText), 0);
    }
}

void WizardScreen::on_textarea_focused(lv_event_t* e) {
    auto* self = static_cast<WizardScreen*>(lv_event_get_user_data(e));
    lv_obj_t* ta = static_cast<lv_obj_t*>(lv_event_get_target(e));
    self->show_keyboard_for(ta);
}

void WizardScreen::on_kb_ready(lv_event_t* e) {
    static_cast<WizardScreen*>(lv_event_get_user_data(e))->submit_current_step();
}

void WizardScreen::on_kb_cancel(lv_event_t* e) {
    auto* self = static_cast<WizardScreen*>(lv_event_get_user_data(e));
    lv_obj_add_flag(self->keyboard_, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace nova
