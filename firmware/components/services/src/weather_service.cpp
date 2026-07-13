#include "weather_service.hpp"

#include <cstring>

#if defined(ESP_PLATFORM)
#include "esp_log.h"
#endif

namespace nova {
namespace {
#if defined(ESP_PLATFORM)
constexpr const char* kTag = "WeatherService";
#endif
constexpr const char* kCacheDomain = "weather";

// Blob de tamanho fixo para persistir em CacheStore -- std::string não pode
// ser gravado cru em flash e relido como válido (ponteiro/buffer interno
// não sobrevive ao round-trip). summary trunca em 31 chars + '\0'; todos os
// valores de summary_for_code() (open_meteo_provider.cpp) cabem com folga.
struct WeatherBlobV1 {
    double temperature_c;
    double precipitation_mm;
    char summary[32];
};

WeatherBlobV1 to_blob(const WeatherState& state) {
    WeatherBlobV1 blob{};
    blob.temperature_c = state.temperature_c;
    blob.precipitation_mm = state.precipitation_mm;
    std::strncpy(blob.summary, state.summary.c_str(), sizeof(blob.summary) - 1);
    blob.summary[sizeof(blob.summary) - 1] = '\0';
    return blob;
}

}  // namespace

WeatherService::WeatherService(StateStore& state_store, CacheStore& cache_store, IWeatherProvider& provider)
    : state_store_(state_store), cache_store_(cache_store), provider_(provider) {}

void WeatherService::load_from_cache(uint64_t now_ms) {
    const auto loaded = cache_store_.load(kCacheDomain, sizeof(WeatherBlobV1));
    if (!loaded.ok()) {
        return;
    }
    WeatherBlobV1 blob{};
    std::memcpy(&blob, loaded.value().data(), sizeof(blob));

    WeatherState state{};
    state.temperature_c = blob.temperature_c;
    state.precipitation_mm = blob.precipitation_mm;
    state.summary.assign(blob.summary);
    state.valid = true;
    state.stale = true;
    state.source = DataSource::Cache;
    state.last_update_ms = now_ms;
    state_store_.set_weather(state);
}

bool WeatherService::refresh_impl(uint64_t now_ms) {
    const UserPreferences preferences = state_store_.preferences();
    const auto result = provider_.fetch_weather(preferences.latitude, preferences.longitude);
    if (!result.ok()) {
#if defined(ESP_PLATFORM)
        ESP_LOGW(kTag, "fetch failed: %s", result.status().message().c_str());
#endif
        return false;
    }

    WeatherState state = result.value();
    state.valid = true;
    state.stale = false;
    state.source = DataSource::Live;
    state.last_update_ms = now_ms;
    state_store_.set_weather(state);

    const WeatherBlobV1 blob = to_blob(state);
    (void)cache_store_.save(kCacheDomain, &blob, sizeof(blob), now_ms);
    return true;
}

bool WeatherService::refresh(void* context, uint64_t now_ms) {
    return static_cast<WeatherService*>(context)->refresh_impl(now_ms);
}

}  // namespace nova
