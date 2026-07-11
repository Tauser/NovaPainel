#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>

namespace nova {

namespace {

static lv_obj_t* g_settings_profile_initials = nullptr;
static lv_obj_t* g_settings_profile_name = nullptr;
static lv_obj_t* g_settings_profile_subtitle = nullptr;
static lv_obj_t* g_settings_wifi_name = nullptr;
static lv_obj_t* g_settings_wifi_meta = nullptr;
static lv_obj_t* g_settings_bt_status = nullptr;
static lv_obj_t* g_settings_timezone_name = nullptr;
static lv_obj_t* g_settings_timezone_meta = nullptr;
static lv_obj_t* g_settings_night_subtitle = nullptr;
static lv_obj_t* g_settings_time_format_24h = nullptr;
static lv_obj_t* g_settings_time_format_12h = nullptr;
static lv_obj_t* g_settings_sys_value[7] = { nullptr };
static lv_obj_t* g_settings_night_toggle = nullptr;
static lv_obj_t* g_settings_lat_value = nullptr;
static lv_obj_t* g_settings_lon_value = nullptr;
static lv_obj_t* g_brightness_slider = nullptr;
static lv_obj_t* g_brightness_label  = nullptr;
static lv_obj_t* g_volume_slider     = nullptr;
static lv_obj_t* g_volume_label      = nullptr;
// Trivial statics — constant-initialized, sem custo em __init_array.
// std::function como GLOBAIS não-triviais causam travamento de boot no ESP-IDF.
static bool      g_prefs_time24h = true;
static ThemeMode g_prefs_theme   = ThemeMode::Auto;
// Ponteiros para std::function alocados só em app_main via np_bind_settings_*.
static SettingsTimeFormatFn*  g_time_format_fn  = nullptr;
static SettingsThemeFn*       g_theme_fn        = nullptr;
static SettingsRebootFn*      g_reboot_fn       = nullptr;
static SettingsBrightnessFn*        g_brightness_fn         = nullptr;
static SettingsBrightnessPreviewFn* g_brightness_preview_fn = nullptr;
static SettingsVolumeFn*            g_volume_fn             = nullptr;
static SettingsNotifyFn*      g_notify_fn       = nullptr;

const char* clock_source_label(const AppState& state)
{
    return state.clock.synced ? "NTP sincronizado" : "Relogio local";
}

void set_line_if_changed(lv_obj_t* label, const char* text)
{
    if (!label) {
        return;
    }

    const char* current = lv_label_get_text(label);
    if (!current || std::strcmp(current, text) != 0) {
        lv_label_set_text(label, text);
    }
}

void set_color_if_changed(lv_obj_t* label, lv_color_t color)
{
    if (!label) {
        return;
    }
    const lv_color_t current = lv_obj_get_style_text_color(label, LV_PART_MAIN);
    if (current.red != color.red || current.green != color.green || current.blue != color.blue) {
        lv_obj_set_style_text_color(label, color, 0);
    }
}

void make_initials(const std::string& name, char* out, size_t out_size)
{
    if (out_size < 3) {
        return;
    }

    out[0] = 'N';
    out[1] = 'P';
    out[2] = '\0';

    if (name.empty()) {
        return;
    }

    size_t write = 0;
    bool take_next = true;
    for (char ch : name) {
        if (ch == ' ' || ch == '\t') {
            take_next = true;
            continue;
        }
        if (take_next) {
            if (ch >= 'a' && ch <= 'z') {
                ch = static_cast<char>(ch - ('a' - 'A'));
            }
            out[write++] = ch;
            if (write == 2) {
                break;
            }
            take_next = false;
        }
    }
    out[write] = '\0';
    if (write == 1) {
        out[1] = '\0';
    }
}

lv_obj_t* slider_row(lv_obj_t* parent, const char* title, int32_t val)
{
    lv_obj_t* row = np_row(parent);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(row, 6, 0);
    np_label(row, title, NP_F_SM, NP_C_TEXT_DIM);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%ld%%", static_cast<long>(val));
    lv_obj_t* value = np_label(row, buf, NP_F_LG, NP_C_ACCENT);

    lv_obj_t* slider = lv_slider_create(parent);
    lv_obj_set_size(slider, lv_pct(100), 6);
    lv_obj_set_style_margin_bottom(slider, 14, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, NP_C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, NP_C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, NP_C_ACCENT, LV_PART_KNOB);
    lv_obj_set_style_radius(slider, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 3, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 6, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 4, LV_PART_KNOB);
    lv_obj_add_state(slider, LV_STATE_DISABLED);
    lv_obj_set_style_opa(slider, LV_OPA_70, 0);
    return value;
}

// Como slider_row mas retorna o slider (interativo, sem DISABLED).
// value_label_out recebe o label de porcentagem para atualização posterior.
lv_obj_t* interactive_slider_row(lv_obj_t* parent, const char* title, int32_t val,
                                  lv_obj_t** value_label_out)
{
    lv_obj_t* row = np_row(parent);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(row, 6, 0);
    np_label(row, title, NP_F_SM, NP_C_TEXT_DIM);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%ld%%", static_cast<long>(val));
    lv_obj_t* value = np_label(row, buf, NP_F_LG, NP_C_ACCENT);
    if (value_label_out) *value_label_out = value;

    lv_obj_t* slider = lv_slider_create(parent);
    lv_obj_set_size(slider, lv_pct(100), 6);
    lv_obj_set_style_margin_bottom(slider, 14, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, NP_C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, NP_C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, NP_C_ACCENT, LV_PART_KNOB);
    lv_obj_set_style_radius(slider, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 3, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 6, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 4, LV_PART_KNOB);
    return slider;
}

void style_format_button(lv_obj_t* button, bool active)
{
    lv_obj_set_style_bg_color(button, active ? lv_color_hex(0x252A3C)
                                             : lv_color_hex(0x1A1D2C), 0);
    lv_obj_t* label = lv_obj_get_child(button, 0);
    if (label) {
        lv_obj_set_style_text_color(label, active ? NP_C_TEXT : NP_C_TEXT_MUTED, 0);
    }
}

// --- Callbacks de interação ---

static void on_time_format_cb(lv_event_t* e)
{
    const bool fmt24 = reinterpret_cast<intptr_t>(lv_event_get_user_data(e)) != 0;
    if (g_prefs_time24h == fmt24) return;
    g_prefs_time24h = fmt24;
    if (g_time_format_fn) (*g_time_format_fn)(fmt24);
    if (g_notify_fn) (*g_notify_fn)(fmt24 ? "Formato 24h salvo" : "Formato 12h salvo");
}

static void on_night_toggle_cb(lv_event_t* e)
{
    lv_obj_t* toggle = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const bool dark = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    g_prefs_theme = dark ? ThemeMode::Dark : ThemeMode::Auto;
    if (g_theme_fn) (*g_theme_fn)(g_prefs_theme);
    if (g_notify_fn) (*g_notify_fn)(dark ? "Modo escuro ativado" : "Modo claro ativado");
}

static void on_reboot_cb(lv_event_t*)
{
    if (g_reboot_fn) (*g_reboot_fn)();
}

static void on_brightness_cb(lv_event_t* e)
{
    // VALUE_CHANGED (drag): atualiza label + aplica PWM via preview.
    // NÃO chama g_brightness_fn aqui — isso evita NVS write a cada pixel
    // de drag, que causava CPU stall → DSI underrun → flash azul.
    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const int32_t val = lv_slider_get_value(slider);
    if (g_brightness_label) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%ld%%", static_cast<long>(val));
        lv_label_set_text(g_brightness_label, buf);
    }
    if (g_brightness_preview_fn) (*g_brightness_preview_fn)(static_cast<int>(val));
}

static void on_brightness_released_cb(lv_event_t* e)
{
    // RELEASED: único ponto que salva nas prefs + NVS.
    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const int32_t val = lv_slider_get_value(slider);
    if (g_brightness_fn) (*g_brightness_fn)(static_cast<int>(val));
    if (g_notify_fn) {
        char msg[32];
        std::snprintf(msg, sizeof(msg), "Brilho: %ld%%", static_cast<long>(val));
        (*g_notify_fn)(msg);
    }
}

// VALUE_CHANGED (todo pixel de drag): só atualiza o label, sem áudio.
// Chamar g_volume_fn aqui causaria esp_codec_dev_write (I2S) a cada pixel.
static void on_volume_cb(lv_event_t* e)
{
    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const int32_t val = lv_slider_get_value(slider);
    if (g_volume_label) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%ld%%", static_cast<long>(val));
        lv_label_set_text(g_volume_label, buf);
    }
}

