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

    // Busca dados OHLC. Chamado a cada kOhlcIntervalMs pelo NetworkWorker.
    bool refresh_ohlc(uint32_t now_ms, OhlcPeriod period = OhlcPeriod::D1);

    // Muda o periodo do grafico e forca re-fetch imediato no proximo refresh().
    // Seguro chamar de qualquer task (apenas seta flags, sem IO).
    void request_ohlc_period(OhlcPeriod period);

private:
    static constexpr uint32_t kOhlcIntervalMs = 5u * 60u * 1000u;  // 5 min

    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    CacheStore&          cache_;
    CoinGeckoProvider&   provider_;
    uint32_t             last_ohlc_ms_{0};
    OhlcPeriod           pending_period_{OhlcPeriod::D1};  // period to fetch next
};

}  // namespace nova
