#pragma once

#include <cstdint>

#include "cache_store.hpp"
#include "i_forex_provider.hpp"
#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

// Atualiza o cambio USD/BRL. Sem task propria: o NetworkWorker chama refresh()
// serializado e por prioridade (ADR-0035, fase 2).
class ForexService final : public Service {
public:
    ForexService(StateStore& store,
                 RequestOrchestrator& orchestrator,
                 CacheStore& cache,
                 IForexProvider& provider);

    const char* name() const override;
    bool init() override;

    bool refresh(uint32_t now_ms);

private:
    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    CacheStore&          cache_;
    IForexProvider&      provider_;
};

}  // namespace nova
