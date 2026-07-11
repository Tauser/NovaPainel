// NovaPanel - services/network_worker.hpp
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
using TaskHandle_t = void*;
#endif

#include "request_orchestrator.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

// Task unica que executa as buscas REST de forma serializada e por prioridade
// (ADR-0035, fase 2). Substitui as tasks proprias de cada service: eles
// registram um fetcher {dominio, refresh}. A cada ciclo o worker escolhe o
// dominio de maior prioridade que esta "due" (RequestOrchestrator::can_request)
// e executa um de cada vez (1 HTTPS por vez, escalonado no boot).
class NetworkWorker final : public Service {
public:
    using RefreshFn = std::function<bool(uint32_t now_ms)>;

    NetworkWorker(StateStore& store, RequestOrchestrator& orchestrator);

    // Registra um dominio e a funcao que o atualiza. Chamar antes de start().
    void register_fetcher(DataDomain domain, RefreshFn refresh);

    const char* name() const override;
    void start() override;

private:
    struct Fetcher {
        DataDomain domain;
        RefreshFn  refresh;
    };

    static void task_entry(void* arg);
    void run();

    StateStore&          store_;
    RequestOrchestrator& orchestrator_;
    std::vector<Fetcher> fetchers_;
    TaskHandle_t         task_handle_{nullptr};
    bool                 started_{false};
};

}  // namespace nova
