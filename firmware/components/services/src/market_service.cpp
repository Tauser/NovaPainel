#include "market_service.hpp"

#include <cstring>

#if defined(ESP_PLATFORM)
#include "esp_log.h"
#endif

namespace nova {
namespace {
#if defined(ESP_PLATFORM)
constexpr const char* kTag = "MarketService";
#endif
constexpr const char* kBtcCacheDomain = "market_btc";
constexpr const char* kForexCacheDomain = "market_forex";

struct BtcBlobV1 {
    double btc_usd;
};

struct ForexBlobV1 {
    double usd_brl;
};

template <typename Blob>
bool load_blob(const CacheStore& cache_store, const char* domain, Blob& out) {
    const auto loaded = cache_store.load(domain, sizeof(Blob));
    if (!loaded.ok()) {
        return false;
    }
    std::memcpy(&out, loaded.value().data(), sizeof(Blob));
    return true;
}

}  // namespace

MarketService::MarketService(StateStore& state_store, CacheStore& cache_store,
                              IMarketProvider& crypto_provider, IForexProvider& forex_provider)
    : state_store_(state_store),
      cache_store_(cache_store),
      crypto_provider_(crypto_provider),
      forex_provider_(forex_provider) {}

void MarketService::load_from_cache(uint64_t now_ms) {
    MarketState current = state_store_.market();
    bool restored_any = false;

    BtcBlobV1 btc{};
    if (load_blob(cache_store_, kBtcCacheDomain, btc)) {
        current.btc_usd = btc.btc_usd;
        restored_any = true;
    }

    ForexBlobV1 forex{};
    if (load_blob(cache_store_, kForexCacheDomain, forex)) {
        current.usd_brl = forex.usd_brl;
        restored_any = true;
    }

    if (!restored_any) {
        return;
    }
    current.valid = true;
    current.stale = true;
    current.source = DataSource::Cache;
    current.last_update_ms = now_ms;
    state_store_.set_market(current);
}

bool MarketService::refresh_crypto_impl(uint64_t now_ms) {
    const auto result = crypto_provider_.fetch_market();
    if (!result.ok()) {
#if defined(ESP_PLATFORM)
        ESP_LOGW(kTag, "crypto fetch failed: %s", result.status().message().c_str());
#endif
        return false;
    }

    MarketState current = state_store_.market();
    current.btc_usd = result.value().btc_usd;
    current.valid = true;
    current.stale = false;
    current.source = DataSource::Live;
    current.last_update_ms = now_ms;
    state_store_.set_market(current);

    const BtcBlobV1 blob{current.btc_usd};
    (void)cache_store_.save(kBtcCacheDomain, &blob, sizeof(blob), now_ms);
    return true;
}

bool MarketService::refresh_forex_impl(uint64_t now_ms) {
    const auto result = forex_provider_.fetch_forex();
    if (!result.ok()) {
#if defined(ESP_PLATFORM)
        ESP_LOGW(kTag, "forex fetch failed: %s", result.status().message().c_str());
#endif
        return false;
    }

    MarketState current = state_store_.market();
    current.usd_brl = result.value().usd_brl;
    current.valid = true;
    current.stale = false;
    current.source = DataSource::Live;
    current.last_update_ms = now_ms;
    state_store_.set_market(current);

    const ForexBlobV1 blob{current.usd_brl};
    (void)cache_store_.save(kForexCacheDomain, &blob, sizeof(blob), now_ms);
    return true;
}

bool MarketService::refresh_crypto(void* context, uint64_t now_ms) {
    return static_cast<MarketService*>(context)->refresh_crypto_impl(now_ms);
}

bool MarketService::refresh_forex(void* context, uint64_t now_ms) {
    return static_cast<MarketService*>(context)->refresh_forex_impl(now_ms);
}

}  // namespace nova
