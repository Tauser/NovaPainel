#pragma once

#include <cstdint>

namespace nova {

class IForexProvider {
public:
    virtual ~IForexProvider() = default;
    virtual bool fetch(double& usd_brl, uint32_t now_ms) = 0;
};

}  // namespace nova
