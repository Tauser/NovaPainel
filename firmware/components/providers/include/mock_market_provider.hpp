// NovaPainel - providers/mock_market_provider.hpp
// Providers abstract external APIs. The real MarketProvider (a later phase) will
// be a CoinGecko REST client with: 60s updates, internal budget of 6 calls/min,
// mandatory cache, and batched requests (one call for BTC + favorites). See
// ADR-0006 and docs/PLANEJAMENTO.md.
//
// MockMarketProvider returns a fixed snapshot so the pipeline can be exercised
// end-to-end without networking.
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

class MockMarketProvider : public IMarketProvider {
public:
    const char* name() const override { return "MockMarketProvider"; }
    Result<MarketSummary> fetch_summary() override;
};

}  // namespace nova
