#include "state_store.hpp"

namespace nova {

StateStore::StateStore(EventBus& event_bus) : event_bus_(event_bus) {}

void StateStore::set_clock(ClockState clock) {
    state_.clock = clock;
    event_bus_.publish(Event{EventType::ClockChanged, 0});
}

void StateStore::set_market(MarketState market) {
    state_.market = market;
    event_bus_.publish(Event{EventType::MarketChanged, 0});
}

void StateStore::set_weather(WeatherState weather) {
    state_.weather = weather;
    event_bus_.publish(Event{EventType::WeatherChanged, 0});
}

void StateStore::set_system(SystemState system) {
    state_.system = system;
    event_bus_.publish(Event{EventType::SystemChanged, 0});
}

void StateStore::set_action_queue_overflows(uint32_t overflow_count) {
    state_.system.action_queue_overflows = overflow_count;
    event_bus_.publish(Event{EventType::SystemChanged, static_cast<int32_t>(overflow_count)});
}

}  // namespace nova
