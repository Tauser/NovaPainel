// NovaPainel - providers/mock_market_provider.cpp
#include "mock_market_provider.hpp"

namespace nova {

Result<MarketSummary> MockMarketProvider::fetch_summary() {
    MarketSummary s{};
    s.btc_usd        = 104200.0;   // mock
    s.usd_brl        = 5.42;       // mock
    s.btc_change_24h = 1.8;        // mock (+1.8%)
    s.valid          = true;
    s.stale          = false;
    return Result<MarketSummary>::ok(s);
}

}  // namespace nova
