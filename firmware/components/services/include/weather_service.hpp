#pragma once

#include <cstdint>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
using TaskHandle_t = void*;
#endif

#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"
#include "cache_store.hpp"
#include "open_meteo_provider.hpp"

namespace nova {

class WeatherService final : public Service {
public:
    WeatherService(StateStore& store,
                   RequestOrchestrator& orchestrator,
                   CacheStore& cache,
                   OpenMeteoProvider& provider);

    const char* name() const override;
    bool init() override;
    void start() override;
    void tick(uint32_t now_ms) override;

private:
    static void task_entry(void* arg);
    void run();
    bool refresh(uint32_t now_ms);

    StateStore&           store_;
    RequestOrchestrator&   orchestrator_;
    CacheStore&           cache_;
    OpenMeteoProvider&     provider_;
    TaskHandle_t          task_handle_{nullptr};
    bool                  started_{false};
};

}  // namespace nova
