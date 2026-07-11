#pragma once

#include "screen_spec.hpp"

#include <array>
#include <cstddef>

namespace nova {

class ScreenRegistry {
public:
    bool register_screen(ScreenSpec spec);
    std::size_t size() const { return size_; }
    const ScreenSpec* at(std::size_t index) const;

private:
    static constexpr std::size_t kMaxScreens = 12;
    std::array<ScreenSpec, kMaxScreens> screens_{};
    std::size_t size_{0};
};

}  // namespace nova
