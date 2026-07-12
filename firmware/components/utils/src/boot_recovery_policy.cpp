#include "boot_recovery_policy.hpp"

namespace nova {

DisplayFailureDecision on_display_failure(
    const UiState& current_ui_state,
    uint32_t attempts_this_boot,
    BootRecoveryConfig config) {
    DisplayFailureDecision decision{};
    decision.next_ui_state = current_ui_state;
    decision.next_ui_state.display_breadcrumb = true;
    ++decision.next_ui_state.display_retry_count;

    uint32_t factor = 1;
    uint32_t remaining_steps = decision.next_ui_state.display_retry_count;
    while (factor < config.max_backoff_factor && remaining_steps > 0) {
        factor <<= 1;
        --remaining_steps;
    }
    if (factor > config.max_backoff_factor) {
        factor = config.max_backoff_factor;
    }

    decision.delay_ms = config.base_delay_ms * factor;
    decision.restart_required = attempts_this_boot >= config.max_attempts_per_boot;
    return decision;
}

}  // namespace nova
