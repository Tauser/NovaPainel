#pragma once

#include <functional>
#include <vector>

#include "lvgl.h"

namespace nova {

// NovaKeyboardManager
// -----------------------------------------------------------------------------
// Global bottom keyboard used by every text input in NovaPanel.
//
// Project rule:
//   - Screens/forms own their lv_textarea inputs.
//   - This manager only opens a shared lv_keyboard and binds it to the focused
//     textarea using lv_keyboard_set_textarea().
//   - No screen should call lv_keyboard_create() directly.
//
// Usage:
//   keyboard_manager_.attach(my_textarea, scroll_container);
//
// When the textarea is focused/clicked:
//   - keyboard opens at the bottom layer
//   - keyboard is connected to the textarea
//   - scroll_container gets bottom padding so the keyboard does not cover fields
//   - focused textarea is scrolled into view
class NovaKeyboardManager {
public:
    using SubmitCallback = std::function<void(lv_obj_t* textarea, const char* text)>;
    using CancelCallback = std::function<void(lv_obj_t* textarea)>;
    using OpenCallback   = std::function<void(lv_obj_t* textarea)>;
    using CloseCallback  = std::function<void()>;

    NovaKeyboardManager() = default;
    ~NovaKeyboardManager() = default;

    NovaKeyboardManager(const NovaKeyboardManager&) = delete;
    NovaKeyboardManager& operator=(const NovaKeyboardManager&) = delete;

    void init();

    // Register a textarea. scroll_container should be the nearest container that
    // can scroll or the screen/root object that should receive bottom padding.
    void attach(lv_obj_t* textarea, lv_obj_t* scroll_container = nullptr);

    // Opens the keyboard programmatically for an already-created textarea.
    void open_for(lv_obj_t* textarea, lv_obj_t* scroll_container = nullptr);

    void close();
    bool is_open() const;

    void set_submit_callback(SubmitCallback cb) { on_submit_ = cb; }
    void set_cancel_callback(CancelCallback cb) { on_cancel_ = cb; }
    void set_open_callback(OpenCallback cb)     { on_open_ = cb; }
    void set_close_callback(CloseCallback cb)   { on_close_ = cb; }

    void set_keyboard_height_large(int height) { keyboard_height_large_ = height; }
    void set_keyboard_height_small(int height) { keyboard_height_small_ = height; }

private:
    struct Binding {
        lv_obj_t* textarea = nullptr;
        lv_obj_t* scroll_container = nullptr;
        int previous_pad_bottom = 0;
        bool has_previous_pad = false;
    };

    static void on_textarea_focused(lv_event_t* e);
    static void on_textarea_clicked(lv_event_t* e);
    static void on_keyboard_ready(lv_event_t* e);
    static void on_keyboard_cancel(lv_event_t* e);

    void ensure_created();
    Binding* find_binding(lv_obj_t* textarea);
    Binding* ensure_binding(lv_obj_t* textarea, lv_obj_t* scroll_container);

    int keyboard_height() const;
    lv_obj_t* resolve_adjust_container() const;
    void apply_screen_adjust();
    void clear_screen_adjust();
    void scroll_active_into_view();

private:
    lv_obj_t* panel_ = nullptr;
    lv_obj_t* keyboard_ = nullptr;
    lv_obj_t* active_textarea_ = nullptr;
    lv_obj_t* active_scroll_container_ = nullptr;

    int keyboard_height_large_ = 260;
    int keyboard_height_small_ = 220;

    std::vector<Binding> bindings_;

    SubmitCallback on_submit_;
    CancelCallback on_cancel_;
    OpenCallback on_open_;
    CloseCallback on_close_;
};

} // namespace nova
