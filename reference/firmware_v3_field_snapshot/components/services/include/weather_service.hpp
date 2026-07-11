// NovaPainel - services/weather_service.hpp
// Pulls a weather snapshot through a provider, but ONLY when the
// RequestOrchestrator allows it (DataDomain::Weather: 10min interval, 6
// req-min). Mirrors market_service.hpp.
#pragma once

#include "cache_store.hpp"
#include "i_weather_provider.hpp"
#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class WeatherService : public Service {
public:
    WeatherService(StateStore& store, RequestOrchestrator& orchestrator,
                   IWeatherProvider& provider, CacheStore& cache)
        : store_(store), orchestrator_(orchestrator), provider_(provider), cache_(cache) {}

    const char* name() const override { return "WeatherService"; }
    bool init() override;
    void tick(uint32_t now_ms) override;

private:
    StateStore&           store_;
    RequestOrchestrator&  orchestrator_;
    IWeatherProvider&     provider_;
    CacheStore&           cache_;
};

}  // namespace nova
