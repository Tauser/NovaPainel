// NovaPainel - providers/i_weather_provider.hpp
// Contract every weather provider must implement (mirrors
// i_market_provider.hpp). WeatherService depends on this, never on a
// concrete provider.
#pragma once

#include "app_state.hpp"
#include "result.hpp"

namespace nova {

class IWeatherProvider {
public:
    virtual ~IWeatherProvider() = default;
    virtual const char* name() const = 0;
    virtual Result<WeatherSummary> fetch_current() = 0;
};

}  // namespace nova
