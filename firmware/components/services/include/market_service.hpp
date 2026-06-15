// NovaPainel - services/market_service.hpp
// Pulls a market snapshot through a provider, but ONLY when the
// RequestOrchestrator allows it (interval + rate-limit). Writes the result to
// the StateStore. MVP uses MockMarketProvider; later swapped for CoinGecko REST.
#pragma once

#include "mock_market_provider.hpp"
#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class MarketService : public Service {
public:
    MarketService(StateStore& store, RequestOrchestrator& orchestrator,
                  IMarketProvider& provider)
        : store_(store), orchestrator_(orchestrator), provider_(provider) {}

    const char* name() const override { return "MarketService"; }
    bool init() override;
    void tick(uint32_t now_ms) override;

private:
    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    IMarketProvider&     provider_;
};

}  // namespace nova
