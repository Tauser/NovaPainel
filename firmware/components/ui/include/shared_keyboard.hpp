#pragma once

#include "lvgl.h"

namespace nova {

struct SharedKeyboardCallbacks {
    void (*on_submit)(lv_obj_t* textarea, const char* text, void* context){nullptr};
    void (*on_cancel)(lv_obj_t* textarea, void* context){nullptr};
    void* context{nullptr};
};

void bind_shared_keyboard_host(lv_obj_t* panel, lv_obj_t* content, lv_obj_t* dots_row);
void shared_keyboard_attach(lv_obj_t* textarea,
                            lv_obj_t* scroll_container,
                            SharedKeyboardCallbacks callbacks = {});
void shared_keyboard_set_fullscreen(bool fullscreen);
void shared_keyboard_open_for(lv_obj_t* textarea);
void shared_keyboard_close();
bool shared_keyboard_visible();

}  // namespace nova
