#include "ui_shell.hpp"

#include "strings_ptbr.hpp"
#include "ui_theme.hpp"

#include "lvgl.h"

namespace nova {
namespace {
void set_text_if_changed(lv_obj_t* label, const char* text) {
    if (lv_strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
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

    update_chrome(*spec);
    update_dots(state.ui.active_screen);
    set_boot_mode(state.ui.active_screen == ScreenId::Boot);

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
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);

    rail_ = lv_obj_create(root_);
    lv_obj_set_size(rail_, ui_theme::kRailWidth, ui_theme::kScreenHeight);
    lv_obj_set_style_bg_color(rail_, ui_theme::color_panel_alt(), 0);
    lv_obj_set_style_border_width(rail_, 0, 0);
    lv_obj_set_style_pad_ver(rail_, 16, 0);
    lv_obj_set_style_pad_hor(rail_, 8, 0);
    lv_obj_set_flex_flow(rail_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rail_, 8, 0);

    for (std::size_t index = 0; index < screen_registry_.size(); ++index) {
        const ScreenSpec* spec = screen_registry_.at(index);
        if (spec == nullptr || spec->id == ScreenId::Boot) {
            continue;
        }

        lv_obj_t* button = lv_button_create(rail_);
        lv_obj_set_width(button, LV_PCT(100));
        lv_obj_set_style_bg_color(button, ui_theme::color_panel(), 0);
        lv_obj_set_style_border_color(button, ui_theme::color_border(), 0);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_radius(button, 10, 0);

        nav_targets_[index].shell = this;
        nav_targets_[index].screen_id = spec->id;
        lv_obj_add_event_cb(button, &UiShell::on_nav_click, LV_EVENT_CLICKED, &nav_targets_[index]);

        lv_obj_t* label = lv_label_create(button);
        lv_obj_set_style_text_font(label, ui_theme::font_small(), 0);
        lv_obj_set_style_text_color(label, ui_theme::color_text(), 0);
        lv_label_set_text(label, spec->title);
        lv_obj_center(label);
    }

    topbar_ = lv_obj_create(root_);
    lv_obj_set_pos(topbar_, ui_theme::kRailWidth, 0);
    lv_obj_set_size(topbar_, ui_theme::kScreenWidth - ui_theme::kRailWidth, ui_theme::kTopbarHeight);
    lv_obj_set_style_bg_color(topbar_, ui_theme::color_panel_alt(), 0);
    lv_obj_set_style_border_width(topbar_, 0, 0);
    lv_obj_set_style_pad_all(topbar_, 16, 0);

    topbar_title_ = lv_label_create(topbar_);
    lv_obj_set_style_text_font(topbar_title_, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(topbar_title_, ui_theme::color_text(), 0);
    lv_obj_align(topbar_title_, LV_ALIGN_LEFT_MID, 0, 0);

    toast_label_ = lv_label_create(topbar_);
    lv_obj_set_style_text_font(toast_label_, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(toast_label_, ui_theme::color_muted(), 0);
    lv_obj_align(toast_label_, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_label_set_text(toast_label_, "toasts e teclado entram por este shell");

    content_ = lv_obj_create(root_);
    lv_obj_set_pos(content_, ui_theme::kRailWidth, ui_theme::kTopbarHeight);
    lv_obj_set_size(content_,
                    ui_theme::kScreenWidth - ui_theme::kRailWidth,
                    ui_theme::kScreenHeight - ui_theme::kTopbarHeight - 24);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_style_pad_all(content_, 0, 0);

    dots_row_ = lv_obj_create(root_);
    lv_obj_set_pos(dots_row_, ui_theme::kRailWidth, ui_theme::kScreenHeight - 24);
    lv_obj_set_size(dots_row_, ui_theme::kScreenWidth - ui_theme::kRailWidth, 24);
    lv_obj_set_style_bg_opa(dots_row_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots_row_, 0, 0);
    lv_obj_set_style_pad_all(dots_row_, 0, 0);
    lv_obj_set_flex_flow(dots_row_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots_row_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots_row_, 6, 0);

    for (std::size_t index = 0; index < screen_registry_.size(); ++index) {
        dots_[index] = lv_obj_create(dots_row_);
        lv_obj_set_size(dots_[index], 8, 8);
        lv_obj_set_style_radius(dots_[index], 4, 0);
        lv_obj_set_style_border_width(dots_[index], 0, 0);
    }

    built_ = true;
}

void UiShell::ensure_screen_built(const ScreenSpec& spec) {
    ScreenSlot& slot = built_screens_[static_cast<std::size_t>(spec.id)];
    if (slot.root != nullptr) {
        return;
    }

    slot.root = spec.build(content_);
    if (slot.root != nullptr) {
        lv_obj_add_flag(slot.root, LV_OBJ_FLAG_HIDDEN);
    }
}

void UiShell::switch_screen(const ScreenSpec& spec) {
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

void UiShell::update_chrome(const ScreenSpec& spec) {
    set_text_if_changed(topbar_title_, spec.title);
    set_text_if_changed(toast_label_,
                        state_store_.ui().display_breadcrumb
                            ? strings_ptbr::kBootDetailRetry
                            : strings_ptbr::kAppTitle);
}

void UiShell::update_dots(ScreenId active_screen) {
    for (std::size_t index = 0; index < screen_registry_.size(); ++index) {
        const ScreenSpec* spec = screen_registry_.at(index);
        if (spec == nullptr || spec->id == ScreenId::Boot || dots_[index] == nullptr) {
            continue;
        }
        const bool active = spec->id == active_screen;
        lv_obj_set_width(dots_[index], active ? 18 : 8);
        lv_obj_set_style_bg_color(dots_[index],
                                  active ? ui_theme::color_accent() : ui_theme::color_border(),
                                  0);
    }
}

void UiShell::set_boot_mode(bool boot_mode) {
    if (boot_mode) {
        lv_obj_add_flag(rail_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(topbar_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dots_row_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(content_, 0, 0);
        lv_obj_set_size(content_, ui_theme::kScreenWidth, ui_theme::kScreenHeight);
    } else {
        lv_obj_clear_flag(rail_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(topbar_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(dots_row_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(content_, ui_theme::kRailWidth, ui_theme::kTopbarHeight);
        lv_obj_set_size(content_,
                        ui_theme::kScreenWidth - ui_theme::kRailWidth,
                        ui_theme::kScreenHeight - ui_theme::kTopbarHeight - 24);
    }
}

void UiShell::on_nav_click(lv_event_t* event) {
    auto* target = static_cast<NavTarget*>(lv_event_get_user_data(event));
    if (target != nullptr && target->shell != nullptr) {
        target->shell->navigate(target->screen_id);
    }
}

}  // namespace nova
