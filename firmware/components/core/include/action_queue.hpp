#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nova {

enum class ActionType : uint8_t {
    None,
    Navigate,
    Refresh,
    SetBrightness,
};

struct Action {
    ActionType type{ActionType::None};
    int32_t value{0};
};

class ActionQueue {
public:
    using Handler = void (*)(const Action& action);

    bool push(Action action);
    void drain(Handler handler);
    std::size_t size() const { return size_; }
    uint32_t overflow_count() const { return overflow_count_; }

private:
    static constexpr std::size_t kCapacity = 16;
    std::array<Action, kCapacity> items_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
    uint32_t overflow_count_{0};
};

}  // namespace nova
