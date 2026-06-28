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
    cache_.save_crypto(market);
    orchestrator_.note_request(DataDomain::MarketSummary, now_ms, true);
    return true;
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

}  // namespace nova

#endif
