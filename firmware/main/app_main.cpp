#include "action_queue.hpp"
#include "app_state.hpp"
#include "builtin_screens.hpp"
#include "event_bus.hpp"
#include "request_orchestrator.hpp"
#include "screen_registry.hpp"
#include "service_manager.hpp"
#include "state_store.hpp"
#include "ui_shell.hpp"
#include "ui_dispatcher.hpp"
#if defined(ESP_PLATFORM)
#include "waveshare_board.hpp"
#else
#include "mock_board.hpp"
#endif

#include "esp_log.h"

namespace {
constexpr const char* kTag = "NovaPanel";

}

extern "C" void app_main(void) {
#if defined(ESP_PLATFORM)
    nova::WaveshareBoard board;
#else
    nova::MockBoard board;
#endif
    nova::EventBus event_bus;
    nova::StateStore state_store(event_bus);
    nova::ActionQueue action_queue;
    nova::RequestOrchestrator request_orchestrator;
    nova::UiDispatcher ui_dispatcher(event_bus);
    nova::ServiceManager service_manager;
    nova::ScreenRegistry screen_registry;
    (void)nova::register_builtin_screens(screen_registry);
    nova::UiShell ui_shell(state_store, screen_registry);

    const nova::BoardStatus board_status = board.bring_up();
    nova::SystemState system{};
    system.valid = true;
    system.source = nova::DataSource::Mock;
    system.display_ready = board_status.display_ready;
    system.network_ready = board_status.network_ready;
    state_store.set_system(system);

    nova::UiState ui_state{};
    ui_state.shell_ready = board_status.display_ready;
    state_store.set_ui(ui_state);

    if (board_status.display_ready && board.lock_ui(100)) {
        ui_shell.render(ui_dispatcher.take_pending_mask() |
                        nova::ui_event_bit(nova::EventType::SystemChanged) |
                        nova::ui_event_bit(nova::EventType::ScreenChanged));
        state_store.navigate_to(nova::ScreenId::Home);
        ui_shell.render(ui_dispatcher.take_pending_mask());
        board.unlock_ui();
    }

    service_manager.tick();
    request_orchestrator.tick(0);
    action_queue.drain([](const nova::Action&) {});

    ESP_LOGI(kTag, "boot shell ready on %s", board.name());
}
