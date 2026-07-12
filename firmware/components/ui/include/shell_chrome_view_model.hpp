#pragma once

#include "app_state.hpp"

#include <string>

namespace nova {

struct ShellChromeViewModel {
    std::string status_line{};
};

ShellChromeViewModel make_shell_chrome_view_model(const AppState& state);

}  // namespace nova
