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
    // Cache so serve p/ boot offline: throttla a persistencia p/ evitar erase
    // de flash a cada fetch (flash branco do MIPI-DSI por contencao no MSPI do
    // P4). Ver nota detalhada em market_service.hpp.
    static constexpr uint32_t kCacheSaveIntervalMs = 30u * 60u * 1000u;  // 30 min

    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    CacheStore&          cache_;
    IForexProvider&      provider_;
    uint32_t             last_cache_save_ms_{0};
};

}  // namespace nova
