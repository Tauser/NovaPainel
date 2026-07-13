#include "setup_screen.hpp"
#include "shared_keyboard.hpp"
#include "setup_view_model.hpp"
#include "strings_ptbr.hpp"
#include "timezone_catalog.hpp"
#include "ui_theme.hpp"

#include "lvgl.h"

#if defined(ESP_PLATFORM)
#include "esp_log.h"
#endif

#include <cstdio>
#include <cstring>
#include <string>

namespace nova {

void update_setup_screen(const AppState& state);

namespace {
#if defined(ESP_PLATFORM)
constexpr const char* kTag = "SetupScreen";
#endif

struct SetupScreenContext {
    lv_obj_t* root{nullptr};
    lv_obj_t* title{nullptr};
    lv_obj_t* detail{nullptr};
    lv_obj_t* step{nullptr};
    lv_obj_t* progress{nullptr};
    lv_obj_t* step_badge{nullptr};
    lv_obj_t* wifi_panel{nullptr};
    lv_obj_t* wifi_list{nullptr};
    lv_obj_t* wifi_selected{nullptr};
    lv_obj_t* wifi_status{nullptr};
    lv_obj_t* wifi_summary{nullptr};
    lv_obj_t* wifi_runtime{nullptr};
    lv_obj_t* wifi_password_label{nullptr};
    lv_obj_t* wifi_password{nullptr};
    lv_obj_t* timezone_panel{nullptr};
    lv_obj_t* timezone_value{nullptr};
    lv_obj_t* timezone_btn{nullptr};
    lv_obj_t* format_value{nullptr};
    lv_obj_t* format_btn{nullptr};
    lv_obj_t* confirmation_panel{nullptr};
    lv_obj_t* confirmation_wifi{nullptr};
    lv_obj_t* confirmation_timezone{nullptr};
    lv_obj_t* confirmation_format{nullptr};
    lv_obj_t* footer{nullptr};
    lv_obj_t* back_btn{nullptr};
    lv_obj_t* next_btn{nullptr};
    lv_obj_t* rescan_btn{nullptr};
};

SetupScreenContext g_setup_screen{};
ActionQueue* g_action_queue{nullptr};
AppState g_last_state{};
std::size_t g_selected_wifi_index{0};
bool g_selected_use_24h{true};
std::size_t g_selected_timezone_index{default_timezone_option_index()};
std::string g_draft_wifi_password{};

void set_text_if_changed(lv_obj_t* label, const char* text) {
    if (lv_strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

void set_button_label(lv_obj_t* button, const char* text) {
    lv_obj_t* label = lv_obj_get_child(button, 0);
    if (label != nullptr) {
        set_text_if_changed(label, text);
    }
}

const WifiNetwork* selected_wifi_network() {
    if (g_selected_wifi_index < g_last_state.setup.wifi_networks.size()) {
        return &g_last_state.setup.wifi_networks[g_selected_wifi_index];
    }
    return nullptr;
}

const TimezoneOption& selected_timezone_option() {
    return timezone_option_at(g_selected_timezone_index);
}

void push_action(const Action& action) {
    if (g_action_queue != nullptr) {
        (void)g_action_queue->push(action);
    }
}

void on_rescan_click(lv_event_t*) {
    Action action{};
    action.type = ActionType::SetupRequestWifiScan;
    push_action(action);
}

void on_back_click(lv_event_t*) {
    Action action{};
    action.type = ActionType::SetupSetStep;
    action.value = -1;
    push_action(action);
}

void on_next_click(lv_event_t*) {
    Action action{};
    action.type = ActionType::SetupSubmit;
    action.setup_submission.step = g_last_state.setup.onboarding_step;
    action.setup_submission.use_24h = g_selected_use_24h;
    std::strncpy(action.setup_submission.timezone, selected_timezone_option().name,
                 sizeof(action.setup_submission.timezone) - 1);

    if (g_last_state.setup.onboarding_step == OnboardingStep::Wifi) {
        const WifiNetwork* selected_network = selected_wifi_network();
        if (selected_network == nullptr) {
            return;
        }

        if (selected_network->secured && g_draft_wifi_password.empty()) {
            return;
        }

        std::strncpy(action.setup_submission.wifi_ssid,
                     selected_network->ssid.c_str(),
                     sizeof(action.setup_submission.wifi_ssid) - 1);
        std::strncpy(action.setup_submission.wifi_password,
                     g_draft_wifi_password.c_str(),
                     sizeof(action.setup_submission.wifi_password) - 1);
    }

    push_action(action);
}

void on_wifi_select(lv_event_t* event) {
    const std::size_t previous_index = g_selected_wifi_index;
    g_selected_wifi_index = static_cast<std::size_t>(
        reinterpret_cast<std::uintptr_t>(lv_event_get_user_data(event)));
    if (previous_index != g_selected_wifi_index) {
        g_draft_wifi_password.clear();
    }
    update_setup_screen(g_last_state);
}

void on_timezone_click(lv_event_t*) {
    g_selected_timezone_index = (g_selected_timezone_index + 1) % timezone_option_count();
    update_setup_screen(g_last_state);
}

void on_format_click(lv_event_t*) {
    g_selected_use_24h = !g_selected_use_24h;
    update_setup_screen(g_last_state);
}

void set_hidden(lv_obj_t* object, bool hidden) {
    if (hidden) {
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    }
}

void style_panel(lv_obj_t* object) {
    lv_obj_set_style_bg_color(object, ui_theme::color_panel(), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(object, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_radius(object, ui_theme::kCardRadius, 0);
    lv_obj_set_style_pad_all(object, 16, 0);
    lv_obj_set_scrollbar_mode(object, LV_SCROLLBAR_MODE_OFF);
}

void style_secondary_button(lv_obj_t* button) {
    lv_obj_set_height(button, 38);
    lv_obj_set_style_bg_color(button, ui_theme::color_panel_alt(), 0);
    lv_obj_set_style_border_color(button, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_radius(button, ui_theme::kButtonRadius, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_pad_hor(button, 14, 0);
}

void style_primary_button(lv_obj_t* button) {
    lv_obj_set_height(button, 38);
    lv_obj_set_style_bg_color(button, ui_theme::color_accent(), 0);
    lv_obj_set_style_bg_color(button, ui_theme::color_disabled(), LV_STATE_DISABLED);
    lv_obj_set_style_radius(button, ui_theme::kButtonRadius, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_pad_hor(button, 18, 0);
}

int progress_for_step(OnboardingStep step) {
    switch (step) {
        case OnboardingStep::Wifi:
            return 33;
        case OnboardingStep::TimezoneAndFormat:
            return 66;
        case OnboardingStep::Confirmation:
        case OnboardingStep::Done:
            return 100;
    }
    return 0;
}

void set_textarea_if_changed(lv_obj_t* textarea, const char* text) {
    const char* current = lv_textarea_get_text(textarea);
    if (current == nullptr || std::strcmp(current, text) != 0) {
        lv_textarea_set_text(textarea, text);
    }
}

void on_wifi_password_changed(lv_event_t* event) {
    const char* text = lv_textarea_get_text(lv_event_get_target_obj(event));
    g_draft_wifi_password = text != nullptr ? text : "";
    update_setup_screen(g_last_state);
}

void on_wifi_password_submit(lv_obj_t*, const char* text, void*) {
    g_draft_wifi_password = text != nullptr ? text : "";
    update_setup_screen(g_last_state);
}

void on_wifi_password_cancel(lv_obj_t*, void*) {}

void render_wifi_list() {
    lv_obj_clean(g_setup_screen.wifi_list);

#if defined(ESP_PLATFORM)
    ESP_LOGI(kTag, "render wifi list: %u network(s), transport=%d, scan=%s",
             static_cast<unsigned>(g_last_state.setup.wifi_networks.size()),
             g_last_state.setup.transport_ready,
             to_string(g_last_state.setup.wifi_scan_status));
#endif

    if (g_last_state.setup.wifi_networks.empty()) {
        lv_obj_t* empty = lv_label_create(g_setup_screen.wifi_list);
        lv_obj_set_width(empty, LV_PCT(100));
        lv_label_set_long_mode(empty, LV_LABEL_LONG_WRAP);
        lv_label_set_text(empty, strings_ptbr::kSetupEmptyNetworks);
        lv_obj_set_style_text_color(empty, ui_theme::color_muted(), 0);
        return;
    }

    if (g_selected_wifi_index >= g_last_state.setup.wifi_networks.size()) {
        g_selected_wifi_index = 0;
    }

    char row_text[128];
    for (std::size_t index = 0; index < g_last_state.setup.wifi_networks.size(); ++index) {
        const WifiNetwork& network = g_last_state.setup.wifi_networks[index];
        lv_obj_t* button = lv_button_create(g_setup_screen.wifi_list);
        lv_obj_set_width(button, LV_PCT(100));
        lv_obj_set_height(button, 56);
        lv_obj_set_style_bg_color(button,
                                  index == g_selected_wifi_index ? ui_theme::color_accent_bg()
                                                                 : ui_theme::color_panel_alt(),
                                  0);
        lv_obj_set_style_border_color(button,
                                      index == g_selected_wifi_index ? ui_theme::color_accent()
                                                                     : ui_theme::color_border(),
                                      0);
        lv_obj_add_event_cb(button, &on_wifi_select, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(index));

        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_radius(button, ui_theme::kButtonRadius, 0);
        lv_obj_set_style_shadow_width(button, 0, 0);
        lv_obj_set_style_pad_all(button, 12, 0);
        lv_obj_set_flex_flow(button, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(button, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(button, 10, 0);

        lv_obj_t* icon = ui_theme::label(button,
                                         network.secured ? ui_theme::icon_wifi_lock()
                                                         : ui_theme::icon_wifi(),
                                         ui_theme::font_icon(),
                                         index == g_selected_wifi_index ? ui_theme::color_accent()
                                                                        : ui_theme::color_muted());

        std::snprintf(row_text, sizeof(row_text), "%s\n%s - %d dBm",
                      network.ssid.c_str(), network.secured ? "Protegida" : "Aberta",
                      static_cast<int>(network.rssi));
        lv_obj_t* label = lv_label_create(button);
        lv_obj_set_style_flex_grow(label, 1, 0);
        lv_obj_set_width(label, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_label_set_text(label, row_text);
        lv_obj_set_style_text_font(label, ui_theme::font_small(), 0);
        lv_obj_set_style_text_color(label, ui_theme::color_text(), 0);

        lv_obj_t* check = ui_theme::label(button, ui_theme::icon_check(), ui_theme::font_icon_xs(),
                                          ui_theme::color_accent());
        set_hidden(check, index != g_selected_wifi_index);
        (void)icon;
    }
}

void sync_local_selection_from_state(const AppState& state) {
    g_selected_use_24h = state.preferences.use_24h;
    g_selected_timezone_index = find_timezone_option_index(state.preferences.timezone);
    if (g_selected_wifi_index >= state.setup.wifi_networks.size()) {
        g_selected_wifi_index = 0;
    }
}

}  // namespace

void bind_setup_screen_action_queue(ActionQueue* action_queue) {
    g_action_queue = action_queue;
}

lv_obj_t* build_setup_screen(lv_obj_t* parent) {
    auto& ctx = g_setup_screen;
    ctx.root = lv_obj_create(parent);
    lv_obj_set_size(ctx.root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(ctx.root, ui_theme::color_bg(), 0);
    lv_obj_set_style_border_width(ctx.root, 0, 0);
    lv_obj_set_style_pad_all(ctx.root, 22, 0);
    lv_obj_set_flex_flow(ctx.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ctx.root, 12, 0);
    lv_obj_set_scrollbar_mode(ctx.root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ctx.root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* header = ui_theme::row(ctx.root);
    lv_obj_set_height(header, 72);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* badge = lv_obj_create(header);
    lv_obj_set_size(badge, 46, 46);
    lv_obj_set_style_bg_color(badge, ui_theme::color_accent_bg(), 0);
    lv_obj_set_style_border_color(badge, ui_theme::color_accent_border(), 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_radius(badge, 23, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    ctx.step_badge = ui_theme::label(badge, "1", ui_theme::font_title(), ui_theme::color_accent());
    lv_obj_align(ctx.step_badge, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* header_text = ui_theme::col(header);
    lv_obj_set_style_flex_grow(header_text, 1, 0);
    lv_obj_set_style_pad_left(header_text, 14, 0);

    ctx.title = lv_label_create(header_text);
    lv_obj_set_style_text_font(ctx.title, ui_theme::font_title_large(), 0);
    lv_obj_set_style_text_color(ctx.title, ui_theme::color_text(), 0);

    ctx.detail = lv_label_create(header_text);
    lv_obj_set_width(ctx.detail, LV_PCT(100));
    lv_label_set_long_mode(ctx.detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.detail, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(ctx.detail, ui_theme::color_muted(), 0);

    lv_obj_t* step_hint = lv_label_create(header);
    lv_obj_set_width(step_hint, 230);
    lv_label_set_long_mode(step_hint, LV_LABEL_LONG_WRAP);
    lv_label_set_text(step_hint, "Primeira configuracao");
    lv_obj_set_style_text_align(step_hint, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(step_hint, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(step_hint, ui_theme::color_muted(), 0);

    ctx.progress = lv_bar_create(ctx.root);
    lv_obj_set_size(ctx.progress, LV_PCT(100), 6);
    lv_obj_set_style_bg_color(ctx.progress, ui_theme::color_panel_alt(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx.progress, ui_theme::color_accent(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(ctx.progress, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(ctx.progress, 3, LV_PART_INDICATOR);
    lv_bar_set_range(ctx.progress, 0, 100);

    ctx.step = lv_label_create(ctx.root);
    lv_obj_set_width(ctx.step, LV_PCT(100));
    lv_label_set_long_mode(ctx.step, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.step, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.step, ui_theme::color_text(), 0);

    ctx.wifi_panel = lv_obj_create(ctx.root);
    lv_obj_set_width(ctx.wifi_panel, LV_PCT(100));
    lv_obj_set_height(ctx.wifi_panel, 350);
    style_panel(ctx.wifi_panel);
    lv_obj_set_style_pad_all(ctx.wifi_panel, 0, 0);
    lv_obj_set_flex_flow(ctx.wifi_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctx.wifi_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* wifi_main = ui_theme::col(ctx.wifi_panel);
    lv_obj_set_style_flex_grow(wifi_main, 1, 0);
    lv_obj_set_width(wifi_main, 0);
    lv_obj_set_height(wifi_main, LV_PCT(100));
    lv_obj_set_style_pad_all(wifi_main, 16, 0);
    lv_obj_set_style_pad_row(wifi_main, 10, 0);

    lv_obj_t* wifi_heading = ui_theme::row(wifi_main);
    lv_obj_set_style_pad_column(wifi_heading, 8, 0);
    ui_theme::label(wifi_heading, ui_theme::icon_wifi(), ui_theme::font_icon(),
                    ui_theme::color_accent());
    ui_theme::label(wifi_heading, "Selecione a rede Wi-Fi do ambiente",
                    ui_theme::font_body(), ui_theme::color_text_medium());

    lv_obj_t* wifi_side = lv_obj_create(ctx.wifi_panel);
    lv_obj_set_size(wifi_side, 300, LV_PCT(100));
    lv_obj_set_style_bg_color(wifi_side, ui_theme::color_panel(), 0);
    lv_obj_set_style_bg_opa(wifi_side, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(wifi_side, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(wifi_side, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(wifi_side, 1, 0);
    lv_obj_set_style_radius(wifi_side, 0, 0);
    lv_obj_set_style_pad_all(wifi_side, 18, 0);
    lv_obj_set_style_pad_row(wifi_side, 9, 0);
    lv_obj_set_flex_flow(wifi_side, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(wifi_side, LV_SCROLLBAR_MODE_OFF);

    ui_theme::label(wifi_side, "REDE SELECIONADA", ui_theme::font_xs(),
                    ui_theme::color_muted());

    ctx.wifi_selected = lv_label_create(wifi_side);
    lv_obj_set_width(ctx.wifi_selected, LV_PCT(100));
    lv_label_set_long_mode(ctx.wifi_selected, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.wifi_selected, ui_theme::font_large(), 0);
    lv_obj_set_style_text_color(ctx.wifi_selected, ui_theme::color_text(), 0);

    ctx.wifi_status = lv_label_create(wifi_side);
    lv_obj_set_width(ctx.wifi_status, LV_PCT(100));
    lv_label_set_long_mode(ctx.wifi_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.wifi_status, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(ctx.wifi_status, ui_theme::color_muted(), 0);

    ui_theme::hsep(wifi_side);

    ctx.wifi_summary = lv_label_create(wifi_side);
    lv_obj_set_width(ctx.wifi_summary, LV_PCT(100));
    lv_label_set_long_mode(ctx.wifi_summary, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.wifi_summary, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(ctx.wifi_summary, ui_theme::color_text(), 0);

    ctx.wifi_runtime = lv_label_create(wifi_side);
    lv_obj_set_width(ctx.wifi_runtime, LV_PCT(100));
    lv_label_set_long_mode(ctx.wifi_runtime, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.wifi_runtime, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(ctx.wifi_runtime, ui_theme::color_muted(), 0);

    ctx.wifi_password_label = lv_label_create(wifi_side);
    lv_obj_set_width(ctx.wifi_password_label, LV_PCT(100));
    lv_label_set_text(ctx.wifi_password_label, strings_ptbr::kSetupPasswordLabel);
    lv_obj_set_style_text_color(ctx.wifi_password_label, ui_theme::color_muted(), 0);

    ctx.wifi_password = lv_textarea_create(wifi_side);
    lv_obj_set_width(ctx.wifi_password, LV_PCT(100));
    lv_textarea_set_one_line(ctx.wifi_password, true);
    lv_textarea_set_password_mode(ctx.wifi_password, true);
    lv_textarea_set_placeholder_text(ctx.wifi_password, strings_ptbr::kSetupPasswordPlaceholder);
    lv_obj_set_style_bg_color(ctx.wifi_password, ui_theme::color_panel_alt(), 0);
    lv_obj_set_style_border_color(ctx.wifi_password, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(ctx.wifi_password, 1, 0);
    lv_obj_set_style_radius(ctx.wifi_password, ui_theme::kButtonRadius, 0);
    lv_obj_set_style_text_font(ctx.wifi_password, ui_theme::font_large(), 0);
    lv_obj_set_style_text_color(ctx.wifi_password, ui_theme::color_text(), 0);
    lv_obj_add_event_cb(ctx.wifi_password, &on_wifi_password_changed, LV_EVENT_VALUE_CHANGED, nullptr);
    shared_keyboard_attach(ctx.wifi_password, ctx.root,
                           SharedKeyboardCallbacks{&on_wifi_password_submit,
                                                   &on_wifi_password_cancel,
                                                   nullptr});

    ctx.wifi_list = lv_obj_create(wifi_main);
    lv_obj_set_width(ctx.wifi_list, LV_PCT(100));
    lv_obj_set_style_flex_grow(ctx.wifi_list, 1, 0);
    lv_obj_set_height(ctx.wifi_list, 0);
    lv_obj_set_flex_flow(ctx.wifi_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ctx.wifi_list, 12, 0);
    lv_obj_set_style_pad_row(ctx.wifi_list, 8, 0);
    lv_obj_set_style_bg_color(ctx.wifi_list, ui_theme::color_bg(), 0);
    lv_obj_set_style_border_color(ctx.wifi_list, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(ctx.wifi_list, 1, 0);
    lv_obj_set_style_radius(ctx.wifi_list, ui_theme::kButtonRadius, 0);
    lv_obj_set_scroll_dir(ctx.wifi_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ctx.wifi_list, LV_SCROLLBAR_MODE_ACTIVE);

    ctx.timezone_panel = lv_obj_create(ctx.root);
    lv_obj_set_width(ctx.timezone_panel, LV_PCT(100));
    lv_obj_set_height(ctx.timezone_panel, 350);
    style_panel(ctx.timezone_panel);
    lv_obj_set_flex_flow(ctx.timezone_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ctx.timezone_panel, 12, 0);

    lv_obj_t* timezone_label = lv_label_create(ctx.timezone_panel);
    lv_label_set_text(timezone_label, strings_ptbr::kSetupTimezoneLabel);
    lv_obj_set_style_text_font(timezone_label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(timezone_label, ui_theme::color_muted(), 0);

    ctx.timezone_value = lv_label_create(ctx.timezone_panel);
    lv_obj_set_width(ctx.timezone_value, LV_PCT(100));
    lv_label_set_long_mode(ctx.timezone_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.timezone_value, ui_theme::font_title(), 0);
    lv_obj_set_style_text_color(ctx.timezone_value, ui_theme::color_text(), 0);

    ctx.timezone_btn = lv_button_create(ctx.timezone_panel);
    style_secondary_button(ctx.timezone_btn);
    lv_obj_t* timezone_btn_label = lv_label_create(ctx.timezone_btn);
    lv_label_set_text(timezone_btn_label, strings_ptbr::kSetupButtonChangeTimezone);
    lv_obj_set_style_text_font(timezone_btn_label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(timezone_btn_label, ui_theme::color_text(), 0);
    lv_obj_center(timezone_btn_label);
    lv_obj_add_event_cb(ctx.timezone_btn, &on_timezone_click, LV_EVENT_CLICKED, nullptr);

    ui_theme::hsep(ctx.timezone_panel);

    lv_obj_t* format_label = lv_label_create(ctx.timezone_panel);
    lv_label_set_text(format_label, strings_ptbr::kSetupFormatLabel);
    lv_obj_set_style_text_font(format_label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(format_label, ui_theme::color_muted(), 0);

    ctx.format_value = lv_label_create(ctx.timezone_panel);
    lv_obj_set_width(ctx.format_value, LV_PCT(100));
    lv_label_set_long_mode(ctx.format_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ctx.format_value, ui_theme::font_title(), 0);
    lv_obj_set_style_text_color(ctx.format_value, ui_theme::color_text(), 0);

    ctx.format_btn = lv_button_create(ctx.timezone_panel);
    style_secondary_button(ctx.format_btn);
    lv_obj_t* format_btn_label = lv_label_create(ctx.format_btn);
    lv_label_set_text(format_btn_label, strings_ptbr::kSetupButtonToggleFormat);
    lv_obj_set_style_text_font(format_btn_label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(format_btn_label, ui_theme::color_text(), 0);
    lv_obj_center(format_btn_label);
    lv_obj_add_event_cb(ctx.format_btn, &on_format_click, LV_EVENT_CLICKED, nullptr);

    ctx.confirmation_panel = lv_obj_create(ctx.root);
    lv_obj_set_width(ctx.confirmation_panel, LV_PCT(100));
    lv_obj_set_height(ctx.confirmation_panel, 350);
    style_panel(ctx.confirmation_panel);
    lv_obj_set_flex_flow(ctx.confirmation_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ctx.confirmation_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ctx.confirmation_panel, 14, 0);

    lv_obj_t* confirmation_title = ui_theme::label(ctx.confirmation_panel, "Tudo pronto!",
                                                   ui_theme::font_title_large(),
                                                   ui_theme::color_text());
    lv_obj_set_style_margin_bottom(confirmation_title, 6, 0);

    ctx.confirmation_wifi = lv_label_create(ctx.confirmation_panel);
    lv_obj_set_width(ctx.confirmation_wifi, LV_PCT(100));
    lv_label_set_long_mode(ctx.confirmation_wifi, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ctx.confirmation_wifi, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ctx.confirmation_wifi, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.confirmation_wifi, ui_theme::color_text(), 0);

    ctx.confirmation_timezone = lv_label_create(ctx.confirmation_panel);
    lv_obj_set_width(ctx.confirmation_timezone, LV_PCT(100));
    lv_label_set_long_mode(ctx.confirmation_timezone, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ctx.confirmation_timezone, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ctx.confirmation_timezone, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.confirmation_timezone, ui_theme::color_text(), 0);

    ctx.confirmation_format = lv_label_create(ctx.confirmation_panel);
    lv_obj_set_width(ctx.confirmation_format, LV_PCT(100));
    lv_label_set_long_mode(ctx.confirmation_format, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ctx.confirmation_format, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ctx.confirmation_format, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.confirmation_format, ui_theme::color_text(), 0);

    ctx.footer = lv_obj_create(ctx.root);
    lv_obj_set_width(ctx.footer, LV_PCT(100));
    lv_obj_set_height(ctx.footer, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ctx.footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx.footer, 0, 0);
    lv_obj_set_style_pad_all(ctx.footer, 0, 0);
    lv_obj_set_style_pad_column(ctx.footer, 12, 0);
    lv_obj_set_flex_flow(ctx.footer, LV_FLEX_FLOW_ROW);

    ctx.rescan_btn = lv_button_create(ctx.footer);
    style_secondary_button(ctx.rescan_btn);
    lv_obj_t* rescan_label = lv_label_create(ctx.rescan_btn);
    lv_label_set_text(rescan_label, strings_ptbr::kSetupButtonRescan);
    lv_obj_set_style_text_font(rescan_label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(rescan_label, ui_theme::color_text(), 0);
    lv_obj_center(rescan_label);
    lv_obj_add_event_cb(ctx.rescan_btn, &on_rescan_click, LV_EVENT_CLICKED, nullptr);

    ctx.back_btn = lv_button_create(ctx.footer);
    style_secondary_button(ctx.back_btn);
    lv_obj_t* back_label = lv_label_create(ctx.back_btn);
    lv_label_set_text(back_label, strings_ptbr::kSetupButtonBack);
    lv_obj_set_style_text_font(back_label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(back_label, ui_theme::color_text(), 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(ctx.back_btn, &on_back_click, LV_EVENT_CLICKED, nullptr);

    ctx.next_btn = lv_button_create(ctx.footer);
    style_primary_button(ctx.next_btn);
    lv_obj_t* next_label = lv_label_create(ctx.next_btn);
    lv_label_set_text(next_label, strings_ptbr::kSetupButtonNext);
    lv_obj_set_style_text_font(next_label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(next_label, ui_theme::color_dark_fg(), 0);
    lv_obj_center(next_label);
    lv_obj_add_event_cb(ctx.next_btn, &on_next_click, LV_EVENT_CLICKED, nullptr);

    return ctx.root;
}

void update_setup_screen(const AppState& state) {
    const bool onboarding_changed = state.setup.onboarding_step != g_last_state.setup.onboarding_step ||
                                    state.preferences.timezone != g_last_state.preferences.timezone ||
                                    state.preferences.use_24h != g_last_state.preferences.use_24h;
    g_last_state = state;
    if (onboarding_changed) {
        sync_local_selection_from_state(state);
    }

    const SetupViewModel vm = make_setup_view_model(state);
    set_text_if_changed(g_setup_screen.title, vm.title);
    set_text_if_changed(g_setup_screen.detail, vm.detail);

    const char* step_text = strings_ptbr::kSetupStepWifi;
    const char* next_text = strings_ptbr::kSetupButtonNext;
    const char* badge_text = "1";
    bool show_wifi = false;
    bool show_timezone = false;
    bool show_confirmation = false;
    switch (state.setup.onboarding_step) {
        case OnboardingStep::Wifi:
            step_text = strings_ptbr::kSetupStepWifi;
            next_text = strings_ptbr::kSetupButtonNext;
            badge_text = "1";
            show_wifi = true;
            break;
        case OnboardingStep::TimezoneAndFormat:
            step_text = strings_ptbr::kSetupStepTimezone;
            next_text = strings_ptbr::kSetupButtonNext;
            badge_text = "2";
            show_timezone = true;
            break;
        case OnboardingStep::Confirmation:
            step_text = strings_ptbr::kSetupStepConfirmation;
            next_text = strings_ptbr::kSetupButtonFinish;
            badge_text = "3";
            show_confirmation = true;
            break;
        case OnboardingStep::Done:
            step_text = "Concluido";
            next_text = strings_ptbr::kSetupButtonFinish;
            badge_text = "OK";
            break;
    }
    set_text_if_changed(g_setup_screen.step, step_text);
    set_text_if_changed(g_setup_screen.step_badge, badge_text);
    lv_bar_set_value(g_setup_screen.progress, progress_for_step(state.setup.onboarding_step), LV_ANIM_OFF);
    set_hidden(g_setup_screen.wifi_panel, !show_wifi);
    set_hidden(g_setup_screen.timezone_panel, !show_timezone);
    set_hidden(g_setup_screen.confirmation_panel, !show_confirmation);
    set_hidden(g_setup_screen.rescan_btn, !show_wifi);
    set_hidden(g_setup_screen.back_btn, state.setup.onboarding_step == OnboardingStep::Wifi);

    char wifi_selected_text[160];
    const WifiNetwork* selected_network = selected_wifi_network();
    if (selected_network != nullptr) {
        std::snprintf(wifi_selected_text, sizeof(wifi_selected_text), "%s %s - %s - %d dBm",
                      strings_ptbr::kSetupWifiSelectedPrefix, selected_network->ssid.c_str(),
                      selected_network->secured ? "Protegida" : "Aberta",
                      static_cast<int>(selected_network->rssi));
    } else {
        std::snprintf(wifi_selected_text, sizeof(wifi_selected_text), "%s %s",
                      strings_ptbr::kSetupWifiSelectedPrefix, strings_ptbr::kSetupConfirmationPending);
    }
    set_text_if_changed(g_setup_screen.wifi_selected, wifi_selected_text);
    set_text_if_changed(g_setup_screen.wifi_summary, vm.wifi_summary.c_str());
    set_text_if_changed(g_setup_screen.wifi_runtime, vm.wifi_runtime.c_str());

    const char* wifi_status = strings_ptbr::kSetupWifiStatusNotSelected;
    bool can_continue_from_wifi = false;
    bool show_password_field = false;
    if (selected_network != nullptr) {
        if (selected_network->secured) {
            show_password_field = true;
            wifi_status = g_draft_wifi_password.empty()
                              ? strings_ptbr::kSetupWifiStatusPasswordPending
                              : strings_ptbr::kSetupWifiStatusPasswordReady;
            can_continue_from_wifi = !g_draft_wifi_password.empty();
        } else {
            wifi_status = strings_ptbr::kSetupWifiStatusOpen;
            can_continue_from_wifi = true;
        }
    }
    set_text_if_changed(g_setup_screen.wifi_status, wifi_status);
    set_hidden(g_setup_screen.wifi_password_label, !show_password_field);
    set_hidden(g_setup_screen.wifi_password, !show_password_field);
    set_textarea_if_changed(g_setup_screen.wifi_password, g_draft_wifi_password.c_str());
    if (!show_wifi || !show_password_field) {
        shared_keyboard_close();
    }
    render_wifi_list();

    const TimezoneOption& timezone = selected_timezone_option();
    char timezone_text[128];
    std::snprintf(timezone_text, sizeof(timezone_text), "%s - %s",
                  timezone.name, timezone.label);
    set_text_if_changed(g_setup_screen.timezone_value, timezone_text);
    set_text_if_changed(g_setup_screen.format_value,
                        g_selected_use_24h ? "24 horas - Ex: 14:30"
                                           : "12 horas - Ex: 2:30 PM");

    char confirmation_wifi[128];
    char confirmation_timezone[192];
    char confirmation_format[64];
    std::snprintf(confirmation_wifi, sizeof(confirmation_wifi), "%s: %s",
                  strings_ptbr::kSetupConfirmationWifi,
                  selected_network != nullptr ? selected_network->ssid.c_str()
                                              : strings_ptbr::kSetupConfirmationPending);
    std::snprintf(confirmation_timezone, sizeof(confirmation_timezone), "%s: %s",
                  strings_ptbr::kSetupConfirmationTimezone, timezone_text);
    std::snprintf(confirmation_format, sizeof(confirmation_format), "%s: %s",
                  strings_ptbr::kSetupConfirmationFormat,
                  g_selected_use_24h ? "24 horas - Ex: 14:30"
                                     : "12 horas - Ex: 2:30 PM");
    set_text_if_changed(g_setup_screen.confirmation_wifi, confirmation_wifi);
    set_text_if_changed(g_setup_screen.confirmation_timezone, confirmation_timezone);
    set_text_if_changed(g_setup_screen.confirmation_format, confirmation_format);

    set_button_label(g_setup_screen.next_btn, next_text);
    if (state.setup.onboarding_step == OnboardingStep::Wifi && !can_continue_from_wifi) {
        lv_obj_add_state(g_setup_screen.next_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(g_setup_screen.next_btn, LV_STATE_DISABLED);
    }
}

}  // namespace nova
