// NovaPainel - core/event_bus.cpp
#include "event_bus.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "EventBus";
}

const char* to_string(EventType type) {
    switch (type) {
        case EventType::BootCompleted:        return "BootCompleted";
        case EventType::ScreenChanged:        return "ScreenChanged";
        case EventType::ClockUpdated:         return "ClockUpdated";
        case EventType::MarketUpdated:        return "MarketUpdated";
        case EventType::WeatherUpdated:       return "WeatherUpdated";
        case EventType::NotificationCreated:  return "NotificationCreated";
        case EventType::RequestPolicyChanged: return "RequestPolicyChanged";
        case EventType::ResourceWarning:      return "ResourceWarning";
        case EventType::UiInvalidated:        return "UiInvalidated";
        case EventType::SystemStatusChanged:  return "SystemStatusChanged";
        case EventType::OnboardingStepSubmitted: return "OnboardingStepSubmitted";
        case EventType::OnboardingStateChanged:  return "OnboardingStateChanged";
        case EventType::WifiScanRequested:       return "WifiScanRequested";
    }
    return "Unknown";
}

SubscriptionId EventBus::subscribe(EventHandler handler) {
    const SubscriptionId id = next_id_++;
    subscribers_.push_back(Subscription{id, std::move(handler)});
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    for (auto it = subscribers_.begin(); it != subscribers_.end(); ++it) {
        if (it->id == id) {
            subscribers_.erase(it);
            return;
        }
    }
}

void EventBus::publish(const Event& event) {
    ESP_LOGD(kTag, "publish %s (i32=%ld)", to_string(event.type),
             static_cast<long>(event.i32));
    // Copy is intentional: handlers may (un)subscribe during dispatch.
    auto snapshot = subscribers_;
    for (auto& sub : snapshot) {
        if (sub.handler) sub.handler(event);
    }
}

void EventBus::publish(EventType type, int32_t i32) {
    publish(Event{type, i32});
}

}  // namespace nova
