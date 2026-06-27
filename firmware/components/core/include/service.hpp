// NovaPanel - core/service.hpp
#pragma once

#include <cstdint>

namespace nova {

class Service {
public:
    virtual ~Service() = default;
    virtual const char* name() const = 0;
    virtual bool init() { return true; }
    virtual void start() {}
    virtual void tick(uint32_t now_ms) { (void)now_ms; }
    virtual void stop() {}
};

}  // namespace nova
