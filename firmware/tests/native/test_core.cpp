#include "action_queue.hpp"
#include "app_state.hpp"
#include "event_bus.hpp"
#include "http_client.hpp"
#include "mock_board.hpp"
#include "request_orchestrator.hpp"
#include "screen_registry.hpp"
#include "state_store.hpp"
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

    nova::ScreenRegistry registry;
    const auto build_stub = +[](lv_obj_t*) -> lv_obj_t* { return nullptr; };
    const auto update_stub = +[](const nova::AppState&) {};
    assert(registry.register_screen(nova::ScreenSpec{
        nova::ScreenId::Boot, "Boot", 1u, build_stub, update_stub, nullptr, nullptr}));
    assert(registry.register_screen(nova::ScreenSpec{
        nova::ScreenId::Home, "Home", 2u, build_stub, update_stub, nullptr, nullptr}));
    assert(registry.size() == 2);
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

    return 0;
}
