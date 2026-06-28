// NovaPanel - services/network_worker.cpp
#include "network_worker.hpp"

#include <utility>

#if defined(ESP_PLATFORM)

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace nova {

namespace {
constexpr const char* kTag = "NetworkWorker";
constexpr uint32_t kTaskStackWords = 8192;
constexpr int      kTaskPriority   = 4;
constexpr uint32_t kIdleDelayMs    = 1000;  // sem rede ou nada "due"
constexpr uint32_t kGapDelayMs     = 400;   // escalonamento entre buscas
}  // namespace

NetworkWorker::NetworkWorker(StateStore& store, RequestOrchestrator& orchestrator)
    : store_(store), orchestrator_(orchestrator)
{
}

const char* NetworkWorker::name() const
{
    return "NetworkWorker";
}

void NetworkWorker::register_fetcher(DataDomain domain, RefreshFn refresh)
{
    fetchers_.push_back(Fetcher{domain, std::move(refresh)});
}

void NetworkWorker::start()
{
    if (started_) {
        return;
    }
    started_ = true;
    BaseType_t ok = xTaskCreate(task_entry, "net_worker", kTaskStackWords, this,
                                kTaskPriority, &task_handle_);
    if (ok != pdPASS) {
        ESP_LOGE(kTag, "failed to create task");
        task_handle_ = nullptr;
        started_ = false;
    }
}

void NetworkWorker::task_entry(void* arg)
{
    static_cast<NetworkWorker*>(arg)->run();
}

void NetworkWorker::run()
{
    ESP_LOGI(kTag, "started: %u fetchers, serialized by priority",
             static_cast<unsigned>(fetchers_.size()));
    while (true) {
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        orchestrator_.update_clock(now_ms);

        if (store_.wifi_status() != WifiConnectStatus::Connected) {
            vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
            continue;
        }

        // Escolhe o fetcher de maior prioridade que pode requisitar agora.
        // Menor ordinal de RequestPriority = maior prioridade. Empate vai
        // para a ordem de registro (escalona o boot de forma estavel).
        const Fetcher* chosen = nullptr;
        int best_priority = 1000;
        for (const Fetcher& f : fetchers_) {
            if (!orchestrator_.can_request(f.domain)) {
                continue;
            }
            const int prio = static_cast<int>(orchestrator_.priority_for(f.domain));
            if (prio < best_priority) {
                best_priority = prio;
                chosen = &f;
            }
        }

        if (chosen == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
            continue;
        }

        chosen->refresh(now_ms);                 // serializado: 1 HTTPS por vez
        vTaskDelay(pdMS_TO_TICKS(kGapDelayMs));   // escalona o proximo
    }
}

}  // namespace nova

#else  // host build: sem task / FreeRTOS

namespace nova {

NetworkWorker::NetworkWorker(StateStore& store, RequestOrchestrator& orchestrator)
    : store_(store), orchestrator_(orchestrator)
{
}

const char* NetworkWorker::name() const { return "NetworkWorker"; }

void NetworkWorker::register_fetcher(DataDomain domain, RefreshFn refresh)
{
    fetchers_.push_back(Fetcher{domain, std::move(refresh)});
}

void NetworkWorker::start() {}

}  // namespace nova

#endif
