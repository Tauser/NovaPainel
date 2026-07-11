#pragma once

#include "event_bus.hpp"

#include <atomic>
#include <cstdint>

namespace nova {

class UiDispatcher {
public:
    using RenderFn = void (*)();

    explicit UiDispatcher(EventBus& event_bus);

    uint32_t pending_mask() const { return pending_mask_.load(); }
    void clear() { pending_mask_.store(0); }
    void process_pending(RenderFn render);

private:
    static void on_event(const Event& event, void* context);
    static uint32_t mask_for(EventType type);

    // on_event() may run on the publisher task while process_pending() runs on
    // main_loop; atomic coalescing keeps the cross-task handoff lock-free.
    std::atomic<uint32_t> pending_mask_{0};
};

}  // namespace nova
