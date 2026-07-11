#include "screen_registry.hpp"

namespace nova {

bool ScreenRegistry::register_screen(ScreenSpec spec) {
    if (spec.id == nullptr || size_ >= screens_.size()) {
        return false;
    }
    screens_[size_++] = spec;
    return true;
}

const ScreenSpec* ScreenRegistry::at(std::size_t index) const {
    if (index >= size_) {
        return nullptr;
    }
    return &screens_[index];
}

}  // namespace nova
