#include "action_queue.hpp"
#include "app_state.hpp"
#include "awesomeapi_provider.hpp"
#include "boot_breadcrumb_store.hpp"
#include "boot_recovery_policy.hpp"
#include "cache_store.hpp"
#include "clock_service.hpp"
#include "coingecko_provider.hpp"
#include "event_bus.hpp"
#include "http_client.hpp"
#include "json_value.hpp"
#include "market_service.hpp"
#include "mock_board.hpp"
#include "network_worker.hpp"
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
#include <cstring>
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

struct FetchCounter {
    int calls{0};
};

class FakeMarketProvider final : public nova::IMarketProvider {
public:
    nova::Result<nova::MarketState> fetch_market() override {
        ++calls;
        return result;
    }
    int calls{0};
    nova::Result<nova::MarketState> result{
        nova::Result<nova::MarketState>::failure(
            nova::Status::error(nova::StatusCode::Unavailable, "not configured"))};
};

class FakeForexProvider final : public nova::IForexProvider {
public:
    nova::Result<nova::MarketState> fetch_forex() override {
        ++calls;
        return result;
    }
    int calls{0};
    nova::Result<nova::MarketState> result{
        nova::Result<nova::MarketState>::failure(
            nova::Status::error(nova::StatusCode::Unavailable, "not configured"))};
};

bool succeed_fetch(void* context, uint64_t /*now_ms*/) {
    static_cast<FetchCounter*>(context)->calls++;
    return true;
}

