#include "weather_service.hpp"

#if defined(ESP_PLATFORM)

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace nova {

namespace {
constexpr const char* kTag = "WeatherService";
constexpr uint32_t kRetryDelayMs = 10000;
constexpr uint32_t kIdleDelayMs = 1000;
constexpr uint32_t kTaskStackWords = 8192;
constexpr int kTaskPriority = 4;
}

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

void WeatherService::start()
{
    if (started_) {
        return;
    }
    started_ = true;
    BaseType_t ok = xTaskCreate(task_entry, "weather_service", kTaskStackWords, this, kTaskPriority,
                                reinterpret_cast<TaskHandle_t*>(&task_handle_));
    if (ok != pdPASS) {
        ESP_LOGE(kTag, "failed to create task");
        task_handle_ = nullptr;
        started_ = false;
    }
}

void WeatherService::tick(uint32_t)
{
}

void WeatherService::task_entry(void* arg)
{
    static_cast<WeatherService*>(arg)->run();
}

bool WeatherService::refresh(uint32_t now_ms)
{
    WeatherSummary weather = store_.snapshot().weather;
    if (!provider_.fetch(weather, now_ms)) {
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

void WeatherService::run()
{
    while (true) {
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        orchestrator_.update_clock(now_ms);

        const auto state = store_.snapshot();
        if (state.onboarding.wifi_status != WifiConnectStatus::Connected) {
            vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
            continue;
        }

        if (!orchestrator_.can_request(DataDomain::Weather)) {
            vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
            continue;
        }

        if (!refresh(now_ms)) {
            vTaskDelay(pdMS_TO_TICKS(kRetryDelayMs));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(kRetryDelayMs));
    }
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
void WeatherService::start() {}
void WeatherService::tick(uint32_t) {}

}  // namespace nova

#endif
