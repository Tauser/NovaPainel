// NovaPainel - providers/open_meteo_provider.hpp
// Real IWeatherProvider implementation (Fase 5, ADR-0022): one HTTPS GET to
// Open-Meteo's /v1/forecast (no API key needed, unlike CoinGecko). Location
// is fixed (no location step in the wizard yet) - see .cpp for coordinates.
#pragma once

#include "i_weather_provider.hpp"

namespace nova {

class OpenMeteoProvider : public IWeatherProvider {
public:
    const char* name() const override { return "OpenMeteoProvider"; }
    Result<WeatherSummary> fetch_current() override;
};

}  // namespace nova
