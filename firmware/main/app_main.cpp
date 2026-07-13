#include "action_queue.hpp"
#include "app_state.hpp"
#include "boot_breadcrumb_store.hpp"
#include "builtin_screens.hpp"
#include "event_bus.hpp"
#include "request_orchestrator.hpp"
#include "screen_registry.hpp"
#include "clock_service.hpp"
#include "network_worker.hpp"
#include "setup_service.hpp"
#include "setup_screen.hpp"
#include "service_manager.hpp"
#include "state_store.hpp"
#include "boot_recovery_policy.hpp"
#include "ui_shell.hpp"
#include "ui_dispatcher.hpp"
#include "esp_timer.h"
#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
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
constexpr nova::BootRecoveryConfig kBootRecoveryConfig{};

nova::SetupService* g_setup_service = nullptr;
nova::StateStore* g_state_store = nullptr;

void handle_action(const nova::Action& action) {
    if (g_setup_service == nullptr || g_state_store == nullptr) {
        return;
    }

    switch (action.type) {
        case nova::ActionType::SetupRequestWifiScan:
            g_setup_service->request_wifi_scan();
            break;
        case nova::ActionType::SetupSetStep:
            // Step-order/back-navigation logic lives in SetupService, not
            // here: main/ is wiring only (CLAUDE.md rule 5).
            g_setup_service->go_back();
            break;
        case nova::ActionType::SetupSubmit: {
            nova::OnboardingSubmission submission{};
            submission.step = action.setup_submission.step;
            submission.use_24h = action.setup_submission.use_24h;
            submission.wifi_ssid = action.setup_submission.wifi_ssid;
            submission.wifi_password = action.setup_submission.wifi_password;
            submission.timezone = action.setup_submission.timezone;
            g_setup_service->submit_onboarding(submission);
            break;
        }
        default:
            break;
    }
}

uint64_t now_ms() {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

struct DisplayBringUpResult {
    nova::BoardStatus status{};
    uint32_t attempts_this_boot{0};
};

template <typename BoardT>
DisplayBringUpResult bring_up_display_with_backoff(
    BoardT& board,
    nova::StateStore& state_store,
    nova::BootBreadcrumbStore& breadcrumb_store) {
    uint32_t attempts_this_boot = 0;
    while (true) {
        const nova::BoardStatus board_status = board.bring_up();
        if (board_status.display_ready) {
            if (!breadcrumb_store.clear().ok()) {
                ESP_LOGW(kTag, "failed to clear boot breadcrumb");
            }
            return DisplayBringUpResult{board_status, attempts_this_boot};
        }

        const nova::DisplayFailureDecision decision =
            nova::on_display_failure(state_store.ui(), attempts_this_boot + 1, kBootRecoveryConfig);
        state_store.set_ui(decision.next_ui_state);
        (void)breadcrumb_store.save(nova::BootBreadcrumb{
            .display_breadcrumb = true,
            .display_retry_count = decision.next_ui_state.display_retry_count,
        });
        ESP_LOGW(kTag, "display init retry=%lu",
                 static_cast<unsigned long>(decision.next_ui_state.display_retry_count));
        ++attempts_this_boot;
#if defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(decision.delay_ms));
        if (decision.restart_required) {
            ESP_LOGE(kTag, "display init exceeded %lu attempts, restarting",
                     static_cast<unsigned long>(attempts_this_boot));
            esp_restart();
        }
#else
        break;
#endif
    }

    return DisplayBringUpResult{board.status(), attempts_this_boot};
}

}

