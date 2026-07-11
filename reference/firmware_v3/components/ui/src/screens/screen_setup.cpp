#include "novapanel_ui.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <string>

#include "common/nova_keyboard_manager.hpp"

namespace nova {

namespace {

struct TimezoneOption {
    const char* name;
    const char* label;
};

constexpr std::array<TimezoneOption, 5> kTimezoneOptions{{
    { "America/Sao_Paulo", "UTC-3  Brasilia" },
    { "America/Manaus", "UTC-4  Manaus" },
    { "America/Rio_Branco", "UTC-5  Rio Branco" },
    { "America/Noronha", "UTC-2  Noronha" },
    { "UTC", "UTC+0  Universal" },
}};

constexpr std::size_t kMaxWifiRowsRendered = 10;

static SetupSubmitFn g_submit_setup{};
static SetupScanFn g_scan_wifi{};
static SetupStepFn g_set_step{};
static NovaKeyboardManager* g_keyboard = nullptr;

static lv_obj_t* g_setup_root = nullptr;
static lv_obj_t* g_progress_bar = nullptr;
static lv_obj_t* g_step_badge = nullptr;
static lv_obj_t* g_step_title = nullptr;
static lv_obj_t* g_step_label = nullptr;
static lv_obj_t* g_step_body = nullptr;
static lv_obj_t* g_back_btn = nullptr;
static lv_obj_t* g_next_btn = nullptr;
static lv_obj_t* g_next_label = nullptr;
static lv_obj_t* g_step_panels[4] = { nullptr, nullptr, nullptr, nullptr };

static lv_obj_t* g_name_confirm = nullptr;
static lv_obj_t* g_wifi_confirm = nullptr;
static lv_obj_t* g_timezone_confirm = nullptr;
static lv_obj_t* g_format_confirm = nullptr;

static AppState g_setup_state{};
static bool g_setup_state_ready = false;
static OnboardingStep g_rendered_step = OnboardingStep::Done;
static bool g_setup_needs_render = false;
static bool g_wifi_scan_requested_for_step = false;
static WifiScanStatus g_rendered_wifi_scan_status = WifiScanStatus::Idle;
static WifiConnectStatus g_rendered_wifi_connect_status = WifiConnectStatus::Idle;
static std::size_t g_rendered_wifi_count = 0;
static std::string g_rendered_wifi_ssid{};
static std::string g_rendered_wifi_first_ssid{};
static std::string g_draft_name{};
static std::string g_draft_wifi_ssid{};
static std::string g_draft_wifi_password{};
static std::string g_draft_timezone{};
static bool g_draft_time_24h = true;

const char* step_title(OnboardingStep step)
{
    switch (step) {
        case OnboardingStep::DisplayName: return "Perfil";
        case OnboardingStep::Wifi: return "Wi-Fi";
        case OnboardingStep::TimezoneAndFormat: return "Fuso Horario";
        case OnboardingStep::Confirmation:
        case OnboardingStep::Done: return "Tudo certo!";
    }
    return "Setup";
}

const char* step_label(OnboardingStep step)
{
    switch (step) {
        case OnboardingStep::DisplayName: return "Passo 1 de 3";
        case OnboardingStep::Wifi: return "Passo 2 de 3";
        case OnboardingStep::TimezoneAndFormat: return "Passo 3 de 3";
        case OnboardingStep::Confirmation:
        case OnboardingStep::Done: return "";
    }
    return "";
}

int step_progress(OnboardingStep step)
{
    switch (step) {
        case OnboardingStep::DisplayName: return 12;
        case OnboardingStep::Wifi: return 42;
        case OnboardingStep::TimezoneAndFormat: return 74;
        case OnboardingStep::Confirmation:
        case OnboardingStep::Done: return 100;
    }
    return 0;
}

std::size_t panel_index_for(OnboardingStep step)
{
    switch (step) {
        case OnboardingStep::DisplayName: return 0;
        case OnboardingStep::Wifi: return 1;
        case OnboardingStep::TimezoneAndFormat: return 2;
        case OnboardingStep::Confirmation:
        case OnboardingStep::Done: return 3;
    }
    return 0;
}

void sync_drafts_from_state(const AppState& state)
{
    if (!g_setup_state_ready || g_draft_name.empty()) {
        g_draft_name = state.preferences.display_name;
    }
    if (!g_setup_state_ready || g_draft_timezone.empty()) {
        g_draft_timezone = state.preferences.timezone.empty()
            ? "America/Sao_Paulo"
            : state.preferences.timezone;
    }
    if (!g_setup_state_ready) {
        g_draft_time_24h = state.preferences.time_format_24h;
    }

    const auto& pending = state.onboarding.pending_submission;
    if (!pending.display_name.empty()) {
        g_draft_name = pending.display_name;
    }
    if (!pending.wifi_ssid.empty()) {
        g_draft_wifi_ssid = pending.wifi_ssid;
    }
    if (!pending.wifi_password.empty()) {
        g_draft_wifi_password = pending.wifi_password;
    }
    if (!pending.timezone.empty()) {
        g_draft_timezone = pending.timezone;
    }
    if (pending.step == OnboardingStep::TimezoneAndFormat) {
        g_draft_time_24h = pending.time_format_24h;
    }
    if (g_draft_timezone.empty()) {
        g_draft_timezone = "America/Sao_Paulo";
    }
}

void name_changed_cb(lv_event_t* e)
{
    auto* textarea = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const char* text = lv_textarea_get_text(textarea);
    g_draft_name = text ? text : "";
}

void wifi_password_changed_cb(lv_event_t* e)
{
    auto* textarea = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const char* text = lv_textarea_get_text(textarea);
    g_draft_wifi_password = text ? text : "";
}

void wifi_pick_cb(lv_event_t* e)
{
    const auto index = static_cast<std::size_t>(
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    if (index >= g_setup_state.onboarding.wifi_networks.size()) {
        return;
    }
    g_draft_wifi_ssid = g_setup_state.onboarding.wifi_networks[index].ssid;
    np_update_setup(g_setup_state);
}

void timezone_pick_cb(lv_event_t* e)
{
    const auto index = static_cast<std::size_t>(
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    if (index >= kTimezoneOptions.size()) {
        return;
    }
    g_draft_timezone = kTimezoneOptions[index].name;
    np_update_setup(g_setup_state);
}

void format_pick_cb(lv_event_t* e)
{
    g_draft_time_24h = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)) != 0;
    np_update_setup(g_setup_state);
}

void wifi_scan_cb(lv_event_t*)
{
    if (g_scan_wifi) {
        g_wifi_scan_requested_for_step = true;
        g_scan_wifi();
    }
}

void maybe_request_wifi_scan()
{
    const bool on_wifi_step = g_setup_state.onboarding.step == OnboardingStep::Wifi;
    if (!on_wifi_step) {
        g_wifi_scan_requested_for_step = false;
        return;
    }

    if (g_wifi_scan_requested_for_step || !g_scan_wifi) {
        return;
    }

    const bool needs_initial_scan =
        g_setup_state.onboarding.wifi_scan_status == WifiScanStatus::Idle &&
        g_setup_state.onboarding.wifi_networks.empty();
    if (!needs_initial_scan) {
        return;
    }

    g_wifi_scan_requested_for_step = true;
    g_scan_wifi();
}

void back_step_cb(lv_event_t*)
{
    if (!g_set_step) {
        return;
    }
    switch (g_setup_state.onboarding.step) {
        case OnboardingStep::Wifi:
            g_set_step(OnboardingStep::DisplayName);
            break;
        case OnboardingStep::TimezoneAndFormat:
            g_set_step(OnboardingStep::Wifi);
            break;
        case OnboardingStep::Confirmation:
        case OnboardingStep::Done:
            g_set_step(OnboardingStep::TimezoneAndFormat);
            break;
        case OnboardingStep::DisplayName:
            break;
    }
}

void next_step_cb(lv_event_t*)
{
    if (!g_submit_setup) {
        return;
    }

    OnboardingSubmission submission{};
    submission.step = g_setup_state.onboarding.step;
    submission.display_name = g_draft_name;
    submission.wifi_ssid = g_draft_wifi_ssid;
    submission.wifi_password = g_draft_wifi_password;
    submission.timezone = g_draft_timezone;
    submission.time_format_24h = g_draft_time_24h;
    g_submit_setup(submission);
}

lv_obj_t* build_confirm_row(lv_obj_t* parent, const char* key, const char* value, lv_color_t color)
{
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row, NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_all(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(row, 8, 0);
    np_label(row, key, NP_F_SM, NP_C_TEXT_DIM);
    return np_label(row, value, NP_F_SM, color);
}

void render_name_panel(lv_obj_t* panel)
{
    lv_obj_clean(panel);
    lv_obj_set_style_pad_hor(panel, 84, 0);
    lv_obj_set_style_pad_row(panel, 0, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* badge = lv_obj_create(panel);
    lv_obj_set_size(badge, 76, 76);
    lv_obj_set_style_bg_color(badge, NP_C_ACCENT_BG, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, NP_C_ACCENT, 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_radius(badge, 38, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_set_style_margin_bottom(badge, 22, 0);
    lv_obj_t* badge_text = np_label(badge, "?", NP_F_3XL, NP_C_ACCENT);
    lv_obj_align(badge_text, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* title = np_label(panel, "Como quer ser chamado?", NP_F_TITLE_MD, NP_C_TEXT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(title, 8, 0);

    lv_obj_t* subtitle = np_label(panel,
        "Seu nome aparecera na saudacao e no perfil do dispositivo.",
        NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(subtitle, 28, 0);

    lv_obj_t* textarea = lv_textarea_create(panel);
    lv_obj_set_width(textarea, 360);
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_placeholder_text(textarea, "Seu nome");
    lv_textarea_set_text(textarea, g_draft_name.c_str());
    lv_obj_set_style_bg_color(textarea, NP_C_CARD, 0);
    lv_obj_set_style_border_color(textarea, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(textarea, 1, 0);
    lv_obj_set_style_radius(textarea, 12, 0);
    lv_obj_set_style_text_font(textarea, NP_F_XL, 0);
    lv_obj_set_style_text_color(textarea, NP_C_TEXT, 0);
    lv_obj_set_style_text_align(textarea, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_event_cb(textarea, name_changed_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    if (g_keyboard) {
        g_keyboard->attach(textarea, panel);
    }
}

void render_wifi_row(lv_obj_t* parent, const WifiNetwork& network, std::size_t index)
{
    const bool selected = network.ssid == g_draft_wifi_ssid;

    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row, selected ? NP_C_ACCENT_BG : NP_C_CARD, 0);
    lv_obj_set_style_border_color(row, selected ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_all(row, 13, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_add_event_cb(row, wifi_pick_cb, LV_EVENT_CLICKED,
        reinterpret_cast<void*>(index));

    lv_obj_t* icon_box = lv_obj_create(row);
    lv_obj_set_size(icon_box, 38, 38);
    lv_obj_set_style_bg_color(icon_box, NP_C_CARD2, 0);
    lv_obj_set_style_border_width(icon_box, 0, 0);
    lv_obj_set_style_radius(icon_box, 9, 0);
    lv_obj_set_style_pad_all(icon_box, 0, 0);
    lv_obj_t* icon = np_label(icon_box, NP_I_WIFI, NP_F_ICON,
        selected ? NP_C_ACCENT : NP_C_TEXT_DIM);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* info = np_col(row);
    lv_obj_set_style_flex_grow(info, 1, 0);
    lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
    np_label(info, network.ssid.c_str(), NP_F_MD, NP_C_TEXT);

    char detail[48];
    std::snprintf(detail, sizeof(detail), "%s  %d dBm",
                  network.secured ? "Protegida" : "Aberta",
                  static_cast<int>(network.rssi));
    lv_obj_t* meta = np_label(info, detail, NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(meta, 2, 0);

    lv_obj_t* check = lv_obj_create(row);
    lv_obj_set_size(check, 22, 22);
    lv_obj_set_style_bg_color(check, NP_C_ACCENT, 0);
    lv_obj_set_style_border_width(check, 0, 0);
    lv_obj_set_style_radius(check, 11, 0);
    lv_obj_set_style_pad_all(check, 0, 0);
    lv_obj_t* ok = np_label(check, NP_I_CHECK, NP_F_ICON_XS, NP_C_DARK_FG);
    lv_obj_align(ok, LV_ALIGN_CENTER, 0, 0);
    if (!selected) {
        lv_obj_add_flag(check, LV_OBJ_FLAG_HIDDEN);
    }
}

void render_wifi_panel(lv_obj_t* panel)
{
    lv_obj_clean(panel);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(panel, 0, 0);

    lv_obj_t* list = lv_obj_create(panel);
    lv_obj_set_style_flex_grow(list, 1, 0);
    lv_obj_set_height(list, lv_pct(100));
    np_obj_clear_style(list);
    lv_obj_set_style_pad_all(list, 16, 0);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);

    np_label(list, "Selecione a rede Wi-Fi do seu ambiente:", NP_F_SM, NP_C_TEXT_DIM);

    lv_obj_t* actions = np_row(list);
    lv_obj_set_style_pad_column(actions, 8, 0);
    lv_obj_t* refresh = np_chip(actions, "Atualizar lista");
    lv_obj_add_event_cb(refresh, wifi_scan_cb, LV_EVENT_CLICKED, nullptr);

    const auto scan_status = g_setup_state.onboarding.wifi_scan_status;
    if (scan_status == WifiScanStatus::Scanning) {
        np_label(actions, "Buscando redes...", NP_F_XS, NP_C_ACCENT);
    } else if (scan_status == WifiScanStatus::Failed) {
        np_label(actions, "Scan indisponivel no runtime atual.", NP_F_XS, NP_C_RED);
    } else if (scan_status == WifiScanStatus::Done &&
               g_setup_state.onboarding.wifi_networks.empty()) {
        np_label(actions, "Nenhuma rede encontrada.", NP_F_XS, NP_C_TEXT_DIM);
    }

    const std::size_t network_count = g_setup_state.onboarding.wifi_networks.size();
    const std::size_t rendered_count =
        network_count > kMaxWifiRowsRendered ? kMaxWifiRowsRendered : network_count;
    for (std::size_t i = 0; i < rendered_count; ++i) {
        render_wifi_row(list, g_setup_state.onboarding.wifi_networks[i], i);
    }
    if (network_count > rendered_count) {
        char more[64];
        std::snprintf(more, sizeof(more), "+%u redes ocultas para manter a UI fluida",
                      static_cast<unsigned>(network_count - rendered_count));
        np_label(list, more, NP_F_XS, NP_C_TEXT_MUTED);
    }

    lv_obj_t* side = lv_obj_create(panel);
    lv_obj_set_size(side, 270, lv_pct(100));
    lv_obj_set_style_bg_color(side, NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(side, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(side, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(side, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(side, 1, 0);
    lv_obj_set_style_radius(side, 0, 0);
    lv_obj_set_style_pad_all(side, 24, 0);
    lv_obj_set_flex_flow(side, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(side,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    np_label(side, "REDE SELECIONADA", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_t* ssid = np_label(side,
        g_draft_wifi_ssid.empty() ? "Escolha uma rede" : g_draft_wifi_ssid.c_str(),
        NP_F_LG, NP_C_TEXT);
    lv_obj_set_style_margin_top(ssid, 8, 0);
    lv_obj_set_style_margin_bottom(ssid, 20, 0);

    np_label(side, "Senha", NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_t* textarea = lv_textarea_create(side);
    lv_obj_set_width(textarea, lv_pct(100));
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_password_mode(textarea, true);
    lv_textarea_set_placeholder_text(textarea, "Senha da rede");
    lv_textarea_set_text(textarea, g_draft_wifi_password.c_str());
    lv_obj_set_style_bg_color(textarea, NP_C_CARD2, 0);
    lv_obj_set_style_border_color(textarea, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(textarea, 1, 0);
    lv_obj_set_style_radius(textarea, 9, 0);
    lv_obj_set_style_text_font(textarea, NP_F_LG, 0);
    lv_obj_set_style_text_color(textarea, NP_C_TEXT, 0);
    lv_obj_set_style_margin_top(textarea, 8, 0);
    lv_obj_set_style_margin_bottom(textarea, 14, 0);
    lv_obj_add_event_cb(textarea, wifi_password_changed_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    if (g_keyboard) {
        g_keyboard->attach(textarea, side);
    }

    np_label(side, "Deixe em branco para redes abertas.", NP_F_XS, NP_C_TEXT_MUTED);

    if (g_setup_state.onboarding.wifi_status == WifiConnectStatus::Failed) {
        lv_obj_t* err = np_label(side, "Falha ao conectar. Revise os dados.",
                                 NP_F_XS, NP_C_RED);
        lv_obj_set_style_margin_top(err, 12, 0);
    } else if (g_setup_state.onboarding.wifi_status == WifiConnectStatus::Connecting) {
        lv_obj_t* waiting = np_label(side, "Conectando...", NP_F_XS, NP_C_ACCENT);
        lv_obj_set_style_margin_top(waiting, 12, 0);
    }
}

bool wifi_render_signature_changed()
{
    const auto& onboarding = g_setup_state.onboarding;
    const std::string first_ssid = onboarding.wifi_networks.empty()
        ? std::string{}
        : onboarding.wifi_networks.front().ssid;
    return g_rendered_wifi_scan_status != onboarding.wifi_scan_status ||
        g_rendered_wifi_connect_status != onboarding.wifi_status ||
        g_rendered_wifi_count != onboarding.wifi_networks.size() ||
        g_rendered_wifi_ssid != g_draft_wifi_ssid ||
        g_rendered_wifi_first_ssid != first_ssid;
}

void render_timezone_option(lv_obj_t* parent, const TimezoneOption& option, std::size_t index)
{
    const bool selected = g_draft_timezone == option.name;
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row, selected ? NP_C_ACCENT_BG : NP_C_CARD, 0);
    lv_obj_set_style_border_color(row, selected ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 9, 0);
    lv_obj_set_style_pad_all(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(row, timezone_pick_cb, LV_EVENT_CLICKED,
        reinterpret_cast<void*>(index));

    lv_obj_t* text = np_col(row);
    lv_obj_set_style_flex_grow(text, 1, 0);
    lv_obj_set_size(text, 0, LV_SIZE_CONTENT);
    np_label(text, option.name, NP_F_MD, NP_C_TEXT);
    lv_obj_t* meta = np_label(text, option.label, NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(meta, 2, 0);

    lv_obj_t* check = lv_obj_create(row);
    lv_obj_set_size(check, 20, 20);
    lv_obj_set_style_bg_color(check, NP_C_ACCENT, 0);
    lv_obj_set_style_border_width(check, 0, 0);
    lv_obj_set_style_radius(check, 10, 0);
    lv_obj_set_style_pad_all(check, 0, 0);
    lv_obj_t* ok = np_label(check, NP_I_CHECK, NP_F_ICON_XS, NP_C_DARK_FG);
    lv_obj_align(ok, LV_ALIGN_CENTER, 0, 0);
    if (!selected) {
        lv_obj_add_flag(check, LV_OBJ_FLAG_HIDDEN);
    }
}

void render_format_option(lv_obj_t* parent, const char* title, const char* example, bool active, bool is_24h)
{
    lv_obj_t* box = lv_obj_create(parent);
    lv_obj_set_size(box, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(box, active ? NP_C_ACCENT_BG : NP_C_CARD2, 0);
    lv_obj_set_style_border_color(box, active ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, 9, 0);
    lv_obj_set_style_pad_all(box, 12, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_margin_bottom(box, 8, 0);
    lv_obj_add_event_cb(box, format_pick_cb, LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<uintptr_t>(is_24h ? 1 : 0)));

    lv_obj_t* label = np_label(box, title, NP_F_TITLE_SM, active ? NP_C_ACCENT : NP_C_TEXT);
    lv_obj_set_style_margin_bottom(label, 3, 0);
    np_label(box, example, NP_F_XS, NP_C_TEXT_MUTED);
}

void render_timezone_panel(lv_obj_t* panel)
{
    lv_obj_clean(panel);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* list = lv_obj_create(panel);
    lv_obj_set_style_flex_grow(list, 1, 0);
    lv_obj_set_height(list, lv_pct(100));
    np_obj_clear_style(list);
    lv_obj_set_style_pad_all(list, 16, 0);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);

    np_label(list, "Escolha seu fuso horario:", NP_F_SM, NP_C_TEXT_DIM);
    for (std::size_t i = 0; i < kTimezoneOptions.size(); ++i) {
        render_timezone_option(list, kTimezoneOptions[i], i);
    }

    lv_obj_t* side = lv_obj_create(panel);
    lv_obj_set_size(side, 270, lv_pct(100));
    lv_obj_set_style_bg_color(side, NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(side, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(side, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(side, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(side, 1, 0);
    lv_obj_set_style_radius(side, 0, 0);
    lv_obj_set_style_pad_all(side, 24, 0);
    lv_obj_set_flex_flow(side, LV_FLEX_FLOW_COLUMN);

    np_label(side, "Formato de hora", NP_F_MD, NP_C_TEXT);
    lv_obj_t* formats = lv_obj_create(side);
    np_obj_clear_style(formats);
    lv_obj_set_size(formats, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(formats, 12, 0);
    lv_obj_set_style_margin_bottom(formats, 18, 0);
    lv_obj_set_flex_flow(formats, LV_FLEX_FLOW_COLUMN);

    render_format_option(formats, "24 horas", "Ex: 14:30", g_draft_time_24h, true);
    render_format_option(formats, "12 horas", "Ex: 2:30 PM", !g_draft_time_24h, false);

    np_hsep(side);
    lv_obj_t* hint = np_label(side, "FUSO SELECIONADO", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(hint, 16, 0);
    lv_obj_set_style_margin_bottom(hint, 8, 0);
    np_label(side, g_draft_timezone.c_str(), NP_F_SM, NP_C_ACCENT);
}

void render_confirmation_panel(lv_obj_t* panel)
{
    lv_obj_clean(panel);
    lv_obj_set_style_pad_hor(panel, 60, 0);
    lv_obj_set_style_pad_row(panel, 0, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* title_row = lv_obj_create(panel);
    np_obj_clear_style(title_row);
    lv_obj_set_size(title_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(title_row, 12, 0);
    lv_obj_set_style_margin_bottom(title_row, 8, 0);

    lv_obj_t* badge = lv_obj_create(title_row);
    lv_obj_set_size(badge, 46, 46);
    lv_obj_set_style_bg_color(badge, NP_C_GREEN_BG, 0);
    lv_obj_set_style_border_color(badge, NP_C_GREEN, 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_radius(badge, 23, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_t* ok = np_label(badge, NP_I_CHECK, NP_F_ICON_SM, NP_C_GREEN);
    lv_obj_align(ok, LV_ALIGN_CENTER, 0, 0);

    np_label(title_row, "Tudo pronto!", NP_F_TITLE_MD, NP_C_TEXT);

    lv_obj_t* subtitle = np_label(panel,
        "Seu NovaPanel esta configurado e pronto para iniciar.",
        NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(subtitle, 16, 0);

    lv_obj_t* wrap = lv_obj_create(panel);
    np_obj_clear_style(wrap);
    lv_obj_set_size(wrap, 380, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_COLUMN);

    g_name_confirm = build_confirm_row(wrap, "Nome",
        g_draft_name.empty() ? "-" : g_draft_name.c_str(), NP_C_TEXT);
    g_wifi_confirm = build_confirm_row(wrap, "Wi-Fi",
        g_draft_wifi_ssid.empty() ? "-" : g_draft_wifi_ssid.c_str(), NP_C_GREEN);
    g_timezone_confirm = build_confirm_row(wrap, "Fuso",
        g_draft_timezone.c_str(), NP_C_TEXT);
    g_format_confirm = build_confirm_row(wrap, "Hora",
        g_draft_time_24h ? "24 horas" : "12 horas", NP_C_TEXT);
}

void render_current_panel()
{
    if (!g_step_body) {
        return;
    }
    const OnboardingStep step = g_setup_state.onboarding.step;
    const std::size_t active_panel = panel_index_for(step);

    switch (active_panel) {
        case 0:
            render_name_panel(g_step_panels[0]);
            break;
        case 1:
            render_wifi_panel(g_step_panels[1]);
            g_rendered_wifi_scan_status = g_setup_state.onboarding.wifi_scan_status;
            g_rendered_wifi_connect_status = g_setup_state.onboarding.wifi_status;
            g_rendered_wifi_count = g_setup_state.onboarding.wifi_networks.size();
            g_rendered_wifi_ssid = g_draft_wifi_ssid;
            g_rendered_wifi_first_ssid = g_setup_state.onboarding.wifi_networks.empty()
                ? std::string{}
                : g_setup_state.onboarding.wifi_networks.front().ssid;
            break;
        case 2:
            render_timezone_panel(g_step_panels[2]);
            break;
        case 3:
            render_confirmation_panel(g_step_panels[3]);
            break;
        default:
            break;
    }
    g_rendered_step = step;
}

void refresh_chrome()
{
    if (!g_progress_bar) {
        return;
    }

    const OnboardingStep step = g_setup_state.onboarding.step;
    lv_bar_set_value(g_progress_bar, step_progress(step), LV_ANIM_OFF);
    lv_label_set_text(g_step_title, step_title(step));
    lv_label_set_text(g_step_label, step_label(step));

    char badge[4] = "1";
    if (step == OnboardingStep::DisplayName) {
        std::strcpy(badge, "1");
    } else if (step == OnboardingStep::Wifi) {
        std::strcpy(badge, "2");
    } else if (step == OnboardingStep::TimezoneAndFormat) {
        std::strcpy(badge, "3");
    } else {
        std::strcpy(badge, "OK");
    }
    lv_label_set_text(g_step_badge, badge);

    const std::size_t active_panel = panel_index_for(step);
    for (std::size_t i = 0; i < 4; ++i) {
        if (!g_step_panels[i]) {
            continue;
        }
        if (i == active_panel) {
            lv_obj_clear_flag(g_step_panels[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(g_step_panels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (step == OnboardingStep::DisplayName) {
        lv_obj_add_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    }

    const bool confirmation = step == OnboardingStep::Confirmation || step == OnboardingStep::Done;
    lv_label_set_text(g_next_label, confirmation ? "Iniciar NovaPanel ->" : "Continuar ->");
    lv_obj_set_style_bg_color(g_next_btn, confirmation ? NP_C_GREEN : NP_C_ACCENT, 0);
}

}  // namespace

void np_bind_setup_submit(SetupSubmitFn fn)
{
    g_submit_setup = std::move(fn);
}

void np_bind_setup_scan(SetupScanFn fn)
{
    g_scan_wifi = std::move(fn);
}

void np_bind_setup_step(SetupStepFn fn)
{
    g_set_step = std::move(fn);
}

void np_update_setup(const AppState& state)
{
    g_setup_state = state;
    sync_drafts_from_state(state);
    g_setup_state_ready = true;
    g_setup_needs_render = true;
    maybe_request_wifi_scan();
}

void np_tick_setup()
{
    if (!g_setup_root || !g_setup_needs_render) {
        return;
    }

    maybe_request_wifi_scan();

    const bool should_rebuild =
        g_rendered_step != g_setup_state.onboarding.step ||
        (g_setup_state.onboarding.step == OnboardingStep::Wifi &&
         wifi_render_signature_changed());
    if (should_rebuild) {
        render_current_panel();
    }
    refresh_chrome();
    g_setup_needs_render = false;
}

void np_screen_setup(lv_obj_t* parent)
{
    const auto index = static_cast<std::size_t>(ScreenId::Setup);

    if (!g_keyboard) {
        g_keyboard = new NovaKeyboardManager();
        g_keyboard->init();
    }

    lv_obj_t* scr = lv_obj_create(parent);
    lv_obj_set_size(scr, NP_W, NP_H);
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_style_bg_color(scr, NP_C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_radius(scr, 12, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    np_screens[index] = scr;
    g_setup_root = scr;

    lv_obj_t* track = lv_obj_create(scr);
    lv_obj_set_size(track, lv_pct(100), 3);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(track, 0, 0);
    lv_obj_set_style_radius(track, 0, 0);
    lv_obj_set_style_pad_all(track, 0, 0);

    g_progress_bar = lv_bar_create(track);
    lv_obj_set_size(g_progress_bar, lv_pct(100), 3);
    lv_bar_set_range(g_progress_bar, 0, 100);
    lv_obj_set_style_bg_color(g_progress_bar, lv_color_hex(0x1A1D2C), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_progress_bar, NP_C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_progress_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_progress_bar, 0, LV_PART_INDICATOR);

    lv_obj_t* header = lv_obj_create(scr);
    np_obj_clear_style(header);
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(header, 16, 0);
    lv_obj_set_style_pad_bottom(header, 0, 0);
    lv_obj_set_style_pad_column(header, 10, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* badge = lv_obj_create(header);
    lv_obj_set_size(badge, 30, 30);
    lv_obj_set_style_bg_color(badge, NP_C_ACCENT_BG, 0);
    lv_obj_set_style_border_color(badge, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_radius(badge, 15, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    g_step_badge = lv_label_create(badge);
    lv_obj_set_style_text_font(g_step_badge, NP_F_SM, 0);
    lv_obj_set_style_text_color(g_step_badge, NP_C_ACCENT, 0);
    lv_obj_align(g_step_badge, LV_ALIGN_CENTER, 0, 0);

    g_step_title = np_label(header, "Perfil", NP_F_LG, NP_C_ACCENT);
    lv_obj_t* spacer = lv_obj_create(header);
    np_obj_clear_style(spacer);
    lv_obj_set_style_flex_grow(spacer, 1, 0);
    g_step_label = np_label(header, "Passo 1 de 3", NP_F_XS, NP_C_TEXT_MUTED);

    g_step_body = lv_obj_create(scr);
    np_obj_clear_style(g_step_body);
    lv_obj_set_size(g_step_body, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(g_step_body, 1, 0);

    for (auto& panel : g_step_panels) {
        panel = lv_obj_create(g_step_body);
        np_obj_clear_style(panel);
        lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    }

    lv_obj_t* footer = lv_obj_create(scr);
    lv_obj_set_size(footer, lv_pct(100), 62);
    lv_obj_set_style_bg_color(footer, NP_C_BG, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 14, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, 12, 0);

    g_back_btn = lv_button_create(footer);
    lv_obj_set_size(g_back_btn, LV_SIZE_CONTENT, 46);
    lv_obj_set_style_bg_color(g_back_btn, NP_C_CARD2, 0);
    lv_obj_set_style_border_color(g_back_btn, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(g_back_btn, 1, 0);
    lv_obj_set_style_radius(g_back_btn, NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(g_back_btn, 0, 0);
    lv_obj_set_style_pad_hor(g_back_btn, 22, 0);
    lv_obj_t* back_text = np_label(g_back_btn, "<- Voltar", NP_F_MD, NP_C_TEXT_DIM);
    lv_obj_align(back_text, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_back_btn, back_step_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* footer_spacer = lv_obj_create(footer);
    np_obj_clear_style(footer_spacer);
    lv_obj_set_style_flex_grow(footer_spacer, 1, 0);

    g_next_btn = lv_button_create(footer);
    lv_obj_set_size(g_next_btn, LV_SIZE_CONTENT, 46);
    lv_obj_set_style_bg_color(g_next_btn, NP_C_ACCENT, 0);
    lv_obj_set_style_radius(g_next_btn, NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(g_next_btn, 0, 0);
    lv_obj_set_style_pad_hor(g_next_btn, 24, 0);
    g_next_label = np_label(g_next_btn, "Continuar ->", NP_F_MD, NP_C_DARK_FG);
    lv_obj_align(g_next_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_next_btn, next_step_cb, LV_EVENT_CLICKED, nullptr);

    g_setup_needs_render = true;
}

}  // namespace nova
