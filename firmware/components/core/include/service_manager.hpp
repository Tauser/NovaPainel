// NovaPanel - core/service_manager.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "service.hpp"

namespace nova {

class ServiceManager {
public:
    void add(Service* service);
    bool init_all();
    void start_all();
    void tick_all(uint32_t now_ms);
    void stop_all();
    size_t count() const { return services_.size(); }

private:
    std::vector<Service*> services_;
};

}  // namespace nova
