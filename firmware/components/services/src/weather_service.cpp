#include "weather_service.hpp"

#if defined(ESP_PLATFORM)

namespace nova {

WeatherService::WeatherService(StateStore& store,
                               RequestOrchestrator& orchestrator,
                               CacheStore& cache,
                               OpenMeteoProvider& provider)
    : store_(store), orchestrator_(orchestrator), cache_(cache), provider_(provider)
{
}

const char* WeatherService::name() const
{
    return "WeatherService";
}

bool WeatherService::init()
{
    WeatherSummary cached{};
    if (cache_.load_weather(cached)) {
        cached.stale = true;
        cached.source = DataSource::Cache;
        store_.set_weather(cached);
    }
    return true;
}

bool WeatherService::refresh(uint32_t now_ms)
{
    WeatherSummary weather = store_.weather();
    const UserPreferences preferences = store_.preferences();
    if (!provider_.fetch(weather, now_ms, preferences)) {
        WeatherSummary cached{};
        if (cache_.load_weather(cached)) {
            cached.stale = true;
            cached.source = DataSource::Cache;
            store_.set_weather(cached);
        }
        orchestrator_.note_request(DataDomain::Weather, now_ms, false);
        return false;
    }

    store_.set_weather(weather);
    cache_.save_weather(weather);
    orchestrator_.note_request(DataDomain::Weather, now_ms, true);
    return true;
}

}  // namespace nova

#else

namespace nova {

WeatherService::WeatherService(StateStore& store,
                               RequestOrchestrator& orchestrator,
                               CacheStore& cache,
                               OpenMeteoProvider& provider)
    : store_(store), orchestrator_(orchestrator), cache_(cache), provider_(provider)
{
}

const char* WeatherService::name() const { return "WeatherService"; }
bool WeatherService::init() { return true; }
bool WeatherService::refresh(uint32_t) { return false; }

}  // namespace nova

#endif
