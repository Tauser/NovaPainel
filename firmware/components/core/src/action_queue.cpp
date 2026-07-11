#include "action_queue.hpp"

namespace nova {

bool ActionQueue::push(Action action) {
    if (size_ == items_.size()) {
        ++overflow_count_;
        return false;
    }
    items_[tail_] = action;
    tail_ = (tail_ + 1) % items_.size();
    ++size_;
    return true;
}

void ActionQueue::drain(Handler handler) {
    while (size_ > 0) {
        const Action action = items_[head_];
        head_ = (head_ + 1) % items_.size();
        --size_;
        if (handler != nullptr) {
            handler(action);
        }
    }
}

}  // namespace nova
