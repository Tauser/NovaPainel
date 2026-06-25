// NovaPainel - services/forex_service.hpp
// Pulls the USD/BRL rate through a dedicated IForexProvider, but ONLY when
// the RequestOrchestrator allows it (DataDomain::Forex: 2min interval, 6
// req-min). Mirrors market_service.hpp/weather_service.hpp.
#pragma once

#include "cache_store.hpp"
#include "i_forex_provider.hpp"
#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class ForexService : public Service {
public:
    ForexService(StateStore& store, RequestOrchestrator& orchestrator,
                IForexProvider& provider, CacheStore& cache)
        : store_(store), orchestrator_(orchestrator), provider_(provider), cache_(cache) {}

    const char* name() const override { return "ForexService"; }
    bool init() override;
    void tick(uint32_t now_ms) override;

private:
    StateStore&           store_;
    RequestOrchestrator&  orchestrator_;
    IForexProvider&       provider_;
    CacheStore&           cache_;
};

}  // namespace nova
