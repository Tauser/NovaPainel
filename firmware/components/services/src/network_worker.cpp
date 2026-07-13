#include "network_worker.hpp"

#if defined(ESP_PLATFORM)
#include "esp_log.h"
#include "esp_timer.h"
#endif

namespace nova {
namespace {
#if defined(ESP_PLATFORM)
constexpr const char* kTag = "NetworkWorker";
constexpr uint32_t kTaskStackWords = 8192;  // RESOURCE-BUDGET.md: net_worker
constexpr int kTaskPriority = 4;
constexpr uint32_t kIdleDelayMs = 1000;  // sem rede ou nada devido
constexpr uint32_t kGapDelayMs = 400;    // escalona o proximo fetch
#endif
}  // namespace

NetworkWorker::NetworkWorker(StateStore& state_store, RequestOrchestrator& orchestrator)
    : state_store_(state_store), orchestrator_(orchestrator) {}

void NetworkWorker::register_fetcher(RequestDomain domain, RefreshFn refresh, void* context) {
    fetchers_.push_back(Fetcher{domain, refresh, context});
}

bool NetworkWorker::network_available() const {
    return state_store_.setup().wifi_connect_status == WifiConnectStatus::Connected;
}

int NetworkWorker::select_fetcher_index(uint64_t now_ms) const {
    int chosen = -1;
    int best_priority = -1;  // RequestPriority: maior ordinal = maior prioridade (Critical > Normal > Low)
    for (std::size_t i = 0; i < fetchers_.size(); ++i) {
        const Fetcher& fetcher = fetchers_[i];
        if (!orchestrator_.can_start(fetcher.domain, now_ms)) {
            continue;
        }
        const int priority = static_cast<int>(orchestrator_.priority_for(fetcher.domain));
        if (priority > best_priority) {
            best_priority = priority;
            chosen = static_cast<int>(i);
        }
    }
    return chosen;
}

#if defined(ESP_PLATFORM)

void NetworkWorker::start() {
    if (started_) {
        return;
    }
    started_ = true;
    const BaseType_t ok = xTaskCreate(task_entry, "net_worker", kTaskStackWords, this,
                                       kTaskPriority, &task_handle_);
    if (ok != pdPASS) {
        ESP_LOGE(kTag, "failed to create task");
        task_handle_ = nullptr;
        started_ = false;
    }
}

void NetworkWorker::task_entry(void* arg) {
    static_cast<NetworkWorker*>(arg)->run();
}

void NetworkWorker::run() {
    ESP_LOGI(kTag, "started: %u fetcher(s), serialized 1 HTTPS per vez",
             static_cast<unsigned>(fetchers_.size()));
    while (true) {
        const uint64_t now_ms = static_cast<uint64_t>(esp_timer_get_time() / 1000);
        // Unico dono do RequestOrchestrator (ADR-0004): main_loop nao toca
        // este objeto para evitar acesso concorrente sem lock (regra 1,
        // Secao 6 do ARCHITECTURE.md).
        orchestrator_.tick(now_ms);

        if (!network_available()) {
            vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
            continue;
        }

        const int index = select_fetcher_index(now_ms);
        if (index < 0) {
            vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
            continue;
        }

        const Fetcher& fetcher = fetchers_[static_cast<std::size_t>(index)];
        if (!orchestrator_.mark_started(fetcher.domain, now_ms).ok()) {
            continue;  // perdeu a corrida para outro criterio de gate; tenta de novo
        }
        const bool ok = fetcher.refresh(fetcher.context, now_ms);
        const uint64_t finished_ms = static_cast<uint64_t>(esp_timer_get_time() / 1000);
        orchestrator_.mark_finished(fetcher.domain, ok, finished_ms);
        vTaskDelay(pdMS_TO_TICKS(kGapDelayMs));
    }
}

#else  // host build: sem task/FreeRTOS

void NetworkWorker::start() {}

void NetworkWorker::task_entry(void*) {}

void NetworkWorker::run() {}

#endif

}  // namespace nova
