// NovaPainel - core/event_bus.hpp
// Synchronous, in-process publish/subscribe bus.
//
// IMPORTANT ARCHITECTURE RULE:
//   The EventBus delivers events to subscribers on the SAME task that called
//   publish(). Service tasks therefore must NOT touch UI objects from inside a
//   subscriber. UI-relevant events are forwarded to the UiDispatcher, which
//   marshals them onto the (future) lvgl_task. See docs/ARCHITECTURE.md and
//   ADR-0007.
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace nova {

enum class EventType {
    BootCompleted,
    ScreenChanged,
    ClockUpdated,
    MarketUpdated,
    WeatherUpdated,
    NotificationCreated,
    RequestPolicyChanged,
    ResourceWarning,
    UiInvalidated,
    SystemStatusChanged,
    // ADR-0017: the wizard publishes an "intention" (never persists/calls
    // hardware itself) by writing to StateStore::submit_onboarding(), which
    // publishes this. SetupService is the only subscriber that acts on it.
    OnboardingStepSubmitted,
    // SetupService publishes this after it persisted something / advanced
    // the wizard step / got a Wi-Fi connect result - the UI re-renders from
    // AppState::onboarding + AppState::preferences, same pattern as
    // ClockUpdated/MarketUpdated.
    OnboardingStateChanged,
    // Wizard's Wifi step asks for a network scan - same "intention" pattern
    // as OnboardingStepSubmitted, just not tied to a wizard step submission.
    WifiScanRequested,
};

const char* to_string(EventType type);

// Minimal event payload. `i32` is a generic small payload (e.g. a ScreenId or
// notification id); richer data lives in the StateStore, which the UI reads.
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
        EventHandler   handler;
    };
    std::vector<Subscription> subscribers_;
    SubscriptionId next_id_{1};
};

}  // namespace nova
