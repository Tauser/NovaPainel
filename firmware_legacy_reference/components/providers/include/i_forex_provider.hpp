// NovaPainel - providers/i_forex_provider.hpp
// Contract every forex (currency exchange rate) provider must implement
// (mirrors i_market_provider.hpp/i_weather_provider.hpp). ForexService
// depends on this, never on a concrete provider.
#pragma once

#include "result.hpp"

namespace nova {

class IForexProvider {
public:
    virtual ~IForexProvider() = default;
    virtual const char* name() const = 0;
    // USD -> BRL rate (e.g. 5.42 means 1 USD = R$ 5.42).
    virtual Result<double> fetch_usd_brl() = 0;
};

}  // namespace nova
