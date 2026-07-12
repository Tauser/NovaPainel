#include "action_queue.hpp"
#include "app_state.hpp"
#include "boot_breadcrumb_store.hpp"
#include "boot_recovery_policy.hpp"
#include "clock_service.hpp"
#include "event_bus.hpp"
#include "http_client.hpp"
#include "mock_board.hpp"
#include "request_orchestrator.hpp"
#include "screen_registry.hpp"
#include "setup_service.hpp"
#include "setup_storage_logic.hpp"
#include "setup_view_model.hpp"
#include "shell_chrome_view_model.hpp"
#include "state_store.hpp"
#include "timezone_catalog.hpp"
#include "boot_view_model.hpp"
#include "home_view_model.hpp"
#include "ui_dispatcher.hpp"

#include <cassert>
#include <cstdint>
#include <string>

namespace {

struct EventCounter {
    int count{0};
};

void count_event(const nova::Event&, void* context) {
    auto* counter = static_cast<EventCounter*>(context);
    ++counter->count;
}

int drained_actions = 0;

void count_action(const nova::Action&) {
    ++drained_actions;
}

int renders = 0;

void count_render() {
    ++renders;
}

}  // namespace

int main() {
    nova::EventBus event_bus;
    EventCounter counter{};
    assert(event_bus.subscribe(nova::EventType::SystemChanged, &count_event, &counter));

    nova::StateStore state_store(event_bus);
    nova::SystemState system{};
    system.valid = true;
    system.source = nova::DataSource::Mock;
    state_store.set_system(system);
    assert(counter.count == 1);
    assert(state_store.system().valid);
    assert(std::string(nova::to_string(state_store.system().source)) == "mock");
    state_store.navigate_to(nova::ScreenId::Home);
    assert(state_store.ui().active_screen == nova::ScreenId::Home);

    nova::UiDispatcher dispatcher(event_bus);
    state_store.set_action_queue_overflows(3);
    assert(dispatcher.pending_mask() != 0);
    dispatcher.process_pending(&count_render);
    assert(renders == 1);
    assert(dispatcher.pending_mask() == 0);
    state_store.set_action_queue_overflows(4);
    assert(dispatcher.pending_mask() != 0);

    nova::ActionQueue queue;
    for (int i = 0; i < 16; ++i) {
        assert(queue.push(nova::Action{nova::ActionType::Refresh, i}));
    }
    assert(!queue.push(nova::Action{nova::ActionType::Refresh, 17}));
    assert(queue.overflow_count() == 1);
    queue.drain(&count_action);
    assert(drained_actions == 16);
    assert(queue.size() == 0);
    state_store.set_action_queue_overflows(queue.overflow_count());
    assert(state_store.system().action_queue_overflows == 1);

    nova::RequestOrchestrator orchestrator;
    assert(orchestrator.configure(nova::RequestPolicy{
        nova::RequestDomain::Weather, nova::RequestPriority::Normal, 1000, 6, 3, 30000, 30000, true}).ok());
    assert(orchestrator.can_start(nova::RequestDomain::Weather, 10));
    assert(orchestrator.mark_started(nova::RequestDomain::Weather, 10).ok());
    assert(orchestrator.in_flight());
    assert(!orchestrator.can_start(nova::RequestDomain::Weather, 500));
    orchestrator.mark_finished(nova::RequestDomain::Weather, true, 20);
    assert(orchestrator.can_start(nova::RequestDomain::Weather, 1010));

    assert(orchestrator.configure(nova::RequestPolicy{
        nova::RequestDomain::MarketSpot, nova::RequestPriority::Normal, 1, 2, 2, 1000, 2000, true}).ok());
    assert(orchestrator.mark_started(nova::RequestDomain::MarketSpot, 2000).ok());
    orchestrator.mark_finished(nova::RequestDomain::MarketSpot, true, 2010);
    assert(!orchestrator.mark_started(nova::RequestDomain::MarketSpot, 2100).ok());
    assert(orchestrator.mark_started(nova::RequestDomain::MarketSpot, 61000).ok());
    orchestrator.mark_finished(nova::RequestDomain::MarketSpot, false, 61010);
    assert(!orchestrator.can_start(nova::RequestDomain::MarketSpot, 61410));
    assert(orchestrator.mark_started(nova::RequestDomain::MarketSpot, 62001).ok());
    orchestrator.mark_finished(nova::RequestDomain::MarketSpot, false, 62010);
    assert(orchestrator.circuit_state(nova::RequestDomain::MarketSpot) == nova::CircuitState::Open);
    assert(!orchestrator.can_start(nova::RequestDomain::MarketSpot, 62500));
    orchestrator.tick(63010);
    assert(orchestrator.circuit_state(nova::RequestDomain::MarketSpot) == nova::CircuitState::HalfOpen);

    nova::HttpClient http_client;
    const auto too_large = http_client.get("https://example.invalid/data", nova::kHttpBodyCapBytes + 1);
    assert(!too_large.ok());
    assert(too_large.status().code() == nova::StatusCode::Truncated);

    nova::MockBoard board;
    const nova::BoardStatus status = board.bring_up();
    assert(status.display_ready);
    assert(board.lock_shared_i2c(10));
    board.unlock_shared_i2c();

    nova::ClockService clock_service(state_store, board, nova::DataSource::Mock);
    clock_service.tick();
    assert(!state_store.clock().valid);
    board.set_rtc_unix_time_s(1704067200ULL + 60ULL);
    clock_service.tick();
    assert(state_store.clock().valid);
    assert(state_store.clock().source == nova::DataSource::Mock);
    assert(state_store.clock().unix_time_s == 1704067260ULL);

    nova::SetupService setup_service(state_store, board);
    setup_service.tick();
    assert(state_store.setup().valid);
    assert(state_store.setup().onboarding_required);
    assert(!state_store.setup().wifi_configured);
    assert(!state_store.setup().transport_ready);
    assert(state_store.setup().wifi_connect_status == nova::WifiConnectStatus::Idle);
    assert(state_store.setup().wifi_scan_status == nova::WifiScanStatus::Idle);
    assert(state_store.preferences().brightness_pct == board.brightness_pct());
    setup_service.request_wifi_scan();
    assert(state_store.setup().wifi_scan_status == nova::WifiScanStatus::Failed);
    assert(!state_store.setup().scan_in_progress);

    const nova::SetupStorageLoadResult empty_storage =
        nova::resolve_persisted_setup(false, 0, nova::PersistedSetupData{});
    assert(empty_storage.schema_supported);
    assert(empty_storage.should_initialize_schema);
    assert(empty_storage.setup.onboarding_required);

    nova::PersistedSetupData persisted_setup{};
    persisted_setup.has_onboarding_done = true;
    persisted_setup.onboarding_done = true;
    persisted_setup.has_brightness_pct = true;
    persisted_setup.brightness_pct = 35;
    persisted_setup.has_use_24h = true;
    persisted_setup.use_24h = false;
    persisted_setup.has_timezone = true;
    persisted_setup.timezone = "UTC";
    persisted_setup.has_wifi_ssid = true;
    persisted_setup.wifi_ssid = "NovaNet";
    persisted_setup.has_wifi_password = true;
    persisted_setup.wifi_password = "segredo";
    const nova::SetupStorageLoadResult supported_storage =
        nova::resolve_persisted_setup(true, nova::kCurrentSetupSchemaVersion, persisted_setup);
    assert(supported_storage.schema_supported);
    assert(!supported_storage.should_initialize_schema);
    assert(!supported_storage.setup.onboarding_required);
    assert(supported_storage.setup.wifi_configured);
    assert(supported_storage.preferences.brightness_pct == 35);
    assert(!supported_storage.preferences.use_24h);
    assert(supported_storage.preferences.timezone == "UTC");
    assert(supported_storage.saved_wifi_ssid == "NovaNet");
    assert(supported_storage.saved_wifi_password == "segredo");

    const nova::SetupStorageLoadResult future_schema =
        nova::resolve_persisted_setup(true, nova::kCurrentSetupSchemaVersion + 1, persisted_setup);
    assert(!future_schema.schema_supported);
    assert(future_schema.setup.onboarding_required);
    assert(future_schema.preferences.timezone == "America/Sao_Paulo");
    assert(nova::timezone_option_count() == 5);
    assert(std::string(nova::timezone_option_at(0).name) == "America/Sao_Paulo");
    assert(nova::find_timezone_option_index("America/Rio_Branco") == 2);
    assert(nova::find_timezone_option_index("Mars/Olympus") ==
           nova::default_timezone_option_index());

    nova::BootBreadcrumbStore breadcrumb_store;
    assert(breadcrumb_store.init().ok());
    assert(breadcrumb_store.save(nova::BootBreadcrumb{true, 3}).ok());
    const auto breadcrumb = breadcrumb_store.load();
    assert(breadcrumb.ok());
    assert(breadcrumb.value().display_breadcrumb);
    assert(breadcrumb.value().display_retry_count == 3);
    assert(breadcrumb_store.clear().ok());

    const nova::DisplayFailureDecision first_failure =
        nova::on_display_failure(nova::UiState{}, 1);
    assert(first_failure.next_ui_state.display_breadcrumb);
    assert(first_failure.next_ui_state.display_retry_count == 1);
    assert(first_failure.delay_ms == 2000);
    assert(!first_failure.restart_required);

    nova::UiState second_state = first_failure.next_ui_state;
    const nova::DisplayFailureDecision third_failure =
        nova::on_display_failure(second_state, 3);
    assert(third_failure.next_ui_state.display_retry_count == 2);
    assert(third_failure.delay_ms == 4000);
    assert(third_failure.restart_required);

    nova::ScreenRegistry registry;
    const auto build_stub = +[](lv_obj_t*) -> lv_obj_t* { return nullptr; };
    const auto update_stub = +[](const nova::AppState&) {};
    assert(registry.register_screen(nova::ScreenSpec{
        nova::ScreenId::Boot, "Boot", 1u, build_stub, update_stub, nullptr, nullptr}));
    assert(registry.register_screen(nova::ScreenSpec{
        nova::ScreenId::Home, "Home", 2u, build_stub, update_stub, nullptr, nullptr}));
    assert(registry.register_screen(nova::ScreenSpec{
        nova::ScreenId::Setup, "Setup", 4u, build_stub, update_stub, nullptr, nullptr}));
    assert(registry.size() == 3);
    assert(registry.find(nova::ScreenId::Home) != nullptr);
    assert(!registry.register_screen(nova::ScreenSpec{
        nova::ScreenId::Home, "Dup", 2u, build_stub, update_stub, nullptr, nullptr}));

    const nova::BootViewModel boot_vm = nova::make_boot_view_model(state_store.snapshot());
    assert(std::string(boot_vm.headline) == "Display pronto");
    system.display_ready = true;
    state_store.set_system(system);
    const nova::HomeViewModel home_vm = nova::make_home_view_model(state_store.snapshot());
    assert(std::string(home_vm.title) == "Inicio");
    assert(std::string(home_vm.status) == "Display ativo");

    state_store.set_clock(nova::ClockState{});
    const nova::ShellChromeViewModel chrome_without_clock =
        nova::make_shell_chrome_view_model(state_store.snapshot());
    assert(chrome_without_clock.status_line.find("sem hora") != std::string::npos);
    assert(chrome_without_clock.status_line.find("sem rede") != std::string::npos);

    nova::ClockState clock{};
    clock.valid = true;
    clock.unix_time_s = 13 * 3600 + 45 * 60;
    state_store.set_clock(clock);
    system.network_ready = true;
    state_store.set_system(system);

    nova::UserPreferences utc_preferences = state_store.preferences();
    utc_preferences.timezone = "UTC";
    state_store.set_preferences(utc_preferences);
    const nova::ShellChromeViewModel chrome_utc =
        nova::make_shell_chrome_view_model(state_store.snapshot());
    assert(chrome_utc.status_line.find("13:45") != std::string::npos);
    assert(chrome_utc.status_line.find("rede ok") != std::string::npos);

    // America/Sao_Paulo is UTC-3, so 13:45 UTC must render as 10:45 local --
    // the topbar clock has to respect preferences.timezone, not raw UTC.
    nova::UserPreferences sp_preferences = state_store.preferences();
    sp_preferences.timezone = "America/Sao_Paulo";
    state_store.set_preferences(sp_preferences);
    const nova::ShellChromeViewModel chrome_with_clock =
        nova::make_shell_chrome_view_model(state_store.snapshot());
    assert(chrome_with_clock.status_line.find("10:45") != std::string::npos);
    assert(chrome_with_clock.status_line.find("rede ok") != std::string::npos);

    const nova::SetupViewModel setup_vm = nova::make_setup_view_model(state_store.snapshot());
    assert(std::string(setup_vm.title) == "Setup");

    return 0;
}
