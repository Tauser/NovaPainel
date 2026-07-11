#pragma once

#include "event_bus.hpp"

#include <cstdint>

namespace nova {

class UiDispatcher {
public:
    using RenderFn = void (*)();

    explicit UiDispatcher(EventBus& event_bus);

    uint32_t pending_mask() const { return pending_mask_; }
    void clear() { pending_mask_ = 0; }
    void process_pending(RenderFn render);

private:
    static void on_event(const Event& event, void* context);
    static uint32_t mask_for(EventType type);

    uint32_t pending_mask_{0};
};

}  // namespace nova
