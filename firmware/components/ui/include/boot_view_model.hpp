#pragma once

#include "app_state.hpp"

namespace nova {

struct BootViewModel {
    const char* headline{nullptr};
    const char* detail{nullptr};
};

BootViewModel make_boot_view_model(const AppState& state);

}  // namespace nova
