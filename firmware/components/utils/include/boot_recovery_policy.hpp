#pragma once

#include "app_state.hpp"

#include <cstdint>

namespace nova {

struct BootRecoveryConfig {
    uint32_t base_delay_ms{1000};
    uint32_t max_backoff_factor{8};
    uint32_t max_attempts_per_boot{3};
};

struct DisplayFailureDecision {
    UiState next_ui_state{};
    uint32_t delay_ms{0};
    bool restart_required{false};
};

DisplayFailureDecision on_display_failure(
    const UiState& current_ui_state,
    uint32_t attempts_this_boot,
    BootRecoveryConfig config = {});

}  // namespace nova
