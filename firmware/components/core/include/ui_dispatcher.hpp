// NovaPainel - core/ui_dispatcher.hpp
// UI marshaling boundary (ADR-0007).
//
// THE RULE: only the future lvgl_task may touch LVGL objects. Service tasks must
// never render directly. Instead they publish events on the EventBus; the
// UiDispatcher subscribes to UI-relevant events, enqueues them, and later
// drains the queue from the lvgl_task via process_pending().
//
//   Service Task -> EventBus -> UiDispatcher (queue) -> lvgl_task -> Screen.render()
//
// In this skeleton there is no LVGL yet: process_pending() simply invokes a
// registered render callback and logs. The contract is what matters now.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "event_bus.hpp"

namespace nova {

// A UI-side event. Kept distinct from the bus Event so the UI layer can evolve
// its own vocabulary (invalidation regions, screen ids, etc.) later.
struct UiEvent {
    EventType source;
    int32_t   i32{0};
};

// Render callback the app binds (e.g. HomeScreen::render bound to StateStore).
using RenderFn = std::function<void(const UiEvent&)>;

class UiDispatcher {
public:
    explicit UiDispatcher(EventBus& bus);

    // Bind the function executed on the (future) lvgl_task when events drain.
    void bind_render(RenderFn fn) { render_ = std::move(fn); }

    // Called by services/EventBus (any task). Thread-safety to be added with
    // the real lvgl_task (FreeRTOS queue). For now it is a plain vector.
    void post(const UiEvent& event);

    // MUST run on the lvgl_task once it exists. Drains queued UI events.
    void process_pending();

    size_t pending() const { return queue_.size(); }

private:
    EventBus&            bus_;
    SubscriptionId       sub_id_{0};
    std::vector<UiEvent> queue_;
    RenderFn             render_{};
};

}  // namespace nova
