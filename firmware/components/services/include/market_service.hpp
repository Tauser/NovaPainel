#pragma once

#include "cache_store.hpp"
#include "provider_interfaces.hpp"
#include "state_store.hpp"

#include <cstdint>

namespace nova {

// Dono do domínio "market" do StateStore: funde as duas fontes (BTC spot via
// IMarketProvider, USD/BRL via IForexProvider) num único MarketState, já que
// hoje o modelo trata os dois como um domínio de dado só (AppState.market).
// Cada fetch atualiza só o campo que a fonte conhece -- nunca zera o outro.
class MarketService {
public:
    MarketService(StateStore& state_store, CacheStore& cache_store,
                  IMarketProvider& crypto_provider, IForexProvider& forex_provider);

    // Repõe o que houver em cache (stale=true, source=Cache) antes do
    // primeiro fetch de rede -- boot offline mostra dado velho sinalizado
    // em vez de vazio (critério de saída da Fase 4).
    void load_from_cache(uint64_t now_ms);

    // Adaptadores NetworkWorker::RefreshFn (ponteiro de função + contexto).
    static bool refresh_crypto(void* context, uint64_t now_ms);
    static bool refresh_forex(void* context, uint64_t now_ms);

private:
    bool refresh_crypto_impl(uint64_t now_ms);
    bool refresh_forex_impl(uint64_t now_ms);

    StateStore& state_store_;
    CacheStore& cache_store_;
    IMarketProvider& crypto_provider_;
    IForexProvider& forex_provider_;
};

}  // namespace nova
