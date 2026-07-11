#include "market_service.hpp"

#if defined(ESP_PLATFORM)

namespace nova {

MarketService::MarketService(StateStore& store,
                             RequestOrchestrator& orchestrator,
                             CacheStore& cache,
                             CoinGeckoProvider& provider)
    : store_(store), orchestrator_(orchestrator), cache_(cache), provider_(provider)
{
}

const char* MarketService::name() const
{
    return "MarketService";
}

bool MarketService::init()
{
    CryptoSummary cached{};
    if (cache_.load_crypto(cached)) {
        cached.stale = true;
        cached.source = DataSource::Cache;
        store_.set_crypto(cached);
    }
    return true;
}

bool MarketService::refresh(uint32_t now_ms)
{
    CryptoSummary market{};
    if (!provider_.fetch(market, now_ms)) {
        CryptoSummary cached{};
        if (cache_.load_crypto(cached)) {
            cached.stale = true;
            cached.source = DataSource::Cache;
            store_.set_crypto(cached);
        }
        orchestrator_.note_request(DataDomain::MarketSummary, now_ms, false);
        return false;
    }

    store_.set_crypto(market);
    // Persiste so na primeira vez (popula o cache de boot logo) e depois a cada
    // kCacheSaveIntervalMs, em vez de todo fetch (ver nota no header).
    if (last_cache_save_ms_ == 0 ||
        (now_ms - last_cache_save_ms_) >= kCacheSaveIntervalMs) {
        cache_.save_crypto(market);
        last_cache_save_ms_ = now_ms;
    }
    orchestrator_.note_request(DataDomain::MarketSummary, now_ms, true);

    // Busca OHLC na primeira vez e depois a cada kOhlcIntervalMs.
    // Feito aqui (apos atualizar preco) para nao atrasar o pipeline principal.
    // Se falhar, o grafico mantem o ultimo dado valido.
    if (last_ohlc_ms_ == 0 || (now_ms - last_ohlc_ms_) >= kOhlcIntervalMs) {
        refresh_ohlc(now_ms, pending_period_);
    }

    return true;
}

bool MarketService::refresh_ohlc(uint32_t now_ms, OhlcPeriod period)
{
    OhlcSeries ohlc{};
    if (!provider_.fetch_ohlc(ohlc, period, now_ms)) {
        return false;
    }
    store_.set_btc_ohlc(ohlc);
    last_ohlc_ms_ = now_ms;
    return true;
}

void MarketService::request_ohlc_period(OhlcPeriod period)
{
    pending_period_ = period;
    last_ohlc_ms_   = 0;  // forca re-fetch no proximo refresh()
}

}  // namespace nova

#else

namespace nova {

MarketService::MarketService(StateStore& store,
                             RequestOrchestrator& orchestrator,
                             CacheStore& cache,
                             CoinGeckoProvider& provider)
    : store_(store), orchestrator_(orchestrator), cache_(cache), provider_(provider)
{
}

const char* MarketService::name() const { return "MarketService"; }
bool MarketService::init() { return true; }
bool MarketService::refresh(uint32_t) { return false; }
bool MarketService::refresh_ohlc(uint32_t, OhlcPeriod) { return false; }
void MarketService::request_ohlc_period(OhlcPeriod) {}

}  // namespace nova

#endif
