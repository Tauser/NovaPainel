// NovaPainel - providers/mock_market_provider.cpp
#include "mock_market_provider.hpp"

namespace nova {

Result<MarketSummary> MockMarketProvider::fetch_summary() {
    MarketSummary s{};
    s.btc_usd        = 104200.0;
    s.usd_brl        = 5.42;
    s.btc_change_24h = 1.8;
    s.valid          = true;
    s.stale          = false;
    s.source         = DataSource::Mock;
    return Result<MarketSummary>::ok(s);
}

}  // namespace nova
