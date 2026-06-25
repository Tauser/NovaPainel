// NovaPainel - providers/forex_provider.hpp
// Real IForexProvider implementation (Fase 5 follow-up): one HTTPS GET to
// AwesomeAPI's /json/last/USD-BRL (no API key, Brazilian public finance API
// - same "no key" convention as CoinGeckoProvider/OpenMeteoProvider). Gives
// the Home screen's "Dolar" row a dedicated rate instead of the
// btc_brl/btc_usd triangulation CoinGeckoProvider used to compute.
#pragma once

#include "i_forex_provider.hpp"

namespace nova {

class ForexProvider : public IForexProvider {
public:
    const char* name() const override { return "ForexProvider"; }
    Result<double> fetch_usd_brl() override;
};

}  // namespace nova
