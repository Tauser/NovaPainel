#pragma once

#include "i_forex_provider.hpp"

namespace nova {

class ForexProvider final : public IForexProvider {
public:
    bool fetch(double& usd_brl, uint32_t now_ms) override;
};

}  // namespace nova
