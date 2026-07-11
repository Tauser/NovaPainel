#include "action_queue.hpp"
#include "app_state.hpp"
#include "event_bus.hpp"
#include "mock_board.hpp"
#include "request_orchestrator.hpp"
#include "screen_registry.hpp"
#include "state_store.hpp"
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

    nova::UiDispatcher dispatcher(event_bus);
    state_store.set_action_queue_overflows(3);
    assert(dispatcher.pending_mask() != 0);
    dispatcher.process_pending(&count_render);
    assert(renders == 1);
    assert(dispatcher.pending_mask() == 0);

    nova::ActionQueue queue;
    for (int i = 0; i < 16; ++i) {
        assert(queue.push(nova::Action{nova::ActionType::Refresh, i}));
    }
    assert(!queue.push(nova::Action{nova::ActionType::Refresh, 17}));
    assert(queue.overflow_count() == 1);
    queue.drain(&count_action);
    assert(drained_actions == 16);
    assert(queue.size() == 0);

    nova::RequestOrchestrator orchestrator;
    assert(orchestrator.configure(nova::RequestPolicy{
        nova::RequestDomain::Weather, nova::RequestPriority::Normal, 1000, 6, true}).ok());
    assert(orchestrator.can_start(nova::RequestDomain::Weather, 10));
    assert(orchestrator.mark_started(nova::RequestDomain::Weather, 10).ok());
    assert(!orchestrator.can_start(nova::RequestDomain::Weather, 500));
    assert(orchestrator.can_start(nova::RequestDomain::Weather, 1010));

    nova::MockBoard board;
    const nova::BoardStatus status = board.bring_up();
    assert(status.display_ready);
    assert(board.lock_shared_i2c(10));
    board.unlock_shared_i2c();

    nova::ScreenRegistry registry;
    assert(registry.register_screen(nova::ScreenSpec{"boot", 1u, nullptr, nullptr}));
    assert(registry.size() == 1);
    assert(registry.at(0) != nullptr);
    assert(registry.at(1) == nullptr);

    return 0;
}
