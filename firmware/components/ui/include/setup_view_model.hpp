#pragma once

#include "app_state.hpp"

namespace nova {

struct SetupViewModel {
    const char* title{nullptr};
    const char* detail{nullptr};
};

SetupViewModel make_setup_view_model(const AppState& state);

}  // namespace nova
