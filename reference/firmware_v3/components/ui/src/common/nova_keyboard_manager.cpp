#include "common/nova_keyboard_manager.hpp"

namespace nova {

namespace {
constexpr uint32_t kCard = 0x141721;
constexpr uint32_t kCard2 = 0x1B1E2D;
constexpr uint32_t kBorder = 0x1E2235;
constexpr uint32_t kText = 0xE8EAF2;
constexpr int kBottomGap = 12;
}

void NovaKeyboardManager::init()
{
}

bool NovaKeyboardManager::is_open() const
{
    return panel_ && !lv_obj_has_flag(panel_, LV_OBJ_FLAG_HIDDEN);
}

void NovaKeyboardManager::attach(lv_obj_t* textarea, lv_obj_t* scroll_container)
{
    if (!textarea) {
        return;
    }

    ensure_binding(textarea, scroll_container);
    lv_obj_add_event_cb(textarea, &NovaKeyboardManager::on_textarea_focused,
                        LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(textarea, &NovaKeyboardManager::on_textarea_clicked,
                        LV_EVENT_CLICKED, this);
}

NovaKeyboardManager::Binding* NovaKeyboardManager::find_binding(lv_obj_t* textarea)
{
    for (auto& binding : bindings_) {
        if (binding.textarea == textarea) {
            return &binding;
        }
    }
    return nullptr;
}

NovaKeyboardManager::Binding* NovaKeyboardManager::ensure_binding(lv_obj_t* textarea,
                                                                  lv_obj_t* scroll_container)
{
    if (!textarea) {
        return nullptr;
    }
    if (auto* binding = find_binding(textarea)) {
        if (scroll_container) {
            binding->scroll_container = scroll_container;
        }
        return binding;
    }

    Binding binding{};
    binding.textarea = textarea;
    binding.scroll_container = scroll_container;
    bindings_.push_back(binding);
    return &bindings_.back();
}

void NovaKeyboardManager::on_textarea_focused(lv_event_t* e)
{
    auto* self = static_cast<NovaKeyboardManager*>(lv_event_get_user_data(e));
    auto* textarea = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!self || !textarea) {
        return;
    }
    auto* binding = self->find_binding(textarea);
    self->open_for(textarea, binding ? binding->scroll_container : nullptr);
}

void NovaKeyboardManager::on_textarea_clicked(lv_event_t* e)
{
    on_textarea_focused(e);
}

void NovaKeyboardManager::open_for(lv_obj_t* textarea, lv_obj_t* scroll_container)
{
    if (!textarea) {
        return;
    }

    ensure_binding(textarea, scroll_container);
    if (active_textarea_ && active_textarea_ != textarea) {
        clear_screen_adjust();
    }

    active_textarea_ = textarea;
    active_scroll_container_ = scroll_container;
    if (auto* binding = find_binding(textarea); binding && !active_scroll_container_) {
        active_scroll_container_ = binding->scroll_container;
    }

    ensure_created();
    lv_keyboard_set_textarea(keyboard_, textarea);
    lv_obj_clear_flag(panel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(panel_, LV_PCT(100), keyboard_height());
    lv_obj_align(panel_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_move_foreground(panel_);

    apply_screen_adjust();
    scroll_active_into_view();

    if (on_open_) {
        on_open_(textarea);
    }
}

void NovaKeyboardManager::ensure_created()
{
    if (panel_) {
        return;
    }

    panel_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(panel_, LV_PCT(100), keyboard_height());
    lv_obj_align(panel_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(panel_, lv_color_hex(kCard), 0);
    lv_obj_set_style_bg_opa(panel_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel_, lv_color_hex(kBorder), 0);
    lv_obj_set_style_border_width(panel_, 1, 0);
    lv_obj_set_style_border_side(panel_, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(panel_, 0, 0);
    lv_obj_set_style_pad_all(panel_, 0, 0);
    lv_obj_set_style_shadow_width(panel_, 0, 0);
    lv_obj_set_scrollbar_mode(panel_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(panel_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(panel_, LV_OBJ_FLAG_HIDDEN);

    keyboard_ = lv_keyboard_create(panel_);
    lv_obj_set_size(keyboard_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(keyboard_, lv_color_hex(kCard2), 0);
    lv_obj_set_style_bg_opa(keyboard_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(keyboard_, 0, 0);
    lv_obj_set_style_radius(keyboard_, 0, 0);
    lv_obj_set_style_shadow_width(keyboard_, 0, 0);
    lv_obj_set_style_bg_color(keyboard_, lv_color_hex(kBorder), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(keyboard_, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(keyboard_, lv_color_hex(kText), LV_PART_ITEMS);
    lv_obj_set_style_radius(keyboard_, 4, LV_PART_ITEMS);

    lv_obj_add_event_cb(keyboard_, &NovaKeyboardManager::on_keyboard_ready,
                        LV_EVENT_READY, this);
    lv_obj_add_event_cb(keyboard_, &NovaKeyboardManager::on_keyboard_cancel,
                        LV_EVENT_CANCEL, this);
}

void NovaKeyboardManager::on_keyboard_ready(lv_event_t* e)
{
    auto* self = static_cast<NovaKeyboardManager*>(lv_event_get_user_data(e));
    if (!self) {
        return;
    }
    if (self->on_submit_ && self->active_textarea_) {
        const char* text = lv_textarea_get_text(self->active_textarea_);
        self->on_submit_(self->active_textarea_, text ? text : "");
    }
    self->close();
}

void NovaKeyboardManager::on_keyboard_cancel(lv_event_t* e)
{
    auto* self = static_cast<NovaKeyboardManager*>(lv_event_get_user_data(e));
    if (!self) {
        return;
    }
    if (self->on_cancel_ && self->active_textarea_) {
        self->on_cancel_(self->active_textarea_);
    }
    self->close();
}

void NovaKeyboardManager::close()
{
    if (!panel_) {
        return;
    }
    if (keyboard_) {
        lv_keyboard_set_textarea(keyboard_, nullptr);
    }
    clear_screen_adjust();
    lv_obj_add_flag(panel_, LV_OBJ_FLAG_HIDDEN);
    active_textarea_ = nullptr;
    active_scroll_container_ = nullptr;
    if (on_close_) {
        on_close_();
    }
}

int NovaKeyboardManager::keyboard_height() const
{
    lv_coord_t height = lv_obj_get_height(lv_layer_top());
    if (height <= 0) {
        height = LV_VER_RES;
    }
    return height >= 600 ? keyboard_height_large_ : keyboard_height_small_;
}

lv_obj_t* NovaKeyboardManager::resolve_adjust_container() const
{
    if (active_scroll_container_) {
        return active_scroll_container_;
    }
    if (active_textarea_) {
        return lv_obj_get_screen(active_textarea_);
    }
    return nullptr;
}

void NovaKeyboardManager::apply_screen_adjust()
{
    lv_obj_t* container = resolve_adjust_container();
    if (!container || !active_textarea_) {
        return;
    }

    auto* binding = find_binding(active_textarea_);
    if (!binding) {
        return;
    }

    if (!binding->has_previous_pad) {
        binding->previous_pad_bottom = lv_obj_get_style_pad_bottom(container, LV_PART_MAIN);
        binding->has_previous_pad = true;
    }

    lv_obj_set_style_pad_bottom(container,
                                binding->previous_pad_bottom + keyboard_height() + kBottomGap,
                                0);
    lv_obj_update_layout(container);
}

void NovaKeyboardManager::clear_screen_adjust()
{
    lv_obj_t* container = resolve_adjust_container();
    if (!container || !active_textarea_) {
        return;
    }

    auto* binding = find_binding(active_textarea_);
    if (!binding || !binding->has_previous_pad) {
        return;
    }

    lv_obj_set_style_pad_bottom(container, binding->previous_pad_bottom, 0);
    binding->has_previous_pad = false;
    binding->previous_pad_bottom = 0;
    lv_obj_update_layout(container);
}

void NovaKeyboardManager::scroll_active_into_view()
{
    if (!active_textarea_) {
        return;
    }

    if (lv_obj_t* screen = lv_obj_get_screen(active_textarea_)) {
        lv_obj_update_layout(screen);
    }
    lv_obj_scroll_to_view_recursive(active_textarea_, LV_ANIM_ON);
}

}  // namespace nova
