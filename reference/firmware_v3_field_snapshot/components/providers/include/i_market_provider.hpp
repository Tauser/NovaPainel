// NovaPainel - providers/i_market_provider.hpp
// Contract every market data provider must implement. MarketService depends on
// this interface, never on a concrete provider. Swap MockMarketProvider for
// CoinGeckoProvider (Fase 5) without touching the service layer.
#pragma once

#include "app_state.hpp"
#include "result.hpp"

namespace nova {

class IMarketProvider {
public:
    virtual ~IMarketProvider() = default;
    virtual const char* name() const = 0;
    virtual Result<MarketSummary> fetch_summary() = 0;
};

}  // namespace nova
