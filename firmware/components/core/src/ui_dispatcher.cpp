// NovaPanel - core/ui_dispatcher.cpp
#include "ui_dispatcher.hpp"

#include "esp_log.h"
#include <algorithm>
#include <utility>

namespace nova {

namespace {
constexpr const char* kTag = "UiDispatcher";

bool is_ui_relevant(EventType type) {
    switch (type) {
        case EventType::BootStateChanged:
        case EventType::ClockUpdated:
        case EventType::MarketUpdated:
        case EventType::WeatherUpdated:
        case EventType::NotificationCreated:
        case EventType::SystemStatusChanged:
        case EventType::ScreenChanged:
        case EventType::UiInvalidated:
        case EventType::OnboardingStateChanged:
            return true;
        default:
            return false;
    }
}

// Data events can be coalesced: only the latest matters per type.
// Navigation (ScreenChanged) and NotificationCreated are kept individually
// because each occurrence may carry a distinct i32 payload.
bool is_coalescible(EventType type) {
    switch (type) {
        case EventType::ClockUpdated:
        case EventType::MarketUpdated:
        case EventType::WeatherUpdated:
        case EventType::SystemStatusChanged:
        case EventType::UiInvalidated:
        case EventType::BootStateChanged:
        case EventType::OnboardingStateChanged:
            return true;
        default:
            return false;
    }
}

// Removes duplicate coalescible events, keeping only the last occurrence of
// each type. Non-coalescible events (ScreenChanged, NotificationCreated) are
// preserved in their original order. This prevents N calls to np_update_home()
// when multiple providers publish in the same 200ms process_pending() window.
std::vector<UiEvent> coalesce(std::vector<UiEvent> events) {
    std::vector<UiEvent> result;
    result.reserve(events.size());

    for (const UiEvent& ev : events) {
        if (is_coalescible(ev.source)) {
            // Drop earlier entry of the same type; we will push the new one.
            result.erase(
                std::remove_if(result.begin(), result.end(),
                    [&](const UiEvent& r) { return r.source == ev.source; }),
                result.end());
        }
        result.push_back(ev);
    }
    return result;
}
}  // namespace

UiDispatcher::UiDispatcher(EventBus& bus) : bus_(bus) {
    sub_id_ = bus_.subscribe([this](const Event& e) {
        if (is_ui_relevant(e.type)) {
            post(UiEvent{e.type, e.i32});
        }
    });
}

void UiDispatcher::bind_render(RenderFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    render_ = std::move(fn);
}

void UiDispatcher::post(const UiEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(event);
}

size_t UiDispatcher::pending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void UiDispatcher::process_pending() {
    std::vector<UiEvent> pending_events;
    RenderFn render;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return;
        pending_events.swap(queue_);
        render = render_;
    }
    ESP_LOGD(kTag, "draining %u ui event(s)", static_cast<unsigned>(pending_events.size()));
    if (!render) {
        return;
    }

    const size_t original_count = pending_events.size();
    const std::vector<UiEvent> coalesced = coalesce(std::move(pending_events));
    if (coalesced.size() < original_count) {
        ESP_LOGD(kTag, "coalesced %u -> %u event(s)",
                 static_cast<unsigned>(original_count),
                 static_cast<unsigned>(coalesced.size()));
    }

    for (const UiEvent& event : coalesced) {
        render(event);
    }
}

}  // namespace nova
