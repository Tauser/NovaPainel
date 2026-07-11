#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nova {

enum class EventType : uint8_t {
    ClockChanged,
    MarketChanged,
    WeatherChanged,
    SystemChanged,
    ScreenChanged,
    ResourceWarning,
    UiAction,
};

constexpr uint32_t ui_event_bit(EventType type) {
    return 1u << static_cast<uint8_t>(type);
}

const char* to_string(EventType type);

struct Event {
    EventType type{EventType::SystemChanged};
    int32_t value{0};
};

class EventBus {
public:
    using Handler = void (*)(const Event& event, void* context);

    bool subscribe(EventType type, Handler handler, void* context);
    void publish(Event event) const;
    std::size_t subscriber_count() const { return subscriber_count_; }

private:
    struct Subscriber {
        EventType type{EventType::SystemChanged};
        Handler handler{nullptr};
        void* context{nullptr};
    };

    static constexpr std::size_t kMaxSubscribers = 16;
    std::array<Subscriber, kMaxSubscribers> subscribers_{};
    std::size_t subscriber_count_{0};
};

}  // namespace nova
