#pragma once

#include <cstdint>

#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"
#include "cache_store.hpp"
#include "open_meteo_provider.hpp"

namespace nova {

// Atualiza o clima. Sem task propria: o NetworkWorker chama refresh()
// serializado e por prioridade (ADR-0035, fase 2).
class WeatherService final : public Service {
public:
    WeatherService(StateStore& store,
                   RequestOrchestrator& orchestrator,
                   CacheStore& cache,
                   OpenMeteoProvider& provider);

    const char* name() const override;
    bool init() override;

    bool refresh(uint32_t now_ms);

private:
    // Cache so serve p/ boot offline: throttla a persistencia p/ evitar erase
    // de flash a cada fetch (flash branco do MIPI-DSI por contencao no MSPI do
    // P4). Ver nota detalhada em market_service.hpp.
    static constexpr uint32_t kCacheSaveIntervalMs = 30u * 60u * 1000u;  // 30 min

    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    CacheStore&          cache_;
    OpenMeteoProvider&   provider_;
    uint32_t             last_cache_save_ms_{0};
};

}  // namespace nova
