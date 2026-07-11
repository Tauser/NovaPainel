// NovaPanel - core/event_bus.hpp
#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace nova {

enum class EventType {
    BootCompleted,
    BootStateChanged,
    BootSkipRequested,
    ScreenChanged,
    ClockUpdated,
    MarketUpdated,
    ForexUpdated,
    WeatherUpdated,
    NotificationCreated,
    RequestPolicyChanged,
    ResourceWarning,
    UiInvalidated,
    SystemStatusChanged,
    OnboardingStepSubmitted,
    OnboardingStateChanged,
    WifiScanRequested,
    OhlcUpdated,
    PreferencesChanged,
};

const char* to_string(EventType type);

struct Event {
    EventType type;
    int32_t   i32{0};
};

using EventHandler = std::function<void(const Event&)>;
using SubscriptionId = uint32_t;

class EventBus {
public:
    SubscriptionId subscribe(EventHandler handler);
    void unsubscribe(SubscriptionId id);
    void publish(const Event& event);
    void publish(EventType type, int32_t i32 = 0);

private:
    struct Subscription {
        SubscriptionId id;
        EventHandler    handler;
    };

    mutable std::mutex       mutex_;
    std::vector<Subscription> subscribers_;
    SubscriptionId next_id_{1};
};

}  // namespace nova
