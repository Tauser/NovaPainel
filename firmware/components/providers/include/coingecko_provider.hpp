#pragma once

#include "provider_interfaces.hpp"

#include <string>

namespace nova {

class CoinGeckoProvider final : public IMarketProvider {
public:
    Result<MarketState> fetch_market() override;

    // Parsing puro (sem IO), exposto para teste host com fixtures reais e
    // malformadas (ADR-0007). Preenche só btc_usd + valid; last_update_ms/
    // stale/source ficam a cargo do service, que conhece o relógio
    // canônico e a política de offline/stale.
    static Result<MarketState> parse_market_payload(const std::string& body);
};

}  // namespace nova
