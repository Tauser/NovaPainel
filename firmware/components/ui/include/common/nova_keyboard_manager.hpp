#pragma once

#include <functional>
#include <vector>

#include "lvgl.h"

namespace nova {

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
    void attach(lv_obj_t* textarea, lv_obj_t* scroll_container = nullptr);
    void open_for(lv_obj_t* textarea, lv_obj_t* scroll_container = nullptr);
    void close();
    bool is_open() const;

    void set_submit_callback(SubmitCallback cb) { on_submit_ = cb; }
    void set_cancel_callback(CancelCallback cb) { on_cancel_ = cb; }
    void set_open_callback(OpenCallback cb) { on_open_ = cb; }
    void set_close_callback(CloseCallback cb) { on_close_ = cb; }
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

    lv_obj_t* panel_ = nullptr;
    lv_obj_t* keyboard_ = nullptr;
    lv_obj_t* active_textarea_ = nullptr;
    lv_obj_t* active_scroll_container_ = nullptr;
    int keyboard_height_large_ = 260;
    int keyboard_height_small_ = 220;
    std::vector<Binding> bindings_{};
    SubmitCallback on_submit_{};
    CancelCallback on_cancel_{};
    OpenCallback on_open_{};
    CloseCallback on_close_{};
};

}  // namespace nova