extern "C" void app_main(void) {
#if defined(ESP_PLATFORM)
    static nova::WaveshareBoard board;
    constexpr nova::DataSource kClockSource = nova::DataSource::Live;
#else
    static nova::MockBoard board;
    constexpr nova::DataSource kClockSource = nova::DataSource::Mock;
#endif
    static nova::EventBus event_bus;
    static nova::StateStore state_store(event_bus);
    static nova::BootBreadcrumbStore breadcrumb_store;
    static nova::ActionQueue action_queue;
    static nova::RequestOrchestrator request_orchestrator;
    static nova::UiDispatcher ui_dispatcher(event_bus);
    static nova::ServiceManager service_manager;
    static nova::ClockService clock_service(state_store, board, kClockSource);
    static nova::SetupService setup_service(state_store, board);
    static nova::NetworkWorker network_worker(state_store, request_orchestrator);
    static nova::ScreenRegistry screen_registry;
    g_setup_service = &setup_service;
    g_state_store = &state_store;
    nova::bind_setup_screen_action_queue(&action_queue);
    (void)nova::register_builtin_screens(screen_registry);
    static nova::UiShell ui_shell(state_store, screen_registry);
    (void)service_manager.add(&nova::ClockService::tick_adapter, &clock_service);
    (void)service_manager.add(&nova::SetupService::tick_adapter, &setup_service);
    // Sem fetchers registrados ainda (providers entram em seguida na Fase 4);
    // o worker sobe e fica ocioso, provando task/gate de rede sem custo.
    network_worker.start();
    const auto breadcrumb_init = breadcrumb_store.init();
    if (!breadcrumb_init.ok()) {
        ESP_LOGW(kTag, "boot breadcrumb init failed");
    }

    const auto persisted_breadcrumb = breadcrumb_store.load();
    if (persisted_breadcrumb.ok()) {
        nova::UiState restored_ui{};
        restored_ui.display_breadcrumb = persisted_breadcrumb.value().display_breadcrumb;
        restored_ui.display_retry_count = persisted_breadcrumb.value().display_retry_count;
        state_store.set_ui(restored_ui);
    }

    const DisplayBringUpResult bring_up_result =
        bring_up_display_with_backoff(board, state_store, breadcrumb_store);
    const nova::BoardStatus board_status = bring_up_result.status;
    nova::SystemState system{};
    system.valid = true;
    system.source = kClockSource;
    system.display_ready = board_status.display_ready;
    system.network_ready = board_status.network_ready;
    state_store.set_system(system);

    nova::UiState ui_state{};
    ui_state.shell_ready = board_status.display_ready;
    ui_state.display_breadcrumb = bring_up_result.attempts_this_boot > 0;
    ui_state.display_retry_count = bring_up_result.attempts_this_boot;
    state_store.set_ui(ui_state);

    // Load persisted preferences (brightness included) before the first
    // frame is backlit, so the initial backlight-on doesn't flash the
    // stock default brightness ahead of the user's saved value.
    setup_service.load_from_storage();

    if (board_status.display_ready && board.lock_ui(100)) {
        ui_shell.render(ui_dispatcher.take_pending_mask() |
                        nova::ui_event_bit(nova::EventType::SystemChanged) |
                        nova::ui_event_bit(nova::EventType::ScreenChanged));
        board.unlock_ui();
#if defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(250));
#endif
        board.set_brightness(state_store.preferences().brightness_pct);
        ESP_LOGI(kTag, "backlight enabled after first UI frame");
    }

    if (board_status.display_ready) {
        // Kicked off asynchronously: on real hardware this can block for
        // several seconds waiting on the ESP-Hosted/C6 link, and must not
        // freeze touch/action-queue processing in the loop below.
        board.start_network_transport_async();
    }

    const uint64_t boot_started_ms = now_ms();
    while (true) {
        const uint64_t current_ms = now_ms();
        if (!state_store.ui().boot_complete &&
            state_store.system().display_ready &&
            current_ms >= boot_started_ms + kBootSplashMs) {
            state_store.navigate_to(nova::ScreenId::Home);
        }

        const bool board_network_ready = board.status().network_ready;
        if (board_network_ready != state_store.system().network_ready) {
            system = state_store.system();
            system.network_ready = board_network_ready;
            state_store.set_system(system);
        }

        if (board.status().display_ready && board.lock_ui(50)) {
            const uint32_t pending_mask = ui_dispatcher.take_pending_mask();
            if (pending_mask != 0) {
                ui_shell.render(pending_mask);
            }
            board.unlock_ui();
        }

        service_manager.tick();
        // request_orchestrator.tick() is NOT called here: NetworkWorker's own
        // task is the sole owner/caller of RequestOrchestrator once it starts
        // (ADR-0004), so main_loop touching it too would be an unsynchronized
        // concurrent access (ARCHITECTURE.md Secao 6, regra 1).
        action_queue.drain(&handle_action);
#if defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(50));
#else
        break;
#endif
    }

    ESP_LOGI(kTag, "boot shell loop ready on %s", board.name());
}
