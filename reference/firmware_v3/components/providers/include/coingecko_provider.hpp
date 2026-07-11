#pragma once

#include <cstdint>

#include "app_state.hpp"

namespace nova {

class CoinGeckoProvider final {
public:
    bool fetch(CryptoSummary& out, uint32_t now_ms);

    // Fetches OHLC candlestick data from CoinGecko.
    // period drives the `days` parameter:
    //   H1  → days=1  (30-min candles, ~48)
    //   H4  → days=7  (4-hour candles, ~42)
    //   D1  → days=90 (daily candles, ~90)
    //   W1  → days=365 (daily candles, up to 64)
    // CoinGecko free tier: 30 req/min — keep calls infrequent.
    bool fetch_ohlc(OhlcSeries& out, OhlcPeriod period, uint32_t now_ms);
};

}  // namespace nova
