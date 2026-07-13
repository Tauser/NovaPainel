#pragma once

#include "provider_interfaces.hpp"

#include <string>

namespace nova {

class OpenMeteoProvider final : public IWeatherProvider {
public:
    Result<WeatherState> fetch_weather(double latitude, double longitude) override;

    // Puros (sem IO), expostos para teste host com fixtures reais e
    // malformadas (ADR-0007).
    static std::string build_url(double latitude, double longitude);
    static Result<WeatherState> parse_weather_payload(const std::string& body);
};

}  // namespace nova
