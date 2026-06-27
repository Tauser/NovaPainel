#pragma once

#include <cstdint>

#include "app_state.hpp"

namespace nova {

class OpenMeteoProvider final {
public:
    bool fetch(WeatherSummary& out, uint32_t now_ms);
};

}  // namespace nova