bool fail_fetch(void* context, uint64_t /*now_ms*/) {
    static_cast<FetchCounter*>(context)->calls++;
    return false;
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
    dispatcher.take_pending_mask();
    nova::SetupState setup_for_dispatch{};
    setup_for_dispatch.valid = true;
    setup_for_dispatch.wifi_scan_status = nova::WifiScanStatus::Done;
    setup_for_dispatch.wifi_networks.push_back(nova::WifiNetwork{"NovaNet", -42, true});
    state_store.set_setup(setup_for_dispatch);
    assert((dispatcher.pending_mask() & nova::ui_event_bit(nova::EventType::SetupChanged)) != 0);

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

    // Real HTTP only exists under ESP_PLATFORM; on host this is always the
    // stub failure (no network stack to fake safely in a unit test).
    nova::HttpClient http_client;
    const auto unavailable_on_host = http_client.get("https://example.invalid/data");
    assert(!unavailable_on_host.ok());
    assert(unavailable_on_host.status().code() == nova::StatusCode::Unavailable);

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
    nova::OnboardingSubmission wifi_submission{};
    wifi_submission.step = nova::OnboardingStep::Wifi;
    wifi_submission.wifi_ssid = "NovaNet";
    wifi_submission.wifi_password = "segredo";
    setup_service.submit_onboarding(wifi_submission);
    assert(state_store.setup().wifi_configured);
    assert(state_store.setup().onboarding_step == nova::OnboardingStep::TimezoneAndFormat);

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

    nova::RequestOrchestrator net_orchestrator;
    nova::NetworkWorker network_worker(state_store, net_orchestrator);
    assert(network_worker.fetcher_count() == 0);
    assert(!network_worker.network_available());
    assert(network_worker.select_fetcher_index(0) == -1);

    FetchCounter weather_counter{};
    FetchCounter market_counter{};
    network_worker.register_fetcher(nova::RequestDomain::Weather, &succeed_fetch, &weather_counter);
    network_worker.register_fetcher(nova::RequestDomain::MarketSpot, &fail_fetch, &market_counter);
    assert(network_worker.fetcher_count() == 2);
    // select_fetcher_index() only decides among "due" domains; it does not
    // gate on network_available() itself -- that's a separate check run()
    // makes before ever calling select_fetcher_index (single responsibility,
    // and it keeps the priority logic host-testable without a live link).
    assert(!network_worker.network_available());

    nova::SetupState connected_setup{};
    connected_setup.wifi_connect_status = nova::WifiConnectStatus::Connected;
    state_store.set_setup(connected_setup);
    assert(network_worker.network_available());

    // Both Weather and MarketSpot come from RequestOrchestrator's default
    // policies (Normal priority, never started yet) and are due at t=0.
    // Equal priority: registration order wins (Weather registered first),
    // matching the ADR-0004 boot-scheduling rule.
    assert(net_orchestrator.priority_for(nova::RequestDomain::Weather) ==
           net_orchestrator.priority_for(nova::RequestDomain::MarketSpot));
    assert(network_worker.select_fetcher_index(0) == 0);

    FetchCounter ohlc_counter{};
    network_worker.register_fetcher(nova::RequestDomain::Ohlc, &succeed_fetch, &ohlc_counter);
    assert(net_orchestrator.configure(nova::RequestPolicy{
        nova::RequestDomain::Ohlc, nova::RequestPriority::Critical, 0, 6, 3, 30000, 30000, true}).ok());
    // Critical outranks the tied Normal fetchers even though it was
    // registered last.
    assert(network_worker.select_fetcher_index(0) == 2);

    assert(net_orchestrator.mark_started(nova::RequestDomain::Ohlc, 0).ok());
    assert(ohlc_counter.calls == 0);  // NetworkWorker::run() drives the call, not selection alone
    net_orchestrator.mark_finished(nova::RequestDomain::Ohlc, true, 0);
    // In-flight gate: nothing else can start until mark_finished runs (already
    // exercised above); after finishing, Ohlc's min_interval_ms=0 keeps it
    // eligible again immediately, so it still wins on priority.
    assert(network_worker.select_fetcher_index(0) == 2);

    struct TestPayload {
        double value_a;
        double value_b;
        uint32_t updated_ms;
    };

    nova::CacheStore cache_store;
    const TestPayload not_mounted{1.0, 2.0, 100};
    assert(!cache_store.save("market", &not_mounted, sizeof(not_mounted), 0).ok());
    assert(cache_store.mount().ok());
    assert(cache_store.ready());

    const auto missing = cache_store.load("market", sizeof(TestPayload));
    assert(!missing.ok());
    assert(missing.status().code() == nova::StatusCode::Unavailable);
    assert(!cache_store.would_throttle("market", 0));

    const TestPayload market_payload{42.5, -1.5, 1000};
    assert(cache_store.save("market", &market_payload, sizeof(market_payload), 1000).ok());
    assert(cache_store.would_throttle("market", 1000 + 60000));  // < 30 min later
    assert(!cache_store.would_throttle("market", 1000 + 30 * 60 * 1000));  // exactly at window

    const auto throttled = cache_store.save("market", &market_payload, sizeof(market_payload), 1000 + 60000);
    assert(!throttled.ok());
    assert(throttled.code() == nova::StatusCode::RateLimited);

    const auto loaded = cache_store.load("market", sizeof(TestPayload));
    assert(loaded.ok());
    assert(loaded.value().size() == sizeof(TestPayload));
    TestPayload roundtrip{};
    std::memcpy(&roundtrip, loaded.value().data(), sizeof(TestPayload));
    assert(roundtrip.value_a == 42.5);
    assert(roundtrip.value_b == -1.5);
    assert(roundtrip.updated_ms == 1000);

    // Weather is a distinct domain: its own throttle window, doesn't share
    // storage with "market".
    const TestPayload weather_payload{18.0, 0.0, 2000};
    assert(cache_store.save("weather", &weather_payload, sizeof(weather_payload), 2000).ok());
    const auto weather_loaded = cache_store.load("weather", sizeof(TestPayload));
    assert(weather_loaded.ok());

    // Schema mismatch (wrong expected_size) is rejected, not interpreted.
    struct WrongSizePayload {
        double only_one_field;
    };
    const auto mismatched = cache_store.load("market", sizeof(WrongSizePayload));
    assert(!mismatched.ok());
    assert(mismatched.status().code() == nova::StatusCode::InvalidArgument);

    // After the 30 min throttle window, a write goes through again.
    assert(cache_store.save("market", &market_payload, sizeof(market_payload),
                             1000 + 30 * 60 * 1000).ok());

    // JsonValue: shape used by real providers (array of objects, nested
    // object, string-typed numeric field), plus malformed/truncated fixtures.
    const auto coingecko_shaped = nova::parse_json(
        R"([{"current_price": 68000.5, "id": "bitcoin", "nested": {"a": 1}}])");
    assert(coingecko_shaped.ok());
    assert(coingecko_shaped.value().is_array());
    assert(coingecko_shaped.value().size() == 1);
    const nova::JsonValue* first = coingecko_shaped.value().at(0);
    assert(first != nullptr && first->is_object());
    const nova::JsonValue* price = first->find("current_price");
    assert(price != nullptr && price->is_number());
    assert(price->as_number() == 68000.5);
    assert(first->find("missing") == nullptr);
    const nova::JsonValue* nested = first->find("nested");
    assert(nested != nullptr && nested->is_object());
    assert(nested->find("a")->as_number() == 1.0);

    const auto awesomeapi_shaped = nova::parse_json(
        R"({"USDBRL": {"bid": "5.4321", "high": "5.50", "pctChange": "-0.42"}})");
    assert(awesomeapi_shaped.ok());
    const nova::JsonValue* pair = awesomeapi_shaped.value().find("USDBRL");
    assert(pair != nullptr && pair->is_object());
    const nova::JsonValue* bid = pair->find("bid");
    assert(bid != nullptr && bid->is_string());
    assert(bid->as_string() == "5.4321");

    assert(!nova::parse_json("{\"unterminated\": ").ok());
    assert(!nova::parse_json("{not json at all}").ok());
    assert(!nova::parse_json("[1, 2,]").ok());  // trailing comma: not valid JSON
    assert(!nova::parse_json("").ok());
    assert(!nova::parse_json("42 43").ok());  // trailing garbage after value
    assert(nova::parse_json("[]").ok());
    assert(nova::parse_json("{}").ok());
    assert(nova::parse_json("null").ok());
    assert(nova::parse_json("true").ok());
    assert(nova::parse_json("\"escaped \\\"quote\\\" and \\u00e9\"").ok());
    assert(nova::parse_json("\"escaped \\\"quote\\\" and \\u00e9\"").value().as_string() ==
           "escaped \"quote\" and \xc3\xa9");

    // Deeply nested array beyond the depth guard must fail, not overflow the
    // stack (embedded safety: recursive descent parser).
    std::string deeply_nested(40, '[');
    deeply_nested.append(40, ']');
    assert(!nova::parse_json(deeply_nested).ok());

    // CoinGeckoProvider: fixtures (ADR-0007) -- real-shaped payload and
    // malformed variants, exercised without any network access.
    const auto coingecko_real = nova::CoinGeckoProvider::parse_market_payload(R"([
        {"id":"bitcoin","symbol":"btc","name":"Bitcoin","current_price":67890.12,
         "market_cap":1234567890,"price_change_percentage_24h":1.23}
    ])");
    assert(coingecko_real.ok());
    assert(coingecko_real.value().btc_usd == 67890.12);
    assert(coingecko_real.value().valid);

    assert(!nova::CoinGeckoProvider::parse_market_payload("[]").ok());
    assert(!nova::CoinGeckoProvider::parse_market_payload("{}").ok());
    assert(!nova::CoinGeckoProvider::parse_market_payload(R"([{"id":"bitcoin"}])").ok());
    assert(!nova::CoinGeckoProvider::parse_market_payload(R"([{"current_price":"not a number"}])").ok());
    assert(!nova::CoinGeckoProvider::parse_market_payload(R"([{"current_price":-5}])").ok());
    assert(!nova::CoinGeckoProvider::parse_market_payload("not json").ok());
    assert(!nova::CoinGeckoProvider::parse_market_payload("").ok());

    // AwesomeApiProvider: fixtures -- real payload has string-typed numbers.
    const auto awesomeapi_real = nova::AwesomeApiProvider::parse_forex_payload(R"({
        "USDBRL":{"code":"USD","codein":"BRL","high":"5.45","low":"5.40",
                   "varBid":"0.0123","pctChange":"0.23","bid":"5.4321","ask":"5.4325",
                   "timestamp":"1700000000"}
    })");
    assert(awesomeapi_real.ok());
    assert(awesomeapi_real.value().usd_brl == 5.4321);
    assert(awesomeapi_real.value().valid);

    assert(!nova::AwesomeApiProvider::parse_forex_payload("{}").ok());
    assert(!nova::AwesomeApiProvider::parse_forex_payload(R"({"USDBRL":{}})").ok());
    assert(!nova::AwesomeApiProvider::parse_forex_payload(R"({"USDBRL":{"bid":"N/A"}})").ok());
    assert(!nova::AwesomeApiProvider::parse_forex_payload(R"({"USDBRL":{"bid":"0"}})").ok());
    assert(!nova::AwesomeApiProvider::parse_forex_payload("[]").ok());
    assert(!nova::AwesomeApiProvider::parse_forex_payload("").ok());

    // MarketService: merges partial fetches without clobbering the other
    // currency, persists to CacheStore, and restores stale-flagged cache on
    // boot -- the Fase 4 exit criterion ("boot offline mostra cache com
    // stale sinalizado").
    nova::CacheStore market_cache;
    assert(market_cache.mount().ok());
    FakeMarketProvider fake_crypto;
    FakeForexProvider fake_forex;
    nova::StateStore market_state_store(event_bus);
    nova::MarketService market_service(market_state_store, market_cache, fake_crypto, fake_forex);

    market_service.load_from_cache(1000);  // nothing cached yet: no-op
    assert(!market_state_store.market().valid);

    fake_crypto.result = nova::Result<nova::MarketState>::success(nova::MarketState{});
    fake_crypto.result.value().btc_usd = 50000.0;
    assert(nova::MarketService::refresh_crypto(&market_service, 2000));
    assert(market_state_store.market().btc_usd == 50000.0);
    assert(market_state_store.market().usd_brl == 0.0);  // untouched: forex never ran
    assert(market_state_store.market().valid);
    assert(!market_state_store.market().stale);
    assert(market_state_store.market().source == nova::DataSource::Live);

    fake_forex.result = nova::Result<nova::MarketState>::success(nova::MarketState{});
    fake_forex.result.value().usd_brl = 5.5;
    assert(nova::MarketService::refresh_forex(&market_service, 3000));
    assert(market_state_store.market().usd_brl == 5.5);
    assert(market_state_store.market().btc_usd == 50000.0);  // crypto preserved

    fake_crypto.result = nova::Result<nova::MarketState>::failure(
        nova::Status::error(nova::StatusCode::Unavailable, "network down"));
    assert(!nova::MarketService::refresh_crypto(&market_service, 4000));
    assert(market_state_store.market().btc_usd == 50000.0);  // last good value kept on failure

    nova::StateStore fresh_state_store(event_bus);
    nova::MarketService cache_reader_service(fresh_state_store, market_cache, fake_crypto, fake_forex);
    cache_reader_service.load_from_cache(5000);
    assert(fresh_state_store.market().valid);
    assert(fresh_state_store.market().stale);
    assert(fresh_state_store.market().source == nova::DataSource::Cache);
    assert(fresh_state_store.market().btc_usd == 50000.0);
    assert(fresh_state_store.market().usd_brl == 5.5);

    return 0;
}
