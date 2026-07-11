#pragma once

#include "app_state.hpp"

namespace nova {

struct HomeViewModel {
    const char* title{nullptr};
    const char* status{nullptr};
    const char* detail{nullptr};
};

HomeViewModel make_home_view_model(const AppState& state);

}  // namespace nova
