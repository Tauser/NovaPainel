#pragma once

#include "app_state.hpp"

#include <cstdint>

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

namespace nova {

struct ScreenSpec {
    ScreenId id{ScreenId::Boot};
    const char* title{nullptr};
    uint32_t invalidation_mask{0};
    lv_obj_t* (*build)(lv_obj_t* parent){nullptr};
    void (*update)(const AppState& state){nullptr};
    void (*on_enter)(){nullptr};
    void (*on_leave)(){nullptr};
};

}  // namespace nova
