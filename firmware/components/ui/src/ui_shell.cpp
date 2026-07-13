#include "ui_shell.hpp"

#include "shared_keyboard.hpp"
#include "strings_ptbr.hpp"
#include "shell_chrome_view_model.hpp"
#include "ui_theme.hpp"

#include "lvgl.h"

#if defined(ESP_PLATFORM)
#include "esp_log.h"
#endif

namespace nova {
namespace {
#if defined(ESP_PLATFORM)
constexpr const char* kTag = "UiShell";
#endif

void set_text_if_changed(lv_obj_t* label, const char* text) {
    if (lv_strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

const char* nav_icon_for(ScreenId screen_id) {
    switch (screen_id) {
        case ScreenId::Home:
            return ui_theme::icon_home();
        case ScreenId::Placeholder:
            return ui_theme::icon_list();
        case ScreenId::Boot:
        case ScreenId::Setup:
            return ui_theme::icon_warning();
    }
    return ui_theme::icon_list();
}

}  // namespace

UiShell::UiShell(StateStore& state_store, const ScreenRegistry& screen_registry)
    : state_store_(state_store), screen_registry_(screen_registry) {
    for (std::size_t index = 0; index < built_screens_.size(); ++index) {
        built_screens_[index].id = static_cast<ScreenId>(index);
    }
}

void UiShell::render(uint32_t pending_mask) {
    if (!built_) {
        build_shell();
    }

    const AppState state = state_store_.snapshot();
    const ScreenSpec* spec = screen_registry_.find(state.ui.active_screen);
    if (spec == nullptr) {
        return;
    }

    ensure_screen_built(*spec);

    const bool screen_changed = !has_active_screen_ || active_screen_ != state.ui.active_screen;
    if (screen_changed) {
        switch_screen(*spec);
    }

    update_chrome(*spec, state);
    update_dots(state.ui.active_screen);
    set_chrome_hidden(state.ui.active_screen == ScreenId::Boot ||
                      state.ui.active_screen == ScreenId::Setup);
    update_toast();

    if (screen_changed || (pending_mask & spec->invalidation_mask) != 0u) {
        spec->update(state);
    }

    if (lv_screen_active() != root_) {
        lv_screen_load(root_);
    }
}

void UiShell::navigate(ScreenId screen_id) {
    state_store_.navigate_to(screen_id);
}

void UiShell::build_shell() {
    root_ = lv_obj_create(nullptr);
    lv_obj_set_size(root_, ui_theme::kScreenWidth, ui_theme::kScreenHeight);
    lv_obj_set_style_bg_color(root_, ui_theme::color_bg(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(root_, &UiShell::on_content_gesture, LV_EVENT_GESTURE, this);

    rail_ = lv_obj_create(root_);
    lv_obj_set_size(rail_, ui_theme::kRailWidth, ui_theme::kScreenHeight);
    lv_obj_set_style_bg_color(rail_, ui_theme::color_rail_bg(), 0);
    lv_obj_set_style_bg_opa(rail_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(rail_, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(rail_, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(rail_, 1, 0);
    lv_obj_set_style_radius(rail_, 0, 0);
    lv_obj_set_style_pad_top(rail_, 12, 0);
    lv_obj_set_style_pad_bottom(rail_, 12, 0);
    lv_obj_set_style_pad_hor(rail_, 0, 0);
    lv_obj_set_flex_flow(rail_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(rail_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(rail_, 2, 0);
    lv_obj_set_scrollbar_mode(rail_, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* logo = lv_obj_create(rail_);
    lv_obj_set_size(logo, 36, 36);
    lv_obj_set_style_bg_color(logo, ui_theme::color_accent(), 0);
    lv_obj_set_style_border_width(logo, 0, 0);
    lv_obj_set_style_radius(logo, 9, 0);
    lv_obj_set_style_pad_all(logo, 0, 0);
    lv_obj_set_style_margin_bottom(logo, 6, 0);
    lv_obj_t* logo_label = ui_theme::label(logo, "NP", ui_theme::font_small(), ui_theme::color_dark_fg());
    lv_obj_set_style_text_letter_space(logo_label, 1, 0);
    lv_obj_align(logo_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* sep = lv_obj_create(rail_);
    lv_obj_set_size(sep, 38, 1);
    lv_obj_set_style_bg_color(sep, ui_theme::color_separator(), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_margin_bottom(sep, 6, 0);

    slider_screen_count_ = 0;
    for (std::size_t index = 0; index < screen_registry_.size(); ++index) {
        const ScreenSpec* spec = screen_registry_.at(index);
        if (spec == nullptr || !is_slider_screen(spec->id)) {
            continue;
        }
        const std::size_t nav_index = slider_screen_count_;
        if (nav_index >= slider_screens_.size()) {
            break;
        }
        slider_screens_[nav_index] = spec->id;
        ++slider_screen_count_;

        lv_obj_t* button = lv_button_create(rail_);
        lv_obj_set_size(button, 44, 44);
        lv_obj_set_style_bg_color(button, ui_theme::color_rail_bg(), 0);
        lv_obj_set_style_bg_color(button, ui_theme::color_accent_bg(), LV_STATE_CHECKED);
        lv_obj_set_style_border_color(button, ui_theme::color_border(), LV_STATE_CHECKED);
        lv_obj_set_style_border_width(button, 0, 0);
        lv_obj_set_style_border_width(button, 1, LV_STATE_CHECKED);
        lv_obj_set_style_radius(button, 10, 0);
        lv_obj_set_style_shadow_width(button, 0, 0);
        lv_obj_set_style_pad_all(button, 0, 0);

        nav_targets_[nav_index].shell = this;
        nav_targets_[nav_index].screen_id = spec->id;
        lv_obj_add_event_cb(button, &UiShell::on_nav_click, LV_EVENT_CLICKED, &nav_targets_[nav_index]);
        nav_buttons_[nav_index] = button;

        lv_obj_t* label = ui_theme::label(button, nav_icon_for(spec->id),
                                          ui_theme::font_icon_large(),
                                          ui_theme::color_disabled());
        nav_labels_[nav_index] = label;
        lv_obj_center(label);
    }

    topbar_ = lv_obj_create(root_);
    lv_obj_add_flag(topbar_, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_pos(topbar_, ui_theme::kRailWidth, 0);
    lv_obj_set_size(topbar_, ui_theme::kScreenWidth - ui_theme::kRailWidth, ui_theme::kTopbarHeight);
    lv_obj_set_style_bg_color(topbar_, ui_theme::color_rail_bg(), 0);
    lv_obj_set_style_bg_opa(topbar_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(topbar_, 0, 0);
    lv_obj_set_style_radius(topbar_, 0, 0);
    lv_obj_set_style_pad_hor(topbar_, 18, 0);
    lv_obj_set_style_pad_ver(topbar_, 0, 0);
    lv_obj_set_style_pad_column(topbar_, 10, 0);
    lv_obj_set_flex_flow(topbar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(topbar_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(topbar_, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* title_col = ui_theme::col(topbar_);
    lv_obj_set_height(title_col, ui_theme::kTopbarHeight);
    lv_obj_set_style_flex_grow(title_col, 1, 0);
    lv_obj_set_flex_align(title_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    topbar_title_ = lv_label_create(title_col);
    lv_obj_set_style_text_font(topbar_title_, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(topbar_title_, ui_theme::color_text(), 0);

    topbar_status_label_ = lv_label_create(title_col);
    lv_obj_set_style_text_font(topbar_status_label_, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(topbar_status_label_, ui_theme::color_muted(), 0);
    lv_label_set_text(topbar_status_label_, strings_ptbr::kToastShellReady);

    ui_theme::icon_button(topbar_, ui_theme::icon_wifi(), ui_theme::color_green());
    ui_theme::icon_button(topbar_, ui_theme::icon_bell());
    lv_obj_t* vsep = lv_obj_create(topbar_);
    lv_obj_set_size(vsep, 1, 18);
    lv_obj_set_style_bg_color(vsep, ui_theme::color_border(), 0);
    lv_obj_set_style_bg_opa(vsep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vsep, 0, 0);
    ui_theme::icon_button(topbar_, ui_theme::icon_settings());

    content_ = lv_obj_create(root_);
    lv_obj_add_flag(content_, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(content_, &UiShell::on_content_gesture, LV_EVENT_GESTURE, this);
    lv_obj_set_pos(content_, ui_theme::kRailWidth, ui_theme::kTopbarHeight);
    lv_obj_set_size(content_,
                    ui_theme::kScreenWidth - ui_theme::kRailWidth,
                    ui_theme::kScreenHeight - ui_theme::kTopbarHeight - ui_theme::kDotsHeight);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_style_pad_all(content_, ui_theme::kPad, 0);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);

    dots_row_ = lv_obj_create(root_);
    lv_obj_set_pos(dots_row_, ui_theme::kRailWidth, ui_theme::kScreenHeight - ui_theme::kDotsHeight);
    lv_obj_set_size(dots_row_, ui_theme::kScreenWidth - ui_theme::kRailWidth, ui_theme::kDotsHeight);
    lv_obj_set_style_bg_opa(dots_row_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots_row_, 0, 0);
    lv_obj_set_style_pad_all(dots_row_, 0, 0);
    lv_obj_set_flex_flow(dots_row_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots_row_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots_row_, 6, 0);

    for (std::size_t index = 0; index < slider_screen_count_; ++index) {
        dots_[index] = lv_obj_create(dots_row_);
        lv_obj_set_size(dots_[index], 5, 5);
        lv_obj_set_style_radius(dots_[index], 3, 0);
        lv_obj_set_style_border_width(dots_[index], 0, 0);
    }

    toast_panel_ = lv_obj_create(root_);
    lv_obj_set_size(toast_panel_, 280, LV_SIZE_CONTENT);
    lv_obj_align(toast_panel_, LV_ALIGN_BOTTOM_RIGHT, -20, -36);
    lv_obj_set_style_bg_color(toast_panel_, ui_theme::color_panel(), 0);
    lv_obj_set_style_border_color(toast_panel_, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(toast_panel_, 1, 0);
    lv_obj_set_style_radius(toast_panel_, ui_theme::kCardRadius, 0);
    lv_obj_set_style_pad_hor(toast_panel_, 14, 0);
    lv_obj_set_style_pad_ver(toast_panel_, 10, 0);
    lv_obj_add_flag(toast_panel_, LV_OBJ_FLAG_HIDDEN);

    toast_label_ = lv_label_create(toast_panel_);
    lv_obj_set_width(toast_label_, LV_PCT(100));
    lv_label_set_long_mode(toast_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(toast_label_, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(toast_label_, ui_theme::color_text(), 0);
    lv_label_set_text(toast_label_, strings_ptbr::kToastShellReady);

    keyboard_panel_ = lv_obj_create(root_);
    lv_obj_set_size(keyboard_panel_, ui_theme::kScreenWidth, 260);
    lv_obj_align(keyboard_panel_, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(keyboard_panel_, ui_theme::color_panel_alt(), 0);
    lv_obj_set_style_border_width(keyboard_panel_, 1, 0);
    lv_obj_set_style_border_color(keyboard_panel_, ui_theme::color_border(), 0);
    lv_obj_set_style_radius(keyboard_panel_, 0, 0);
    lv_obj_set_style_pad_all(keyboard_panel_, 0, 0);
    lv_obj_add_flag(keyboard_panel_, LV_OBJ_FLAG_HIDDEN);
    bind_shared_keyboard_host(keyboard_panel_, content_, dots_row_);

    built_ = true;
}

void UiShell::ensure_screen_built(const ScreenSpec& spec) {
    ScreenSlot& slot = built_screens_[static_cast<std::size_t>(spec.id)];
    if (slot.root != nullptr) {
        return;
    }

    slot.root = spec.build(content_);
    if (slot.root != nullptr) {
        if (is_slider_screen(spec.id)) {
            lv_obj_add_flag(slot.root, LV_OBJ_FLAG_GESTURE_BUBBLE);
            lv_obj_add_event_cb(slot.root, &UiShell::on_content_gesture, LV_EVENT_GESTURE, this);
        }
        lv_obj_add_flag(slot.root, LV_OBJ_FLAG_HIDDEN);
    }
}

void UiShell::switch_screen(const ScreenSpec& spec) {
    shared_keyboard_close();

    const ScreenSpec* previous_spec = screen_registry_.find(active_screen_);
    ScreenSlot& previous_slot = built_screens_[static_cast<std::size_t>(active_screen_)];
    if (previous_slot.root != nullptr) {
        lv_obj_add_flag(previous_slot.root, LV_OBJ_FLAG_HIDDEN);
    }
    if (previous_spec != nullptr && previous_spec->on_leave != nullptr) {
        previous_spec->on_leave();
    }

    ScreenSlot& next_slot = built_screens_[static_cast<std::size_t>(spec.id)];
    if (next_slot.root != nullptr) {
        lv_obj_clear_flag(next_slot.root, LV_OBJ_FLAG_HIDDEN);
    }
    if (spec.on_enter != nullptr) {
        spec.on_enter();
    }
    active_screen_ = spec.id;
    has_active_screen_ = true;
}

void UiShell::update_chrome(const ScreenSpec& spec, const AppState& state) {
    const ShellChromeViewModel chrome_vm = make_shell_chrome_view_model(state);
    set_text_if_changed(topbar_title_, spec.title);
    set_text_if_changed(topbar_status_label_, chrome_vm.status_line.c_str());
}

void UiShell::update_dots(ScreenId active_screen) {
    for (std::size_t index = 0; index < slider_screen_count_; ++index) {
        if (dots_[index] == nullptr) {
            continue;
        }
        const bool active = slider_screens_[index] == active_screen;
        lv_obj_set_width(dots_[index], active ? 18 : 5);
        lv_obj_set_style_bg_color(dots_[index],
                                  active ? ui_theme::color_accent() : ui_theme::color_dot_inactive(),
                                  0);
        if (nav_buttons_[index] != nullptr) {
            if (active) {
                lv_obj_add_state(nav_buttons_[index], LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(nav_buttons_[index], LV_STATE_CHECKED);
            }
        }
        if (nav_labels_[index] != nullptr) {
            lv_obj_set_style_text_color(nav_labels_[index],
                                        active ? ui_theme::color_accent()
                                               : ui_theme::color_disabled(),
                                        0);
        }
    }
}

bool UiShell::is_slider_screen(ScreenId screen_id) const {
    return screen_id != ScreenId::Boot && screen_id != ScreenId::Setup;
}

std::size_t UiShell::slider_index_for(ScreenId screen_id) const {
    for (std::size_t index = 0; index < slider_screen_count_; ++index) {
        if (slider_screens_[index] == screen_id) {
            return index;
        }
    }
    return slider_screen_count_;
}

void UiShell::navigate_by_delta(int delta) {
    if (delta == 0 || shared_keyboard_visible() || !is_slider_screen(active_screen_) ||
        slider_screen_count_ == 0) {
        return;
    }

    const std::size_t current_index = slider_index_for(active_screen_);
    if (current_index >= slider_screen_count_) {
        return;
    }

    if (delta < 0 && current_index + 1 < slider_screen_count_) {
        navigate(slider_screens_[current_index + 1]);
    } else if (delta > 0 && current_index > 0) {
        navigate(slider_screens_[current_index - 1]);
    }
}

void UiShell::set_chrome_hidden(bool hidden) {
    if (hidden) {
        lv_obj_add_flag(rail_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(topbar_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dots_row_, LV_OBJ_FLAG_HIDDEN);
        if (!shared_keyboard_visible()) {
            lv_obj_add_flag(keyboard_panel_, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_pos(content_, 0, 0);
        lv_obj_set_size(content_, ui_theme::kScreenWidth, ui_theme::kScreenHeight);
        lv_obj_set_style_pad_all(content_, 0, 0);
    } else {
        lv_obj_clear_flag(rail_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(topbar_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(dots_row_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(content_, ui_theme::kRailWidth, ui_theme::kTopbarHeight);
        lv_obj_set_size(content_,
                        ui_theme::kScreenWidth - ui_theme::kRailWidth,
                        ui_theme::kScreenHeight - ui_theme::kTopbarHeight - ui_theme::kDotsHeight);
        lv_obj_set_style_pad_all(content_, ui_theme::kPad, 0);
    }
    shared_keyboard_set_fullscreen(hidden);
}

void UiShell::update_toast() {
    const UiState ui_state = state_store_.ui();
    if (active_screen_ == ScreenId::Boot) {
        hide_toast();
        return;
    }

    if (ui_state.display_breadcrumb) {
        show_toast(strings_ptbr::kToastBootRecovered);
    } else {
        hide_toast();
    }
}

void UiShell::show_toast(const char* text) {
    set_text_if_changed(toast_label_, text);
    lv_obj_clear_flag(toast_panel_, LV_OBJ_FLAG_HIDDEN);
    toast_visible_ = true;
}

void UiShell::hide_toast() {
    if (!toast_visible_) {
        return;
    }
    lv_obj_add_flag(toast_panel_, LV_OBJ_FLAG_HIDDEN);
    toast_visible_ = false;
}

void UiShell::on_nav_click(lv_event_t* event) {
    auto* target = static_cast<NavTarget*>(lv_event_get_user_data(event));
    if (target != nullptr && target->shell != nullptr) {
        target->shell->navigate(target->screen_id);
    }
}

void UiShell::on_content_gesture(lv_event_t* event) {
    auto* shell = static_cast<UiShell*>(lv_event_get_user_data(event));
    if (shell == nullptr) {
        return;
    }

    lv_indev_t* indev = lv_indev_active();
    if (indev == nullptr) {
        return;
    }
    const lv_dir_t direction = lv_indev_get_gesture_dir(indev);
#if defined(ESP_PLATFORM)
    ESP_LOGI(kTag, "gesture dir=%d active=%s", static_cast<int>(direction),
             to_string(shell->active_screen_));
#endif
    if (direction == LV_DIR_LEFT) {
        shell->navigate_by_delta(-1);
        lv_event_stop_bubbling(event);
    } else if (direction == LV_DIR_RIGHT) {
        shell->navigate_by_delta(1);
        lv_event_stop_bubbling(event);
    }
}

}  // namespace nova
