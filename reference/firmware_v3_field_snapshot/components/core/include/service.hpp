// NovaPainel - core/service.hpp
// Base interface every internal service implements. Services are lightweight,
// cooperative units driven by ServiceManager::tick_all(). They never block the
// UI and never touch LVGL objects directly.
#pragma once

#include <cstdint>

namespace nova {

class Service {
public:
    virtual ~Service() = default;

    // Stable, human-readable name used in logs.
    virtual const char* name() const = 0;

    // One-time setup. Return false to signal the service failed to initialize.
    virtual bool init() { return true; }

    // Called after all services are initialized.
    virtual void start() {}

    // Called periodically by ServiceManager. `now_ms` is a monotonic clock in ms.
    virtual void tick(uint32_t now_ms) { (void)now_ms; }

    // Graceful shutdown (e.g. maintenance mode).
    virtual void stop() {}
};

}  // namespace nova
