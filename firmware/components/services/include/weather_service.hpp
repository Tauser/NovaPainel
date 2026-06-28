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
    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    CacheStore&          cache_;
    OpenMeteoProvider&   provider_;
};

}  // namespace nova
