#pragma once

#include <string>

#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class ClockService final : public Service {
public:
    explicit ClockService(StateStore& store);

    const char* name() const override;
    bool init() override;
    void tick(uint32_t now_ms) override;

private:
    void advance_mock();
    void apply_timezone_if_changed();

    StateStore& store_;
    ClockState  clock_{};
    uint32_t    last_tick_ms_{0};
    std::string applied_timezone_;
    bool        last_synced_{false};
};

}  // namespace nova
