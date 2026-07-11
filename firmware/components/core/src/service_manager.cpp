#include "service_manager.hpp"

namespace nova {

bool ServiceManager::add(TickFn tick, void* context) {
    if (tick == nullptr || service_count_ >= services_.size()) {
        return false;
    }
    services_[service_count_++] = Service{tick, context};
    return true;
}

void ServiceManager::tick() {
    for (std::size_t i = 0; i < service_count_; ++i) {
        services_[i].tick(services_[i].context);
    }
}

}  // namespace nova
