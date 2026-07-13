#pragma once

#include "cache_store.hpp"
#include "provider_interfaces.hpp"
#include "state_store.hpp"

#include <cstdint>

namespace nova {

// Dono do domínio "weather" do StateStore. Lê latitude/longitude de
// UserPreferences a cada fetch (em vez de fixar no construtor) para já
// funcionar quando o onboarding aprender a coletar essa preferência.
class WeatherService {
public:
    WeatherService(StateStore& state_store, CacheStore& cache_store, IWeatherProvider& provider);

    // Repõe o StateStore a partir do CacheStore (stale=true, source=Cache)
    // antes do primeiro fetch de rede -- boot offline mostra dado velho
    // sinalizado (critério de saída da Fase 4).
    void load_from_cache(uint64_t now_ms);

    // Adaptador NetworkWorker::RefreshFn (ponteiro de função + contexto).
    static bool refresh(void* context, uint64_t now_ms);

private:
    bool refresh_impl(uint64_t now_ms);

    StateStore& state_store_;
    CacheStore& cache_store_;
    IWeatherProvider& provider_;
};

}  // namespace nova
