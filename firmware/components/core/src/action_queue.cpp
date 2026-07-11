#include "action_queue.hpp"

#include "esp_log.h"

namespace nova {
namespace {
constexpr const char* kTag = "ActionQueue";
}

bool ActionQueue::push(Action action) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (size_ == items_.size()) {
        ++overflow_count_;
        ESP_LOGW(kTag, "overflow: dropped action type=%d total=%lu",
                 static_cast<int>(action.type),
                 static_cast<unsigned long>(overflow_count_));
        return false;
    }
    items_[tail_] = action;
    tail_ = (tail_ + 1) % items_.size();
    ++size_;
    return true;
}

void ActionQueue::drain(Handler handler) {
    while (true) {
        Action action{};
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (size_ == 0) {
                return;
            }
            action = items_[head_];
            head_ = (head_ + 1) % items_.size();
            --size_;
        }
        if (handler != nullptr) {
            handler(action);
        }
    }
}

std::size_t ActionQueue::size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return size_;
}

uint32_t ActionQueue::overflow_count() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return overflow_count_;
}

}  // namespace nova
