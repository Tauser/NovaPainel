#pragma once

#include "provider_interfaces.hpp"

#include <string>

namespace nova {

class AwesomeApiProvider final : public IForexProvider {
public:
    Result<MarketState> fetch_forex() override;

    // Parsing puro (sem IO), exposto para teste host com fixtures reais e
    // malformadas (ADR-0007). Preenche só usd_brl + valid; last_update_ms/
    // stale/source ficam a cargo do service.
    static Result<MarketState> parse_forex_payload(const std::string& body);
};

}  // namespace nova
