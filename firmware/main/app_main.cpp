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
#include "esp_timer.h"
#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
#if defined(ESP_PLATFORM)
#include "waveshare_board.hpp"
#else
#include "mock_board.hpp"
#endif

#include "esp_log.h"

namespace {
constexpr const char* kTag = "NovaPanel";
constexpr uint64_t kBootSplashMs = 1500;
constexpr uint32_t kDisplayRetryDelayMs = 1000;

uint64_t now_ms() {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

template <typename BoardT>
nova::BoardStatus bring_up_display_with_backoff(BoardT& board, nova::StateStore& state_store) {
    while (true) {
        const nova::BoardStatus board_status = board.bring_up();
        if (board_status.display_ready) {
            return board_status;
        }

        nova::UiState ui_state = state_store.ui();
        ui_state.display_breadcrumb = true;
        ++ui_state.display_retry_count;
        state_store.set_ui(ui_state);
        ESP_LOGW(kTag, "display init retry=%lu", static_cast<unsigned long>(ui_state.display_retry_count));
#if defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(kDisplayRetryDelayMs));
#else
        break;
#endif
    }

    return board.status();
}

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

    const nova::BoardStatus board_status = bring_up_display_with_backoff(board, state_store);
    nova::SystemState system{};
    system.valid = true;
    system.source = nova::DataSource::Mock;
    system.display_ready = board_status.display_ready;
    system.network_ready = board_status.network_ready;
    state_store.set_system(system);

    nova::UiState ui_state{};
    ui_state.shell_ready = board_status.display_ready;
    ui_state.display_breadcrumb = !board_status.display_ready;
    state_store.set_ui(ui_state);

    if (board_status.display_ready && board.lock_ui(100)) {
        ui_shell.render(ui_dispatcher.take_pending_mask() |
                        nova::ui_event_bit(nova::EventType::SystemChanged) |
                        nova::ui_event_bit(nova::EventType::ScreenChanged));
        board.unlock_ui();
    }

    const uint64_t boot_started_ms = now_ms();
    while (true) {
        const uint64_t current_ms = now_ms();
        if (!state_store.ui().boot_complete &&
            state_store.system().display_ready &&
            current_ms >= boot_started_ms + kBootSplashMs) {
            state_store.navigate_to(nova::ScreenId::Home);
        }

        if (board.status().display_ready && board.lock_ui(50)) {
            const uint32_t pending_mask = ui_dispatcher.take_pending_mask();
            if (pending_mask != 0) {
                ui_shell.render(pending_mask);
            }
            board.unlock_ui();
        }

        service_manager.tick();
        request_orchestrator.tick(current_ms);
        action_queue.drain([](const nova::Action&) {});
#if defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(50));
#else
        break;
#endif
    }

    ESP_LOGI(kTag, "boot shell loop ready on %s", board.name());
}
