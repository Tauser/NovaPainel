// NovaPainel - providers/mock_market_provider.hpp
// Returns a fixed market snapshot so the full pipeline can be exercised
// end-to-end without networking. Real provider (CoinGecko REST) comes in Fase 5.
// See ADR-0006 and docs/PLANEJAMENTO.md.
#pragma once

#include "i_market_provider.hpp"

namespace nova {

class MockMarketProvider : public IMarketProvider {
public:
    const char* name() const override { return "MockMarketProvider"; }
    Result<MarketSummary> fetch_summary() override;
};

}  // namespace nova
