#pragma once

#include <cstdint>

#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"
#include "cache_store.hpp"
#include "coingecko_provider.hpp"

namespace nova {

// Atualiza a cotacao de cripto. Nao roda task propria: o NetworkWorker chama
// refresh() de forma serializada e por prioridade (ADR-0035, fase 2).
class MarketService final : public Service {
public:
    MarketService(StateStore& store,
                  RequestOrchestrator& orchestrator,
                  CacheStore& cache,
                  CoinGeckoProvider& provider);

    const char* name() const override;
    bool init() override;

    // Chamado pelo NetworkWorker quando o dominio esta "due".
    bool refresh(uint32_t now_ms);

private:
    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    CacheStore&          cache_;
    CoinGeckoProvider&   provider_;
};

}  // namespace nova
