#include "screen_registry.hpp"

namespace nova {

bool ScreenRegistry::register_screen(ScreenSpec spec) {
    if (spec.title == nullptr || spec.build == nullptr || spec.update == nullptr ||
        size_ >= screens_.size() || find(spec.id) != nullptr) {
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

const ScreenSpec* ScreenRegistry::find(ScreenId id) const {
    for (std::size_t index = 0; index < size_; ++index) {
        if (screens_[index].id == id) {
            return &screens_[index];
        }
    }
    return nullptr;
}

}  // namespace nova