// RELEASED (dedo soltou): aplica volume no codec + toca beep de feedback.
// esp_codec_dev_set_out_vol (I2C) + esp_codec_dev_write (I2S) são chamados
// uma única vez por gesto completo.
static void on_volume_released_cb(lv_event_t* e)
{
    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const int32_t val = lv_slider_get_value(slider);
    if (g_volume_fn) (*g_volume_fn)(static_cast<int>(val));
}

}  // namespace

void np_update_settings(const AppState& state)
{
    // Cache dos campos triviais usados pelos callbacks
    g_prefs_time24h = state.preferences.time_format_24h;
    g_prefs_theme   = state.preferences.theme;

    char initials[3];
    make_initials(state.preferences.display_name, initials, sizeof(initials));
    set_line_if_changed(g_settings_profile_initials, initials);

    const char* display_name = state.preferences.display_name.empty()
        ? "NovaPanel"
        : state.preferences.display_name.c_str();
    set_line_if_changed(g_settings_profile_name, display_name);
    set_line_if_changed(g_settings_profile_subtitle, "Perfil padrao / NovaPanel ESP32-P4");

    const bool connected = state.onboarding.wifi_status == WifiConnectStatus::Connected;
    set_line_if_changed(g_settings_wifi_name, connected ? "Wi-Fi conectado" : "Wi-Fi offline");
    char wifi_meta[96];
    std::snprintf(wifi_meta, sizeof(wifi_meta), "%s / %s",
                  state.system.network_ready ? "ESP-Hosted ativo" : "sem transporte",
                  connected ? "link estabelecido" : "sem conexao");
    set_line_if_changed(g_settings_wifi_meta, wifi_meta);

    set_line_if_changed(g_settings_bt_status, "Desativado");

    const char* timezone = state.preferences.timezone.empty()
        ? "America/Sao_Paulo"
        : state.preferences.timezone.c_str();
    set_line_if_changed(g_settings_timezone_name, timezone);
    char tz_meta[80];
    std::snprintf(tz_meta, sizeof(tz_meta), "%s / %s",
                  "UTC-3", state.clock.synced ? "NTP automatico" : "sem sync");
    set_line_if_changed(g_settings_timezone_meta, tz_meta);

    const bool night_mode = state.preferences.theme == ThemeMode::Dark;
    set_line_if_changed(g_settings_night_subtitle,
                        night_mode ? "Tema escuro ativo" : "Tema automatico ou claro");

    if (g_settings_time_format_24h && g_settings_time_format_12h) {
        style_format_button(g_settings_time_format_24h, state.preferences.time_format_24h);
        style_format_button(g_settings_time_format_12h, !state.preferences.time_format_24h);
    }

    // Sync toggle noturno com a preferência atual
    if (g_settings_night_toggle) {
        if (state.preferences.theme == ThemeMode::Dark) {
            lv_obj_add_state(g_settings_night_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(g_settings_night_toggle, LV_STATE_CHECKED);
        }
    }

    // Lat/lon
    if (g_settings_lat_value) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.4f", state.preferences.latitude);
        set_line_if_changed(g_settings_lat_value, buf);
    }
    if (g_settings_lon_value) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.4f", state.preferences.longitude);
        set_line_if_changed(g_settings_lon_value, buf);
    }

    // Sincroniza slider/label de brilho com as prefs — mas só se divergirem
    // do valor atual para evitar lv_slider_set_value desnecessário, que
    // invalida o slider e causa flash visível a cada update de serviço.
    if (g_brightness_slider) {
        const int32_t pref_val = static_cast<int32_t>(state.preferences.brightness);
        if (lv_slider_get_value(g_brightness_slider) != pref_val) {
            lv_slider_set_value(g_brightness_slider, pref_val, LV_ANIM_OFF);
        }
    }
    if (g_brightness_label) {
        char bbuf[8];
        std::snprintf(bbuf, sizeof(bbuf), "%d%%", state.preferences.brightness);
        set_line_if_changed(g_brightness_label, bbuf);
    }

    const char* sys_values[7] = {
        state.preferences.display_name.empty() ? "NovaPanel ESP32-P4" : state.preferences.display_name.c_str(),
        "ESP32-P4",
        state.system.network_ready ? "online" : "offline",
        state.system.cache_ready ? "ok" : "indisponivel",
        state.system.reset_reason,
        nullptr,
        clock_source_label(state),
    };

    char reboot_count[24];
    std::snprintf(reboot_count, sizeof(reboot_count), "%lu",
                  static_cast<unsigned long>(state.system.reboot_count));
    sys_values[5] = reboot_count;

    const lv_color_t sys_colors[7] = {
        NP_C_TEXT,
        NP_C_TEXT,
        state.system.network_ready ? NP_C_GREEN : NP_C_ACCENT,
        state.system.cache_ready ? NP_C_GREEN : NP_C_RED,
        NP_C_ACCENT,
        NP_C_TEXT,
        state.clock.synced ? NP_C_GREEN : NP_C_ACCENT,
    };

    for (int i = 0; i < 7; ++i) {
        set_line_if_changed(g_settings_sys_value[i], sys_values[i]);
        set_color_if_changed(g_settings_sys_value[i], sys_colors[i]);
    }
}

