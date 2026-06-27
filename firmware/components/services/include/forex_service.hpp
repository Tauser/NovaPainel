#pragma once

#include <cstdint>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
using TaskHandle_t = void*;
#endif

#include "cache_store.hpp"
#include "i_forex_provider.hpp"
#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class ForexService final : public Service {
public:
    ForexService(StateStore& store,
                 RequestOrchestrator& orchestrator,
                 CacheStore& cache,
                 IForexProvider& provider);

    const char* name() const override;
    bool init() override;
    void start() override;
    void tick(uint32_t now_ms) override;

private:
    static void task_entry(void* arg);
    void run();
    bool refresh(uint32_t now_ms);

    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    CacheStore&          cache_;
    IForexProvider&      provider_;
    TaskHandle_t         task_handle_{nullptr};
    bool                 started_{false};
};

}  // namespace nova
