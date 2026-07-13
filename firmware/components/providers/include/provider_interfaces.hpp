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
    // Localização faz parte do contrato (não só de um provider específico):
    // qualquer fonte de clima precisa de coordenadas para responder.
    virtual Result<WeatherState> fetch_weather(double latitude, double longitude) = 0;
};

class IForexProvider {
public:
    virtual ~IForexProvider() = default;
    virtual Result<MarketState> fetch_forex() = 0;
};

}  // namespace nova
