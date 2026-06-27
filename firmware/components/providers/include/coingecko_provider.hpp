#pragma once

#include <cstdint>

#include "app_state.hpp"

namespace nova {

class CoinGeckoProvider final {
public:
    bool fetch(CryptoSummary& out, uint32_t now_ms);
};

}  // namespace nova
