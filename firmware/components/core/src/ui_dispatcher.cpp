// NovaPainel - core/ui_dispatcher.cpp
#include "ui_dispatcher.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "UiDispatcher";

// Which bus events are relevant to the UI and should be marshaled.
bool is_ui_relevant(EventType type) {
    switch (type) {
        case EventType::ClockUpdated:
        case EventType::MarketUpdated:
        case EventType::NotificationCreated:
        case EventType::SystemStatusChanged:
        case EventType::ScreenChanged:
        case EventType::UiInvalidated:
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

void UiDispatcher::post(const UiEvent& event) {
    // FUTURE: push to a FreeRTOS queue consumed by lvgl_task.
    queue_.push_back(event);
}

void UiDispatcher::process_pending() {
    if (queue_.empty()) return;
    // Coalesce: a full screen re-render reads the latest StateStore, so we draw
    // once per drain (driven by the most recent event) instead of once per
    // event. This is the marshaling step that will run on lvgl_task later.
    ESP_LOGD(kTag, "draining %u ui event(s) -> 1 render [would run on lvgl_task]",
             static_cast<unsigned>(queue_.size()));
    const UiEvent last = queue_.back();
    queue_.clear();
    if (render_) render_(last);
}

}  // namespace nova
