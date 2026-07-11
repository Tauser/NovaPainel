// NovaPainel - services/system_service.hpp
// Boot diagnostics (Fase 7, ADR-0028): reset reason + reboot counter, the
// same esp_reset_reason()/NVS pattern already validated on real hardware by
// the Fase 0 harness (firmware/experiments/gate15_coexistence/main/gate15_main.c,
// load_and_bump_reboot_count()). Mirrors ClockService/SetupService - all the
// work happens once, in init().
#pragma once

#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class SystemService : public Service {
public:
    explicit SystemService(StateStore& store) : store_(store) {}

    const char* name() const override { return "SystemService"; }
    bool init() override;

private:
    StateStore& store_;
};

}  // namespace nova
