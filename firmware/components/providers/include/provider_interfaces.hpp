#pragma once

#include "app_state.hpp"
#include "status.hpp"

namespace nova {

class IMarketProvider {
public:
    virtual ~IMarketProvider() = default;
    virtual Result<MarketState> fetch_market() = 0;
};

class IWeatherProvider {
public:
    virtual ~IWeatherProvider() = default;
    virtual Result<WeatherState> fetch_weather() = 0;
};

class IForexProvider {
public:
    virtual ~IForexProvider() = default;
    virtual Result<MarketState> fetch_forex() = 0;
};

}  // namespace nova
