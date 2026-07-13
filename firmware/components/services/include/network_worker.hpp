#pragma once

#include "request_orchestrator.hpp"
#include "state_store.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"  // must precede freertos/task.h
#include "freertos/task.h"
#else
using TaskHandle_t = void*;
#endif

namespace nova {

// Task unica que executa os fetches REST de forma serializada (ADR-0004):
// services de dados registram {dominio, refresh} em vez de terem task
// propria. A cada volta o worker escolhe, entre os fetchers "due" segundo o
// RequestOrchestrator, o de maior prioridade (empate = ordem de registro) e
// executa um por vez -- nunca dois HTTPS simultaneos no firmware.
class NetworkWorker final {
public:
    // ponteiro de funcao + contexto (PROIBIDO std::function global/estatico,
    // CLAUDE.md); retorna true em sucesso, false em falha (conta no breaker).
    using RefreshFn = bool (*)(void* context, uint64_t now_ms);

    NetworkWorker(StateStore& state_store, RequestOrchestrator& orchestrator);

    // Registra um dominio e a funcao que o atualiza. Chamar antes de start().
    void register_fetcher(RequestDomain domain, RefreshFn refresh, void* context);
    std::size_t fetcher_count() const { return fetchers_.size(); }

    void start();

    // True quando ha um transporte de rede com estacao conectada. Puro
    // (le apenas StateStore), exposto para teste host sem FreeRTOS.
    bool network_available() const;

    // Escolhe o fetcher devido de maior prioridade (RequestPriority::Critical
    // > Normal > Low); -1 se nenhum estiver pronto. Puro e host-testavel: nao
    // inicia nem finaliza o request, so decide.
    int select_fetcher_index(uint64_t now_ms) const;

private:
    struct Fetcher {
        RequestDomain domain{RequestDomain::Weather};
        RefreshFn refresh{nullptr};
        void* context{nullptr};
    };

    static void task_entry(void* arg);
    void run();

    StateStore& state_store_;
    RequestOrchestrator& orchestrator_;
    std::vector<Fetcher> fetchers_{};
    TaskHandle_t task_handle_{nullptr};
    bool started_{false};
};

}  // namespace nova
