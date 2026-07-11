// NovaPainel - core/service_manager.hpp
// Owns the lifecycle of all registered services. Does not own the Service
// pointers' memory (they are typically static/stack in app_main).
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "service.hpp"

namespace nova {

class ServiceManager {
public:
    void add(Service* service);

    bool init_all();   // returns false if any service failed to init
    void start_all();
    void tick_all(uint32_t now_ms);
    void stop_all();

    size_t count() const { return services_.size(); }

private:
    std::vector<Service*> services_;
};

}  // namespace nova
