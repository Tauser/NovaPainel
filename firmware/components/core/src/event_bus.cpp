#include "event_bus.hpp"

namespace nova {

const char* to_string(EventType type) {
    switch (type) {
        case EventType::ClockChanged:
            return "clock_changed";
        case EventType::MarketChanged:
            return "market_changed";
        case EventType::WeatherChanged:
            return "weather_changed";
        case EventType::SystemChanged:
            return "system_changed";
        case EventType::SetupChanged:
            return "setup_changed";
        case EventType::ScreenChanged:
            return "screen_changed";
        case EventType::ResourceWarning:
            return "resource_warning";
        case EventType::UiAction:
            return "ui_action";
    }
    return "unknown";
}

bool EventBus::subscribe(EventType type, Handler handler, void* context) {
    if (handler == nullptr || subscriber_count_ >= subscribers_.size()) {
        return false;
    }
    subscribers_[subscriber_count_++] = Subscriber{type, handler, context};
    return true;
}

void EventBus::publish(Event event) const {
    for (std::size_t i = 0; i < subscriber_count_; ++i) {
        const Subscriber& subscriber = subscribers_[i];
        if (subscriber.type == event.type && subscriber.handler != nullptr) {
            subscriber.handler(event, subscriber.context);
        }
    }
}

}  // namespace nova
