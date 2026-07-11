#include "state_store.hpp"

namespace nova {

StateStore::StateStore(EventBus& event_bus) : event_bus_(event_bus) {}

AppState StateStore::snapshot() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_;
}

ClockState StateStore::clock() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_.clock;
}

MarketState StateStore::market() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_.market;
}

WeatherState StateStore::weather() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_.weather;
}

SystemState StateStore::system() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_.system;
}

UiState StateStore::ui() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_.ui;
}

void StateStore::set_clock(ClockState clock) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.clock = clock;
    }
    event_bus_.publish(Event{EventType::ClockChanged, 0});
}

void StateStore::set_market(MarketState market) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.market = market;
    }
    event_bus_.publish(Event{EventType::MarketChanged, 0});
}

void StateStore::set_weather(WeatherState weather) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.weather = weather;
    }
    event_bus_.publish(Event{EventType::WeatherChanged, 0});
}

void StateStore::set_system(SystemState system) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.system = system;
    }
    event_bus_.publish(Event{EventType::SystemChanged, 0});
}

void StateStore::set_ui(UiState ui) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.ui = ui;
    }
    event_bus_.publish(Event{EventType::ScreenChanged, static_cast<int32_t>(ui.active_screen)});
}

void StateStore::navigate_to(ScreenId screen_id) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.ui.active_screen = screen_id;
        if (screen_id != ScreenId::Boot) {
            state_.ui.boot_complete = true;
        }
    }
    event_bus_.publish(Event{EventType::ScreenChanged, static_cast<int32_t>(screen_id)});
}

void StateStore::set_action_queue_overflows(uint32_t overflow_count) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.system.action_queue_overflows = overflow_count;
    }
    event_bus_.publish(Event{EventType::SystemChanged, static_cast<int32_t>(overflow_count)});
}

}  // namespace nova
