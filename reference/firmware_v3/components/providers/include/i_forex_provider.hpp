#pragma once

#include <cstdint>

#include "app_state.hpp"

namespace nova {

class IForexProvider {
public:
    virtual ~IForexProvider() = default;
    virtual bool fetch(ForexSummary& out, uint32_t now_ms) = 0;
};

}  // namespace nova