void np_screen_settings(lv_obj_t* parent)
{
    const auto index = static_cast<std::size_t>(ScreenId::Settings);

    lv_obj_t* scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr, 11, 0);
    np_screens[index] = scr;

    lv_obj_t* prof = np_card(scr);
    lv_obj_set_size(prof, lv_pct(100), 68);
    lv_obj_set_style_pad_hor(prof, 18, 0);
    lv_obj_set_style_pad_ver(prof, 0, 0);
    lv_obj_set_flex_flow(prof, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(prof,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(prof, 14, 0);

    lv_obj_t* avatar = lv_obj_create(prof);
    lv_obj_set_size(avatar, 42, 42);
    lv_obj_set_style_bg_color(avatar, NP_C_ACCENT, 0);
    lv_obj_set_style_radius(avatar, 21, 0);
    lv_obj_set_style_border_width(avatar, 0, 0);
    lv_obj_set_style_pad_all(avatar, 0, 0);
    lv_obj_set_scrollbar_mode(avatar, LV_SCROLLBAR_MODE_OFF);
    g_settings_profile_initials = np_label(avatar, "NP", NP_F_LG, NP_C_DARK_FG);
    lv_obj_align(g_settings_profile_initials, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* profile_info = np_col(prof);
    lv_obj_set_style_flex_grow(profile_info, 1, 0);
    lv_obj_set_size(profile_info, 0, LV_SIZE_CONTENT);
    g_settings_profile_name = np_label(profile_info, "NovaPanel", NP_F_MD, NP_C_TEXT);
    lv_label_set_long_mode(g_settings_profile_name, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_settings_profile_name, 220);
    g_settings_profile_subtitle = np_label(profile_info,
        "Perfil padrao / NovaPanel ESP32-P4", NP_F_SM, lv_color_hex(0x5A6478));
    lv_label_set_long_mode(g_settings_profile_subtitle, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_settings_profile_subtitle, 240);
    np_chip(prof, "Editar");

    lv_obj_t* grid = lv_obj_create(scr);
    np_obj_clear_style(grid);
    lv_obj_set_size(grid, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(grid, 1, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(grid, 11, 0);

    lv_obj_t* net = np_card(grid);
    lv_obj_set_style_flex_grow(net, 1, 0);
    lv_obj_set_height(net, lv_pct(100));
    lv_obj_set_style_pad_all(net, 16, 0);
    lv_obj_set_flex_flow(net, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(net, LV_SCROLLBAR_MODE_ON);

    lv_obj_t* net_h = np_row(net);
    lv_obj_set_style_margin_bottom(net_h, 14, 0);
    lv_obj_set_style_pad_column(net_h, 7, 0);
    np_label(net_h, NP_I_WIFI, NP_F_ICON_SM, NP_C_ACCENT);
    np_label(net_h, "Rede", NP_F_SM, NP_C_ACCENT);
    np_label(net, "WI-FI", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t* wifi_card = lv_obj_create(net);
    lv_obj_set_size(wifi_card, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wifi_card, lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_bg_opa(wifi_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(wifi_card, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_border_width(wifi_card, 1, 0);
    lv_obj_set_style_radius(wifi_card, 9, 0);
    lv_obj_set_style_pad_all(wifi_card, 11, 0);
    lv_obj_set_style_margin_top(wifi_card, 7, 0);
    lv_obj_set_scrollbar_mode(wifi_card, LV_SCROLLBAR_MODE_OFF);
    g_settings_wifi_name = np_label(wifi_card, "Wi-Fi offline", NP_F_SM, NP_C_TEXT);
    g_settings_wifi_meta = np_label(wifi_card, "sem transporte / sem conexao",
                                    NP_F_XS, lv_color_hex(0x5A6478));
    lv_label_set_long_mode(g_settings_wifi_meta, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_settings_wifi_meta, 220);

    lv_obj_t* wifi_manage = np_label(net, "Gerenciar redes Wi-Fi", NP_F_SM, NP_C_ACCENT);
    lv_obj_set_style_margin_top(wifi_manage, 7, 0);
    lv_obj_set_style_margin_bottom(wifi_manage, 14, 0);
    np_hsep(net);

    lv_obj_t* bt_h = np_row(net);
    lv_obj_set_style_margin_top(bt_h, 14, 0);
    lv_obj_set_style_margin_bottom(bt_h, 4, 0);
    np_label(bt_h, "BLUETOOTH", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t* bt_row = np_row(net);
    lv_obj_set_flex_align(bt_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(bt_row, 14, 0);
    lv_obj_t* bt_info = np_col(bt_row);
    lv_obj_set_size(bt_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(bt_info, "Bluetooth", NP_F_SM, NP_C_TEXT);
    g_settings_bt_status = np_label(bt_info, "Desativado", NP_F_XS, lv_color_hex(0x5A6478));
    lv_obj_t* bt_toggle = np_toggle(bt_row, false);
    lv_obj_add_state(bt_toggle, LV_STATE_DISABLED);
    np_hsep(net);

    lv_obj_t* tz_h = np_row(net);
    lv_obj_set_style_margin_top(tz_h, 14, 0);
    lv_obj_set_style_margin_bottom(tz_h, 8, 0);
    np_label(tz_h, "FUSO HORARIO", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t* tz = lv_obj_create(net);
    lv_obj_set_size(tz, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tz, lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_bg_opa(tz, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tz, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_border_width(tz, 1, 0);
    lv_obj_set_style_radius(tz, 9, 0);
    lv_obj_set_style_pad_all(tz, 10, 0);
    lv_obj_set_flex_flow(tz, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tz,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(tz, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* tz_info = np_col(tz);
    lv_obj_set_size(tz_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    g_settings_timezone_name = np_label(tz_info, "America/Sao_Paulo", NP_F_SM, NP_C_TEXT);
    lv_label_set_long_mode(g_settings_timezone_name, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_settings_timezone_name, 150);
    g_settings_timezone_meta = np_label(tz_info, "UTC-3 / sem sync", NP_F_XS, lv_color_hex(0x5A6478));
    lv_label_set_long_mode(g_settings_timezone_meta, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_settings_timezone_meta, 150);
    np_label(tz, NP_I_CHEV_R, NP_F_ICON_SM, NP_C_TEXT_MUTED);

    np_hsep(net);

    lv_obj_t* loc_h = np_row(net);
    lv_obj_set_style_margin_top(loc_h, 14, 0);
    lv_obj_set_style_margin_bottom(loc_h, 8, 0);
    np_label(loc_h, "LOCALIZACAO", NP_F_XS, NP_C_TEXT_DARK);

    static const char* kLocKeys[] = { "Latitude", "Longitude" };
    lv_obj_t** kLocVals[] = { &g_settings_lat_value, &g_settings_lon_value };
    for (int li = 0; li < 2; ++li) {
        lv_obj_t* row = np_row(net);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x1A1D2C), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 9, 0);
        np_label(row, kLocKeys[li], NP_F_SM, lv_color_hex(0x7A8298));
        *kLocVals[li] = np_label(row, "--", NP_F_SM, NP_C_TEXT);
        lv_obj_set_style_text_align(*kLocVals[li], LV_TEXT_ALIGN_RIGHT, 0);
    }
    lv_obj_t* loc_note = np_label(net, "Alterar no assistente de configuracao",
                                  NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(loc_note, 6, 0);

    lv_obj_t* disp = np_card(grid);
    lv_obj_set_style_flex_grow(disp, 1, 0);
    lv_obj_set_height(disp, lv_pct(100));
    lv_obj_set_style_pad_all(disp, 16, 0);
    lv_obj_set_flex_flow(disp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(disp, LV_SCROLLBAR_MODE_ON);

    lv_obj_t* disp_h = np_row(disp);
    lv_obj_set_style_margin_bottom(disp_h, 14, 0);
    lv_obj_set_style_pad_column(disp_h, 7, 0);
    np_label(disp_h, NP_I_SUN, NP_F_ICON_SM, NP_C_ACCENT);
    np_label(disp_h, "Display & Som", NP_F_SM, NP_C_ACCENT);

    np_label(disp, "BRILHO", NP_F_XS, NP_C_TEXT_DARK);
    g_brightness_slider = interactive_slider_row(disp, "Brilho da tela", 78,
                                                  &g_brightness_label);
    lv_obj_add_event_cb(g_brightness_slider, on_brightness_cb,
                        LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(g_brightness_slider, on_brightness_released_cb,
                        LV_EVENT_RELEASED, nullptr);

    np_label(disp, "VOLUME", NP_F_XS, NP_C_TEXT_DARK);
    g_volume_slider = interactive_slider_row(disp, "Sistema", 65, &g_volume_label);
    lv_obj_add_event_cb(g_volume_slider, on_volume_cb,
                        LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(g_volume_slider, on_volume_released_cb,
                        LV_EVENT_RELEASED, nullptr);
    slider_row(disp, "Musica", 42);
    slider_row(disp, "Alarme", 70);
    np_hsep(disp);

    lv_obj_t* night_row = np_row(disp);
    lv_obj_set_flex_align(night_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(night_row, 14, 0);
    lv_obj_set_style_margin_bottom(night_row, 12, 0);
    lv_obj_t* night_info = np_col(night_row);
    lv_obj_set_size(night_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(night_info, "Modo noturno", NP_F_SM, NP_C_TEXT);
    g_settings_night_subtitle = np_label(night_info,
        "Tema automatico ou claro", NP_F_XS, lv_color_hex(0x5A6478));
    lv_label_set_long_mode(g_settings_night_subtitle, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_settings_night_subtitle, 120);
    g_settings_night_toggle = np_toggle(night_row, false);
    lv_obj_add_event_cb(g_settings_night_toggle, on_night_toggle_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* format_row = np_row(disp);
    lv_obj_set_flex_align(format_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    np_label(format_row, "Formato de hora", NP_F_SM, NP_C_TEXT);

    lv_obj_t* format = lv_obj_create(format_row);
    lv_obj_set_size(format, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_bg_color(format, lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(format, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(format, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(format, 1, 0);
    lv_obj_set_style_radius(format, 7, 0);
    lv_obj_set_style_pad_all(format, 2, 0);
    lv_obj_set_flex_flow(format, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollbar_mode(format, LV_SCROLLBAR_MODE_OFF);

    g_settings_time_format_24h = lv_obj_create(format);
    lv_obj_set_size(g_settings_time_format_24h, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_border_width(g_settings_time_format_24h, 0, 0);
    lv_obj_set_style_radius(g_settings_time_format_24h, 5, 0);
    lv_obj_set_style_pad_hor(g_settings_time_format_24h, 10, 0);
    lv_obj_set_scrollbar_mode(g_settings_time_format_24h, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* fmt24 = np_label(g_settings_time_format_24h, "24h", NP_F_SM, NP_C_TEXT);
    lv_obj_align(fmt24, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(g_settings_time_format_24h, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_settings_time_format_24h, on_time_format_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<intptr_t>(1)));

    g_settings_time_format_12h = lv_obj_create(format);
    lv_obj_set_size(g_settings_time_format_12h, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_border_width(g_settings_time_format_12h, 0, 0);
    lv_obj_set_style_radius(g_settings_time_format_12h, 5, 0);
    lv_obj_set_style_pad_hor(g_settings_time_format_12h, 10, 0);
    lv_obj_set_scrollbar_mode(g_settings_time_format_12h, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* fmt12 = np_label(g_settings_time_format_12h, "12h", NP_F_SM, NP_C_TEXT_MUTED);
    lv_obj_align(fmt12, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(g_settings_time_format_12h, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_settings_time_format_12h, on_time_format_cb, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<intptr_t>(0)));

    lv_obj_t* sys = np_card(grid);
    lv_obj_set_style_flex_grow(sys, 1, 0);
    lv_obj_set_height(sys, lv_pct(100));
    lv_obj_set_style_pad_all(sys, 16, 0);
    lv_obj_set_flex_flow(sys, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(sys, LV_SCROLLBAR_MODE_ON);

    lv_obj_t* sys_h = np_row(sys);
    lv_obj_set_style_margin_bottom(sys_h, 14, 0);
    lv_obj_set_style_pad_column(sys_h, 7, 0);
    np_label(sys_h, NP_I_SETTINGS, NP_F_ICON_SM, NP_C_ACCENT);
    np_label(sys_h, "Sistema", NP_F_SM, NP_C_ACCENT);

    static const char* keys[] = {
        "Dispositivo",
        "Chip",
        "Rede",
        "Cache",
        "Reset",
        "Reboots",
        "Relogio",
    };

    for (int i = 0; i < 7; ++i) {
        lv_obj_t* row = np_row(sys);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x1A1D2C), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 9, 0);
        np_label(row, keys[i], NP_F_SM, lv_color_hex(0x7A8298));
        g_settings_sys_value[i] = np_label(row, "--", NP_F_SM, NP_C_TEXT);
        lv_label_set_long_mode(g_settings_sys_value[i], LV_LABEL_LONG_DOT);
        lv_obj_set_width(g_settings_sys_value[i], 90);
        lv_obj_set_style_text_align(g_settings_sys_value[i], LV_TEXT_ALIGN_RIGHT, 0);
    }

    lv_obj_t* update_btn = lv_button_create(sys);
    lv_obj_set_size(update_btn, lv_pct(100), 40);
    lv_obj_set_style_bg_color(update_btn, NP_C_GREEN_BG, 0);
    lv_obj_set_style_border_color(update_btn, NP_C_GREEN_BD, 0);
    lv_obj_set_style_border_width(update_btn, 1, 0);
    lv_obj_set_style_radius(update_btn, NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(update_btn, 0, 0);
    lv_obj_set_style_margin_top(update_btn, 12, 0);
    lv_obj_set_style_margin_bottom(update_btn, 7, 0);
    lv_obj_set_flex_flow(update_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(update_btn,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(update_btn, 8, 0);
    lv_obj_add_state(update_btn, LV_STATE_DISABLED);
    np_label(update_btn, NP_I_REFRESH, NP_F_ICON_SM, NP_C_GREEN);
    np_label(update_btn, "Atualizar firmware", NP_F_SM, NP_C_GREEN);

    lv_obj_t* reboot_btn = lv_button_create(sys);
    lv_obj_set_size(reboot_btn, lv_pct(100), 40);
    lv_obj_set_style_bg_color(reboot_btn, lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_border_color(reboot_btn, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(reboot_btn, 1, 0);
    lv_obj_set_style_radius(reboot_btn, NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(reboot_btn, 0, 0);
    lv_obj_set_flex_flow(reboot_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(reboot_btn,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(reboot_btn, 8, 0);
    lv_obj_add_event_cb(reboot_btn, on_reboot_cb, LV_EVENT_CLICKED, nullptr);
    np_label(reboot_btn, NP_I_REFRESH, NP_F_ICON_SM, NP_C_TEXT_DIM);
    np_label(reboot_btn, "Reiniciar dispositivo", NP_F_SM, NP_C_TEXT_DIM);
}

// Bind functions — alocam no heap só aqui (dentro de app_main), nunca em boot estático.

void np_bind_settings_time_format(SettingsTimeFormatFn fn)
{
    delete g_time_format_fn;
    g_time_format_fn = new SettingsTimeFormatFn(std::move(fn));
}

void np_bind_settings_theme(SettingsThemeFn fn)
{
    delete g_theme_fn;
    g_theme_fn = new SettingsThemeFn(std::move(fn));
}

void np_bind_settings_reboot(SettingsRebootFn fn)
{
    delete g_reboot_fn;
    g_reboot_fn = new SettingsRebootFn(std::move(fn));
}

void np_bind_settings_brightness(SettingsBrightnessFn fn)
{
    delete g_brightness_fn;
    g_brightness_fn = new SettingsBrightnessFn(std::move(fn));
}

void np_bind_settings_brightness_preview(SettingsBrightnessPreviewFn fn)
{
    delete g_brightness_preview_fn;
    g_brightness_preview_fn = new SettingsBrightnessPreviewFn(std::move(fn));
}

void np_bind_settings_volume(SettingsVolumeFn fn)
{
    delete g_volume_fn;
    g_volume_fn = new SettingsVolumeFn(std::move(fn));
}

void np_bind_settings_notify(SettingsNotifyFn fn)
{
    delete g_notify_fn;
    g_notify_fn = new SettingsNotifyFn(std::move(fn));
}

}  // namespace nova
