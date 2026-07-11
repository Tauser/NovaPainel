#pragma once

#include "app_state.hpp"

#include <cstdint>

namespace nova {

struct ScreenViewModel {
    AppState state{};
};

struct ScreenSpec {
    const char* id{nullptr};
    uint32_t invalidation_mask{0};
    void (*build)(const ScreenViewModel& vm){nullptr};
    void (*update)(const ScreenViewModel& vm){nullptr};
};

}  // namespace nova
