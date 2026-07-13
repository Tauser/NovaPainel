#include "shared_keyboard.hpp"

#include "ui_theme.hpp"

namespace nova {
namespace {

constexpr lv_coord_t kKeyboardHeight = 260;
constexpr lv_coord_t kBottomGap = 12;
constexpr std::size_t kMaxBindings = 8;

struct KeyboardBinding {
    lv_obj_t* textarea{nullptr};
    lv_obj_t* scroll_container{nullptr};
    SharedKeyboardCallbacks callbacks{};
    lv_coord_t previous_pad_bottom{0};
    bool has_previous_pad{false};
};

struct KeyboardHost {
    lv_obj_t* panel{nullptr};
    lv_obj_t* keyboard{nullptr};
    lv_obj_t* content{nullptr};
    lv_obj_t* dots_row{nullptr};
    bool fullscreen{false};
};

KeyboardHost g_keyboard_host{};
KeyboardBinding g_bindings[kMaxBindings]{};
lv_obj_t* g_active_textarea{nullptr};
bool g_keyboard_visible{false};

void update_shell_layout() {
    if (g_keyboard_host.panel == nullptr) {
        return;
    }

    lv_obj_set_size(g_keyboard_host.panel, ui_theme::kScreenWidth, kKeyboardHeight);
    lv_obj_align(g_keyboard_host.panel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_move_foreground(g_keyboard_host.panel);
}

KeyboardBinding* find_binding(lv_obj_t* textarea) {
    for (auto& binding : g_bindings) {
        if (binding.textarea == textarea) {
            return &binding;
        }
    }
    return nullptr;
}

KeyboardBinding* ensure_binding(lv_obj_t* textarea) {
    if (KeyboardBinding* binding = find_binding(textarea); binding != nullptr) {
        return binding;
    }

    for (auto& binding : g_bindings) {
        if (binding.textarea == nullptr) {
            binding.textarea = textarea;
            return &binding;
        }
    }
    return nullptr;
}

void scroll_active_into_view() {
    if (g_active_textarea == nullptr) {
        return;
    }
    if (lv_obj_t* screen = lv_obj_get_screen(g_active_textarea)) {
        lv_obj_update_layout(screen);
    }
    lv_obj_scroll_to_view_recursive(g_active_textarea, LV_ANIM_ON);
}

lv_obj_t* resolve_adjust_container(KeyboardBinding* binding) {
    if (binding != nullptr && binding->scroll_container != nullptr) {
        return binding->scroll_container;
    }
    if (g_active_textarea != nullptr) {
        return lv_obj_get_screen(g_active_textarea);
    }
    return nullptr;
}

void apply_screen_adjust() {
    KeyboardBinding* binding = find_binding(g_active_textarea);
    lv_obj_t* container = resolve_adjust_container(binding);
    if (binding == nullptr || container == nullptr) {
        return;
    }

    if (!binding->has_previous_pad) {
        binding->previous_pad_bottom = lv_obj_get_style_pad_bottom(container, LV_PART_MAIN);
        binding->has_previous_pad = true;
    }

    lv_obj_set_style_pad_bottom(container,
                                binding->previous_pad_bottom + kKeyboardHeight + kBottomGap,
                                0);
    lv_obj_update_layout(container);
}

void clear_screen_adjust() {
    KeyboardBinding* binding = find_binding(g_active_textarea);
    lv_obj_t* container = resolve_adjust_container(binding);
    if (binding == nullptr || container == nullptr || !binding->has_previous_pad) {
        return;
    }

    lv_obj_set_style_pad_bottom(container, binding->previous_pad_bottom, 0);
    binding->has_previous_pad = false;
    binding->previous_pad_bottom = 0;
    lv_obj_update_layout(container);
}

void on_textarea_focus(lv_event_t* event) {
    shared_keyboard_open_for(lv_event_get_target_obj(event));
}

void on_textarea_click(lv_event_t* event) {
    shared_keyboard_open_for(lv_event_get_target_obj(event));
}

void on_textarea_defocus(lv_event_t* event) {
    if (lv_event_get_target_obj(event) == g_active_textarea) {
        shared_keyboard_close();
    }
}

void on_keyboard_ready(lv_event_t*) {
    if (g_active_textarea != nullptr) {
        if (KeyboardBinding* binding = find_binding(g_active_textarea); binding != nullptr &&
            binding->callbacks.on_submit != nullptr) {
            const char* text = lv_textarea_get_text(g_active_textarea);
            binding->callbacks.on_submit(g_active_textarea, text != nullptr ? text : "",
                                         binding->callbacks.context);
        }
    }
    shared_keyboard_close();
}

void on_keyboard_cancel(lv_event_t*) {
    if (g_active_textarea != nullptr) {
        if (KeyboardBinding* binding = find_binding(g_active_textarea); binding != nullptr &&
            binding->callbacks.on_cancel != nullptr) {
            binding->callbacks.on_cancel(g_active_textarea, binding->callbacks.context);
        }
    }
    shared_keyboard_close();
}

}  // namespace

void bind_shared_keyboard_host(lv_obj_t* panel, lv_obj_t* content, lv_obj_t* dots_row) {
    g_keyboard_host.panel = panel;
    g_keyboard_host.content = content;
    g_keyboard_host.dots_row = dots_row;

    g_keyboard_host.keyboard = lv_keyboard_create(panel);
    update_shell_layout();
    lv_obj_set_size(g_keyboard_host.keyboard, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_keyboard_host.keyboard, ui_theme::color_panel_alt(), 0);
    lv_obj_set_style_bg_color(g_keyboard_host.keyboard, ui_theme::color_border(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(g_keyboard_host.keyboard, ui_theme::color_text(), LV_PART_ITEMS);
    lv_obj_set_style_border_width(g_keyboard_host.keyboard, 0, 0);
    lv_obj_add_event_cb(g_keyboard_host.keyboard, &on_keyboard_ready, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(g_keyboard_host.keyboard, &on_keyboard_cancel, LV_EVENT_CANCEL, nullptr);
}

void shared_keyboard_attach(lv_obj_t* textarea,
                            lv_obj_t* scroll_container,
                            SharedKeyboardCallbacks callbacks) {
    if (textarea == nullptr) {
        return;
    }

    KeyboardBinding* binding = ensure_binding(textarea);
    if (binding == nullptr) {
        return;
    }

    binding->scroll_container = scroll_container;
    binding->callbacks = callbacks;
    lv_obj_add_event_cb(textarea, &on_textarea_focus, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(textarea, &on_textarea_click, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(textarea, &on_textarea_defocus, LV_EVENT_DEFOCUSED, nullptr);
}

void shared_keyboard_set_fullscreen(bool fullscreen) {
    g_keyboard_host.fullscreen = fullscreen;
    update_shell_layout();
}

void shared_keyboard_open_for(lv_obj_t* textarea) {
    if (textarea == nullptr || g_keyboard_host.panel == nullptr || g_keyboard_host.keyboard == nullptr) {
        return;
    }

    if (g_active_textarea != nullptr && g_active_textarea != textarea) {
        clear_screen_adjust();
    }

    g_active_textarea = textarea;
    lv_keyboard_set_textarea(g_keyboard_host.keyboard, textarea);
    lv_obj_set_height(g_keyboard_host.panel, kKeyboardHeight);
    lv_obj_clear_flag(g_keyboard_host.panel, LV_OBJ_FLAG_HIDDEN);
    g_keyboard_visible = true;
    update_shell_layout();
    apply_screen_adjust();
    scroll_active_into_view();
}

void shared_keyboard_close() {
    if (g_keyboard_host.panel == nullptr || g_keyboard_host.keyboard == nullptr) {
        g_active_textarea = nullptr;
        g_keyboard_visible = false;
        return;
    }

    clear_screen_adjust();
    lv_keyboard_set_textarea(g_keyboard_host.keyboard, nullptr);
    lv_obj_add_flag(g_keyboard_host.panel, LV_OBJ_FLAG_HIDDEN);
    g_active_textarea = nullptr;
    g_keyboard_visible = false;
    update_shell_layout();
}

bool shared_keyboard_visible() {
    return g_keyboard_visible;
}

}  // namespace nova
