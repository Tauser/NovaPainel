// NovaPanel - core/ui_dispatcher.cpp
#include "ui_dispatcher.hpp"

#include "esp_log.h"
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
    for (const UiEvent& event : pending_events) {
        render(event);
    }
}

}  // namespace nova
