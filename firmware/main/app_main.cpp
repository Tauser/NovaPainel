#include "action_queue.hpp"
#include "app_state.hpp"
#include "event_bus.hpp"
#include "mock_board.hpp"
#include "request_orchestrator.hpp"
#include "screen_registry.hpp"
#include "service_manager.hpp"
#include "state_store.hpp"
#include "ui_dispatcher.hpp"

#include "esp_log.h"

namespace {
constexpr const char* kTag = "NovaPanel";

void render_pending_ui() {
    ESP_LOGI(kTag, "ui refresh");
}
}

extern "C" void app_main(void) {
    nova::MockBoard board;
    nova::EventBus event_bus;
    nova::StateStore state_store(event_bus);
    nova::ActionQueue action_queue;
    nova::RequestOrchestrator request_orchestrator;
    nova::UiDispatcher ui_dispatcher(event_bus);
    nova::ServiceManager service_manager;
    nova::ScreenRegistry screen_registry;

    const nova::BoardStatus board_status = board.bring_up();
    nova::SystemState system{};
    system.valid = true;
    system.source = nova::DataSource::Mock;
    system.display_ready = board_status.display_ready;
    system.network_ready = board_status.network_ready;
    state_store.set_system(system);

    ui_dispatcher.process_pending(&render_pending_ui);

    service_manager.tick();
    request_orchestrator.tick(0);
    action_queue.drain([](const nova::Action&) {});

    ESP_LOGI(kTag, "boot skeleton ready on %s", board.name());
}
