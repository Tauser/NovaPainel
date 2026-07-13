#include "ui_dispatcher.hpp"

namespace nova {

UiDispatcher::UiDispatcher(EventBus& event_bus) {
    event_bus.subscribe(EventType::ClockChanged, &UiDispatcher::on_event, this);
    event_bus.subscribe(EventType::MarketChanged, &UiDispatcher::on_event, this);
    event_bus.subscribe(EventType::WeatherChanged, &UiDispatcher::on_event, this);
    event_bus.subscribe(EventType::SystemChanged, &UiDispatcher::on_event, this);
    event_bus.subscribe(EventType::SetupChanged, &UiDispatcher::on_event, this);
    event_bus.subscribe(EventType::ScreenChanged, &UiDispatcher::on_event, this);
    event_bus.subscribe(EventType::ResourceWarning, &UiDispatcher::on_event, this);
}

void UiDispatcher::process_pending(RenderFn render) {
    const uint32_t pending = take_pending_mask();
    if (pending == 0) {
        return;
    }
    if (render != nullptr) {
        render();
    }
}

void UiDispatcher::on_event(const Event& event, void* context) {
    auto* dispatcher = static_cast<UiDispatcher*>(context);
    dispatcher->pending_mask_.fetch_or(mask_for(event.type));
}

uint32_t UiDispatcher::mask_for(EventType type) {
    return ui_event_bit(type);
}

}  // namespace nova
