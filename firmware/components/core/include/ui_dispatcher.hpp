// NovaPanel - core/ui_dispatcher.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

#include "event_bus.hpp"

namespace nova {

struct UiEvent {
    EventType source;
    int32_t   i32{0};
};

using RenderFn = std::function<void(const UiEvent&)>;

class UiDispatcher {
public:
    explicit UiDispatcher(EventBus& bus);
    void bind_render(RenderFn fn);
    void post(const UiEvent& event);
    void process_pending();
    size_t pending() const;

private:
    EventBus&              bus_;
    SubscriptionId         sub_id_{0};
    mutable std::mutex     mutex_;
    std::vector<UiEvent>   queue_;
    RenderFn               render_{};
};

}  // namespace nova
