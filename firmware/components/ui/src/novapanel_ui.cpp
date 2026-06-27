#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "esp_log.h"

namespace nova {

namespace {

constexpr const char *kTag = "NovaPanelUI";

struct NavItem {
    ScreenId screen;
    const char *symbol;
};

constexpr NavItem kNavItems[] = {
    { ScreenId::Home,      NP_I_HOME },
    { ScreenId::Calendar,  NP_I_CALENDAR },
    { ScreenId::Market,    NP_I_CHART },
    { ScreenId::Devices,   NP_I_DEVICE },
    { ScreenId::Routines,  NP_I_REFRESH },
    { ScreenId::System,    NP_I_SETTINGS },
    { ScreenId::Focus,     NP_I_FOCUS },
};

constexpr std::size_t kNavCount = sizeof(kNavItems) / sizeof(kNavItems[0]);

static lv_obj_t *g_nav_buttons[kUiScreenCount] = { nullptr };
static lv_obj_t *g_dot_items[kUiScreenCount] = { nullptr };
static lv_obj_t *g_topbar_title = nullptr;
static lv_obj_t *g_topbar_greeting = nullptr;
static lv_obj_t *g_topbar_clock = nullptr;
static lv_obj_t *g_topbar_greeting_chip = nullptr;
static lv_obj_t *g_topbar_notification_badge = nullptr;
static lv_obj_t *g_shell_col = nullptr;
static lv_obj_t *g_menu_btn = nullptr;
static lv_obj_t *g_menu_icon = nullptr;
static bool g_menu_open = false;
static ScreenId g_current_screen = ScreenId::Boot;

static const char *kWeekdaysPt[] = {
    "Domingo", "Segunda-Feira", "Terca-Feira", "Quarta-Feira",
    "Quinta-Feira", "Sexta-Feira", "Sabado",
};

static const char *kMonthsPt[] = {
    "Janeiro", "Fevereiro", "Marco", "Abril", "Maio", "Junho",
    "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro",
};

static const char *greet_by_hour(int hour)
{
    if (hour >= 5 && hour < 12) return "Bom dia";
    if (hour >= 12 && hour < 18) return "Boa tarde";
    return "Boa noite";
}

static void set_topbar_line_if_changed(lv_obj_t *label, const char *text)
{
    if (!label) {
        return;
    }

    const char *current = lv_label_get_text(label);
    if (!current || std::strcmp(current, text) != 0) {
        lv_label_set_text(label, text);
    }
}

static void refresh_clock_label()
{
    if (!g_topbar_clock) return;

    const std::time_t now = std::time(nullptr);
    std::tm tm_now{};
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif

    char buffer[64];
    const char *weekday =
        (tm_now.tm_wday >= 0 && tm_now.tm_wday < 7) ? kWeekdaysPt[tm_now.tm_wday] : "Data";
    const char *month =
        (tm_now.tm_mon >= 0 && tm_now.tm_mon < 12) ? kMonthsPt[tm_now.tm_mon] : "Mes";
    std::snprintf(buffer, sizeof(buffer), "%s, %02d de %s", weekday, tm_now.tm_mday, month);
    lv_label_set_text(g_topbar_clock, buffer);

    if (g_topbar_greeting) {
        lv_label_set_text(g_topbar_greeting, greet_by_hour(tm_now.tm_hour));
    }
}

static void update_dots()
{
    for (std::size_t i = 0; i < kNavCount; ++i) {
        if (!g_dot_items[i]) continue;
        const bool active = (kNavItems[i].screen == g_current_screen);
        lv_obj_set_width(g_dot_items[i], active ? 18 : 5);
        lv_obj_set_style_bg_color(
            g_dot_items[i], active ? NP_C_ACCENT : lv_color_hex(0x252A3C), 0);
    }
}

static void update_menu_button()
{
    if (!g_menu_icon) return;
    lv_label_set_text(g_menu_icon, g_menu_open ? NP_I_CHEV_L : NP_I_CHEV_R);
}

static void apply_shell_layout()
{
    if (!np_rail || !np_topbar || !g_shell_col) return;

    const lv_coord_t rail_x = g_menu_open ? 0 : -NP_RAIL_W;
    const lv_coord_t shell_x = g_menu_open ? NP_RAIL_W : 0;
    const lv_coord_t shell_w = g_menu_open ? NP_CONTENT_W : NP_W;
    const lv_coord_t menu_h = 60;
    const lv_coord_t menu_w = 28;
    const lv_coord_t menu_x = g_menu_open ? (NP_RAIL_W - 6) : -4;
    const lv_coord_t menu_y = (NP_H - menu_h) / 2;

    if (g_menu_open) {
        lv_obj_clear_flag(np_rail, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(np_rail, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_set_pos(np_rail, rail_x, 0);
    lv_obj_set_pos(np_topbar, shell_x, 0);
    lv_obj_set_size(np_topbar, shell_w, NP_TOPBAR_H);
    lv_obj_set_pos(g_shell_col, shell_x, NP_TOPBAR_H);
    lv_obj_set_size(g_shell_col, shell_w, NP_CONTENT_H);
    if (g_menu_btn) {
        lv_obj_set_pos(g_menu_btn, menu_x, menu_y);
        lv_obj_set_size(g_menu_btn, menu_w, menu_h);
        lv_obj_set_style_radius(g_menu_btn, 14, 0);
        lv_obj_set_style_pad_hor(g_menu_btn, 0, 0);
        lv_obj_set_style_pad_ver(g_menu_btn, 0, 0);
        lv_obj_clear_flag(g_menu_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_menu_btn, LV_OBJ_FLAG_FLOATING);
        lv_obj_move_foreground(g_menu_btn);
    }
    update_menu_button();
}

static void nav_cb(lv_event_t *e)
{
    const ScreenId screen = static_cast<ScreenId>(
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    np_navigate_to(screen);
}

static void gear_cb(lv_event_t *)
{
    np_navigate_to(ScreenId::Settings);
}

static void bell_cb(lv_event_t *)
{
    np_navigate_to(ScreenId::System);
}

static void menu_cb(lv_event_t *)
{
    g_menu_open = !g_menu_open;
    apply_shell_layout();
}

static void build_rail()
{
    np_rail = lv_obj_create(np_root);
    lv_obj_set_size(np_rail, NP_RAIL_W, NP_H);
    lv_obj_set_pos(np_rail, 0, 0);
    lv_obj_set_style_bg_color(np_rail, NP_C_RAIL_BG, 0);
    lv_obj_set_style_bg_opa(np_rail, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(np_rail, 0, 0);
    lv_obj_set_style_radius(np_rail, 0, 0);
    lv_obj_set_style_pad_top(np_rail, 12, 0);
    lv_obj_set_style_pad_bottom(np_rail, 12, 0);
    lv_obj_set_style_pad_hor(np_rail, 0, 0);
    lv_obj_set_flex_flow(np_rail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(np_rail,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(np_rail, 2, 0);
    lv_obj_set_scrollbar_mode(np_rail, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *logo = lv_obj_create(np_rail);
    lv_obj_set_size(logo, 36, 36);
    lv_obj_set_style_bg_color(logo, NP_C_ACCENT, 0);
    lv_obj_set_style_radius(logo, 9, 0);
    lv_obj_set_style_border_width(logo, 0, 0);
    lv_obj_set_style_pad_all(logo, 0, 0);
    lv_obj_set_style_margin_bottom(logo, 6, 0);
    lv_obj_set_scrollbar_mode(logo, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *logo_l = lv_label_create(logo);
    lv_label_set_text(logo_l, "NP");
    lv_obj_set_style_text_font(logo_l, NP_F_SM, 0);
    lv_obj_set_style_text_color(logo_l, NP_C_DARK_FG, 0);
    lv_obj_align(logo_l, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *sep = lv_obj_create(np_rail);
    lv_obj_set_size(sep, 38, 1);
    lv_obj_set_style_bg_color(sep, NP_C_SEP, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_radius(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    lv_obj_set_style_margin_bottom(sep, 6, 0);

    for (std::size_t i = 0; i < kNavCount; ++i) {
        const auto screen_index = static_cast<std::size_t>(kNavItems[i].screen);
        lv_obj_t *btn = lv_button_create(np_rail);
        lv_obj_set_size(btn, 44, 44);
        lv_obj_set_style_bg_color(btn, NP_C_RAIL_BG, 0);
        lv_obj_set_style_bg_color(btn, NP_C_ACCENT_BG, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, NP_C_BORDER, LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 1, LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

        lv_obj_t *ic = lv_label_create(btn);
        lv_label_set_text(ic, kNavItems[i].symbol);
        lv_obj_set_style_text_font(ic, NP_F_ICON, 0);
        lv_obj_set_style_text_color(ic, NP_C_TEXT_MUTED, 0);
        lv_obj_set_style_text_color(ic, NP_C_ACCENT, LV_STATE_CHECKED);
        lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(btn, nav_cb, LV_EVENT_CLICKED,
            reinterpret_cast<void *>(static_cast<uintptr_t>(screen_index)));
        g_nav_buttons[screen_index] = btn;
    }
}

static void build_topbar()
{
    np_topbar = lv_obj_create(np_root);
    lv_obj_set_size(np_topbar, NP_CONTENT_W, NP_TOPBAR_H);
    lv_obj_set_pos(np_topbar, NP_RAIL_W, 0);
    lv_obj_set_style_bg_color(np_topbar, NP_C_RAIL_BG, 0);
    lv_obj_set_style_bg_opa(np_topbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(np_topbar, 0, 0);
    lv_obj_set_style_radius(np_topbar, 0, 0);
    lv_obj_set_style_pad_hor(np_topbar, 18, 0);
    lv_obj_set_style_pad_ver(np_topbar, 0, 0);
    lv_obj_set_flex_flow(np_topbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(np_topbar,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(np_topbar, 10, 0);
    lv_obj_set_scrollbar_mode(np_topbar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title_box = lv_obj_create(np_topbar);
    lv_obj_set_height(title_box, NP_TOPBAR_H);
    lv_obj_set_style_bg_opa(title_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_box, 0, 0);
    lv_obj_set_style_pad_all(title_box, 0, 0);
    lv_obj_set_style_pad_row(title_box, 1, 0);
    lv_obj_set_style_flex_grow(title_box, 1, 0);
    lv_obj_set_flex_flow(title_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(title_box,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(title_box, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title_row = lv_obj_create(title_box);
    np_obj_clear_style(title_row);
    lv_obj_set_size(title_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(title_row, 8, 0);

    g_topbar_title = np_label(title_row, "NovaPanel", NP_F_LG, NP_C_TEXT);
    g_topbar_greeting_chip = lv_obj_create(title_row);
    lv_obj_set_size(g_topbar_greeting_chip, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(g_topbar_greeting_chip, NP_C_ACCENT_BG, 0);
    lv_obj_set_style_bg_opa(g_topbar_greeting_chip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_topbar_greeting_chip, 0, 0);
    lv_obj_set_style_radius(g_topbar_greeting_chip, 999, 0);
    lv_obj_set_style_pad_hor(g_topbar_greeting_chip, 8, 0);
    lv_obj_set_style_pad_ver(g_topbar_greeting_chip, 2, 0);
    lv_obj_set_style_margin_left(g_topbar_greeting_chip, 2, 0);
    lv_obj_set_style_margin_top(g_topbar_greeting_chip, 6, 0);
    lv_obj_set_flex_flow(g_topbar_greeting_chip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_topbar_greeting_chip,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(g_topbar_greeting_chip, LV_SCROLLBAR_MODE_OFF);
    g_topbar_greeting = np_label(g_topbar_greeting_chip, "Bom dia", NP_F_XS, NP_C_ACCENT);
    lv_obj_align(g_topbar_greeting, LV_ALIGN_CENTER, 0, 0);
    g_topbar_clock = np_label(title_box, "", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(g_topbar_clock, -1, 0);

    np_icon_btn(np_topbar, NP_I_WIFI, NP_C_GREEN);

    lv_obj_t *bell = np_icon_btn(np_topbar, NP_I_BELL);
    lv_obj_add_event_cb(bell, bell_cb, LV_EVENT_CLICKED, nullptr);

    g_topbar_notification_badge = lv_obj_create(np_topbar);
    lv_obj_set_size(g_topbar_notification_badge, 18, 18);
    lv_obj_set_style_bg_color(g_topbar_notification_badge, NP_C_ACCENT, 0);
    lv_obj_set_style_border_width(g_topbar_notification_badge, 0, 0);
    lv_obj_set_style_radius(g_topbar_notification_badge, 9, 0);
    lv_obj_set_style_pad_all(g_topbar_notification_badge, 0, 0);
    lv_obj_set_style_margin_left(g_topbar_notification_badge, -18, 0);
    lv_obj_set_style_margin_top(g_topbar_notification_badge, -10, 0);
    lv_obj_set_scrollbar_mode(g_topbar_notification_badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(g_topbar_notification_badge, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *badge_label = np_label(g_topbar_notification_badge, "3", NP_F_XS, NP_C_DARK_FG);
    lv_obj_align(badge_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *vs = lv_obj_create(np_topbar);
    lv_obj_set_size(vs, 1, 18);
    lv_obj_set_style_bg_color(vs, NP_C_BORDER, 0);
    lv_obj_set_style_bg_opa(vs, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vs, 0, 0);
    lv_obj_set_style_radius(vs, 0, 0);

    lv_obj_t *gear = np_icon_btn(np_topbar, NP_I_SETTINGS);
    lv_obj_add_event_cb(gear, gear_cb, LV_EVENT_CLICKED, nullptr);
}

static void build_dots(lv_obj_t *col)
{
    np_dots_bar = lv_obj_create(col);
    lv_obj_set_size(np_dots_bar, lv_pct(100), NP_DOTS_H);
    np_obj_clear_style(np_dots_bar);
    lv_obj_set_flex_flow(np_dots_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(np_dots_bar,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(np_dots_bar, 5, 0);

    for (std::size_t i = 0; i < kNavCount; ++i) {
        lv_obj_t *d = lv_obj_create(np_dots_bar);
        const bool active = (kNavItems[i].screen == g_current_screen);
        lv_obj_set_size(d, active ? 18 : 5, 5);
        lv_obj_set_style_bg_color(d,
            active ? NP_C_ACCENT : lv_color_hex(0x252A3C), 0);
        lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(d, 0, 0);
        lv_obj_set_style_radius(d, 3, 0);
        g_dot_items[i] = d;
    }
}

static void build_root()
{
    np_root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(np_root, NP_W, NP_H);
    lv_obj_set_pos(np_root, 0, 0);
    lv_obj_set_style_bg_color(np_root, NP_C_BG, 0);
    lv_obj_set_style_bg_opa(np_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(np_root, 0, 0);
    lv_obj_set_style_radius(np_root, 0, 0);
    lv_obj_set_style_pad_all(np_root, 0, 0);
    lv_obj_set_scrollbar_mode(np_root, LV_SCROLLBAR_MODE_OFF);

    build_rail();
    build_topbar();

    g_menu_btn = lv_button_create(np_root);
    lv_obj_set_style_bg_color(g_menu_btn, NP_C_ITEM_BG, 0);
    lv_obj_set_style_border_color(g_menu_btn, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(g_menu_btn, 1, 0);
    lv_obj_set_style_radius(g_menu_btn, 16, 0);
    lv_obj_set_style_shadow_width(g_menu_btn, 0, 0);
    lv_obj_set_style_pad_all(g_menu_btn, 0, 0);
    lv_obj_set_style_min_width(g_menu_btn, 0, 0);
    lv_obj_add_flag(g_menu_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(g_menu_btn, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_scrollbar_mode(g_menu_btn, LV_SCROLLBAR_MODE_OFF);
    g_menu_icon = lv_label_create(g_menu_btn);
    lv_label_set_text(g_menu_icon, NP_I_CHEV_R);
    lv_obj_set_style_text_font(g_menu_icon, NP_F_ICON_SM, 0);
    lv_obj_set_style_text_color(g_menu_icon, NP_C_TEXT_DIM, 0);
    lv_obj_align(g_menu_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_menu_icon, menu_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_move_foreground(g_menu_btn);
    lv_obj_add_event_cb(g_menu_btn, menu_cb, LV_EVENT_CLICKED, nullptr);

    g_shell_col = lv_obj_create(np_root);
    lv_obj_set_size(g_shell_col, NP_CONTENT_W, NP_CONTENT_H);
    lv_obj_set_pos(g_shell_col, NP_RAIL_W, NP_TOPBAR_H);
    np_obj_clear_style(g_shell_col);
    lv_obj_set_flex_flow(g_shell_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_shell_col,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    np_content = lv_obj_create(g_shell_col);
    lv_obj_set_size(np_content, lv_pct(100), NP_CONTENT_H - NP_DOTS_H);
    lv_obj_set_style_bg_opa(np_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(np_content, 0, 0);
    lv_obj_set_style_radius(np_content, 0, 0);
    lv_obj_set_style_pad_all(np_content, NP_PAD, 0);
    lv_obj_set_scrollbar_mode(np_content, LV_SCROLLBAR_MODE_OFF);

    build_dots(g_shell_col);

    g_menu_open = false;
    apply_shell_layout();
}

static void hide_all_screens()
{
    for (std::size_t i = 0; i < kUiScreenCount; ++i) {
        if (np_screens[i]) {
            lv_obj_add_flag(np_screens[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ensure_screen_built(ScreenId screen)
{
    const auto index = static_cast<std::size_t>(screen);
    if (index >= kUiScreenCount || np_screens[index]) {
        return;
    }

    switch (screen) {
        case ScreenId::Boot:
            np_screen_boot(np_root);
            break;
        case ScreenId::Setup:
            np_screen_setup(np_root);
            break;
        case ScreenId::Home:
            np_screen_home(np_content);
            break;
        case ScreenId::Calendar:
            np_screen_calendar(np_content);
            break;
        case ScreenId::Market:
            np_screen_market(np_content);
            break;
        case ScreenId::Devices:
            np_screen_devices(np_content);
            break;
        case ScreenId::Focus:
            np_screen_focus(np_content);
            break;
        case ScreenId::PhotoFrame:
            np_screen_photoframe(np_content);
            break;
        case ScreenId::Routines:
            np_screen_routines(np_content);
            break;
        case ScreenId::Settings:
            np_screen_settings(np_content);
            break;
        case ScreenId::System:
            np_screen_system(np_content);
            break;
    }
}

}  // namespace

lv_obj_t *np_root = nullptr;
lv_obj_t *np_rail = nullptr;
lv_obj_t *np_topbar = nullptr;
lv_obj_t *np_content = nullptr;
lv_obj_t *np_dots_bar = nullptr;
lv_obj_t *np_screens[kUiScreenCount] = { nullptr };

const char *np_screen_title(ScreenId screen)
{
    switch (screen) {
        case ScreenId::Boot:      return "Boot";
        case ScreenId::Setup:     return "Setup";
        case ScreenId::Home:      return "Home";
        case ScreenId::Market:    return "Market";
        case ScreenId::Calendar:  return "Agenda";
        case ScreenId::Devices:   return "Casa";
        case ScreenId::Focus:     return "Focus";
        case ScreenId::PhotoFrame: return "Photo";
        case ScreenId::Routines:   return "Rotinas";
        case ScreenId::Settings:   return "Configuracoes";
        case ScreenId::System:     return "Sistema";
    }
    return "NovaPanel";
}

void np_navigate_to(ScreenId screen)
{
    const auto target = static_cast<std::size_t>(screen);
    if (target >= kUiScreenCount) return;
    ensure_screen_built(screen);
    if (!np_screens[target]) return;

    const auto current = static_cast<std::size_t>(g_current_screen);
    if (current < kUiScreenCount && np_screens[current]) {
        lv_obj_add_flag(np_screens[current], LV_OBJ_FLAG_HIDDEN);
    }
    if (current < kUiScreenCount && g_nav_buttons[current]) {
        lv_obj_clear_state(g_nav_buttons[current], LV_STATE_CHECKED);
    }

    g_current_screen = screen;

    lv_obj_clear_flag(np_screens[target], LV_OBJ_FLAG_HIDDEN);
    if (g_nav_buttons[target]) {
        lv_obj_add_state(g_nav_buttons[target], LV_STATE_CHECKED);
    }

    update_dots();
}

void np_tick()
{
    refresh_clock_label();
    np_tick_setup();
}

void np_update_shell(const AppState& state)
{
    const char *display_name = state.preferences.display_name.empty()
        ? "NovaPanel"
        : state.preferences.display_name.c_str();
    set_topbar_line_if_changed(g_topbar_title, display_name);

    if (g_topbar_notification_badge) {
        lv_obj_add_flag(g_topbar_notification_badge, LV_OBJ_FLAG_HIDDEN);
    }
}

void np_init()
{
    build_root();
    ensure_screen_built(ScreenId::Boot);

    hide_all_screens();
    np_navigate_to(ScreenId::Boot);
    refresh_clock_label();

    ESP_LOGI(kTag, "UI shell ready: boot screen eager, others lazy");
}

}  // namespace nova
