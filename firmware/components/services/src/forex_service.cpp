#include "forex_service.hpp"

#if defined(ESP_PLATFORM)

namespace nova {

ForexService::ForexService(StateStore& store,
                           RequestOrchestrator& orchestrator,
                           CacheStore& cache,
                           IForexProvider& provider)
    : store_(store), orchestrator_(orchestrator), cache_(cache), provider_(provider)
{
}

const char* ForexService::name() const
{
    return "ForexService";
}

bool ForexService::init()
{
    ForexSummary cached{};
    if (cache_.load_forex(cached)) {
        cached.usd_brl_stale = true;
        cached.usd_brl_source = DataSource::Cache;
        store_.set_forex(cached);
    }
    return true;
}

bool ForexService::refresh(uint32_t now_ms)
{
    ForexSummary forex{};
    if (!provider_.fetch(forex, now_ms)) {
        ForexSummary cached{};
        if (cache_.load_forex(cached)) {
            cached.usd_brl_stale = true;
            cached.usd_brl_source = DataSource::Cache;
            store_.set_forex(cached);
        }
        orchestrator_.note_request(DataDomain::Forex, now_ms, false);
        return false;
    }

    store_.set_forex(forex);
    cache_.save_forex(forex);
    orchestrator_.note_request(DataDomain::Forex, now_ms, true);
    return true;
}

}  // namespace nova

#else

namespace nova {

ForexService::ForexService(StateStore& store,
                           RequestOrchestrator& orchestrator,
                           CacheStore& cache,
                           IForexProvider& provider)
    : store_(store), orchestrator_(orchestrator), cache_(cache), provider_(provider)
{
}

const char* ForexService::name() const { return "ForexService"; }
bool ForexService::init() { return true; }
bool ForexService::refresh(uint32_t) { return false; }

}  // namespace nova

#endif
