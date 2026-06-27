#include "forex_service.hpp"

#if defined(ESP_PLATFORM)

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace nova {

namespace {
constexpr const char* kTag = "ForexService";
constexpr uint32_t kRetryDelayMs = 10000;
constexpr uint32_t kIdleDelayMs = 1000;
constexpr uint32_t kTaskStackWords = 6144;
constexpr int kTaskPriority = 4;
}

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

void ForexService::start()
{
    if (started_) {
        return;
    }
    started_ = true;
    BaseType_t ok = xTaskCreate(task_entry, "forex_service", kTaskStackWords, this, kTaskPriority,
                                &task_handle_);
    if (ok != pdPASS) {
        ESP_LOGE(kTag, "failed to create task");
        task_handle_ = nullptr;
        started_ = false;
    }
}

void ForexService::tick(uint32_t)
{
}

void ForexService::task_entry(void* arg)
{
    static_cast<ForexService*>(arg)->run();
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

void ForexService::run()
{
    while (true) {
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        orchestrator_.update_clock(now_ms);

        const auto state = store_.snapshot();
        if (state.onboarding.wifi_status != WifiConnectStatus::Connected) {
            vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
            continue;
        }

        if (!orchestrator_.can_request(DataDomain::Forex)) {
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

ForexService::ForexService(StateStore& store,
                           RequestOrchestrator& orchestrator,
                           CacheStore& cache,
                           IForexProvider& provider)
    : store_(store), orchestrator_(orchestrator), cache_(cache), provider_(provider)
{
}

const char* ForexService::name() const { return "ForexService"; }
bool ForexService::init() { return true; }
void ForexService::start() {}
void ForexService::tick(uint32_t) {}

}  // namespace nova

#endif
