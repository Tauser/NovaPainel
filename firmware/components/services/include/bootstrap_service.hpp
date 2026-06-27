#pragma once

#include "event_bus.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class BootstrapService final : public Service {
public:
    BootstrapService(StateStore& store, EventBus& bus);

    const char* name() const override;
    bool init() override;
    void start() override;
    void tick(uint32_t now_ms) override;
    void stop() override;

private:
    static constexpr uint32_t kBootVisibleMs = 2000;

    BootState compute_boot_state(uint32_t elapsed_ms) const;
    void finish_boot();

    StateStore& store_;
    EventBus& bus_;
    SubscriptionId sub_id_{0};
    bool started_{false};
    bool switched_to_home_{false};
    bool skip_requested_{false};
    uint32_t boot_started_ms_{0};
};

}  // namespace nova
