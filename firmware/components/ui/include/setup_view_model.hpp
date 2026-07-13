#pragma once

#include "app_state.hpp"

#include <string>

namespace nova {

struct SetupViewModel {
    const char* title{nullptr};
    const char* detail{nullptr};
    std::string wifi_summary{};
    std::string wifi_runtime{};
    std::string timezone_summary{};
    std::string format_summary{};
};

SetupViewModel make_setup_view_model(const AppState& state);

}  // namespace nova
