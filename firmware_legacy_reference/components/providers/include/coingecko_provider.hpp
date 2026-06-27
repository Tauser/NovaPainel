// NovaPainel - providers/coingecko_provider.hpp
// Real IMarketProvider implementation (Fase 5, ADR-0006/ADR-0021): one
// batched HTTPS GET to CoinGecko's /simple/price for BTC in USD and BRL.
// 60s interval / 6 req-min are enforced by RequestOrchestrator
// (DataDomain::MarketSummary), not here - this class just performs the
// request when MarketService::tick() calls it.
#pragma once

#include "i_market_provider.hpp"

namespace nova {

class CoinGeckoProvider : public IMarketProvider {
public:
    const char* name() const override { return "CoinGeckoProvider"; }
    Result<MarketSummary> fetch_summary() override;
};

}  // namespace nova
