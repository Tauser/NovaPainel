#include "request_orchestrator.hpp"

namespace nova {

RequestOrchestrator::RequestOrchestrator() {
    configure_default_policies();
}

Status RequestOrchestrator::configure(RequestPolicy policy) {
    if (policy.rate_limit_per_minute == 0 || policy.circuit_failures_to_open == 0) {
        return Status::error(StatusCode::InvalidArgument, "invalid request policy");
    }

    DomainState* existing = find(policy.domain);
    if (existing != nullptr) {
        existing->policy = policy;
        existing->configured = true;
        return Status::success();
    }

    for (DomainState& domain : domains_) {
        if (!domain.configured) {
            domain.policy = policy;
            domain.configured = true;
            return Status::success();
        }
    }
    return Status::error(StatusCode::Overflow, "request domain capacity exhausted");
}

Status RequestOrchestrator::configure_default_policies() {
    const RequestPolicy defaults[] = {
        RequestPolicy{RequestDomain::Weather, RequestPriority::Normal, 30 * 60 * 1000, 6, 3, 30000, 30 * 60 * 1000, true},
        RequestPolicy{RequestDomain::MarketSpot, RequestPriority::Normal, 3 * 60 * 1000, 6, 3, 30000, 30 * 60 * 1000, true},
        RequestPolicy{RequestDomain::Forex, RequestPriority::Normal, 60 * 60 * 1000, 6, 3, 30000, 30 * 60 * 1000, true},
        RequestPolicy{RequestDomain::Ohlc, RequestPriority::Low, 5 * 60 * 1000, 6, 3, 30000, 30 * 60 * 1000, true},
    };

    for (const RequestPolicy& policy : defaults) {
        const Status status = configure(policy);
        if (!status.ok()) {
            return status;
        }
    }
    return Status::success();
}

bool RequestOrchestrator::can_start(RequestDomain domain, uint64_t now_ms) const {
    const DomainState* state = find(domain);
    if (state == nullptr || !state->policy.enabled) {
        return false;
    }
    if (in_flight_) {
        return false;
    }
    if (last_global_start_ms_ != 0 && now_ms < last_global_start_ms_ + kGlobalGapMs) {
        return false;
    }
    if (!circuit_allows_start(*state, now_ms) || rate_limited(*state, now_ms)) {
        return false;
    }
    if (state->last_start_ms != 0 && now_ms < state->last_start_ms + state->policy.min_interval_ms) {
        return false;
    }
    return true;
}

Status RequestOrchestrator::mark_started(RequestDomain domain, uint64_t now_ms) {
    DomainState* state = find(domain);
    if (state == nullptr) {
        return Status::error(StatusCode::InvalidArgument, "unknown request domain");
    }
    if (!can_start(domain, now_ms)) {
        return Status::error(StatusCode::RateLimited, "request domain interval not elapsed");
    }
    state->last_start_ms = now_ms;
    if (state->rate_window_start_ms == 0 || now_ms >= state->rate_window_start_ms + 60000) {
        state->rate_window_start_ms = now_ms;
        state->starts_in_window = 0;
    }
    ++state->starts_in_window;
    if (state->circuit_state == CircuitState::Open) {
        state->circuit_state = CircuitState::HalfOpen;
    }
    in_flight_ = true;
    in_flight_domain_ = domain;
    last_global_start_ms_ = now_ms;
    return Status::success();
}

void RequestOrchestrator::mark_finished(RequestDomain domain, bool ok, uint64_t now_ms) {
    DomainState* state = find(domain);
    if (state == nullptr) {
        return;
    }
    if (in_flight_ && in_flight_domain_ == domain) {
        in_flight_ = false;
    }
    if (ok) {
        state->consecutive_failures = 0;
        state->circuit_state = CircuitState::Closed;
        state->opened_at_ms = 0;
        state->current_backoff_ms = 0;
        return;
    }

    ++state->consecutive_failures;
    if (state->consecutive_failures >= state->policy.circuit_failures_to_open) {
        state->circuit_state = CircuitState::Open;
        state->opened_at_ms = now_ms;
        state->current_backoff_ms = next_backoff_ms(*state);
    }
}

void RequestOrchestrator::tick(uint64_t now_ms) {
    for (DomainState& state : domains_) {
        if (state.configured &&
            state.circuit_state == CircuitState::Open &&
            circuit_allows_start(state, now_ms)) {
            state.circuit_state = CircuitState::HalfOpen;
        }
    }
}

CircuitState RequestOrchestrator::circuit_state(RequestDomain domain) const {
    const DomainState* state = find(domain);
    if (state == nullptr) {
        return CircuitState::Open;
    }
    return state->circuit_state;
}

uint32_t RequestOrchestrator::consecutive_failures(RequestDomain domain) const {
    const DomainState* state = find(domain);
    if (state == nullptr) {
        return 0;
    }
    return state->consecutive_failures;
}

RequestPriority RequestOrchestrator::priority_for(RequestDomain domain) const {
    const DomainState* state = find(domain);
    if (state == nullptr) {
        return RequestPriority::Normal;
    }
    return state->policy.priority;
}

RequestOrchestrator::DomainState* RequestOrchestrator::find(RequestDomain domain) {
    for (DomainState& state : domains_) {
        if (state.configured && state.policy.domain == domain) {
            return &state;
        }
    }
    return nullptr;
}

const RequestOrchestrator::DomainState* RequestOrchestrator::find(RequestDomain domain) const {
    for (const DomainState& state : domains_) {
        if (state.configured && state.policy.domain == domain) {
            return &state;
        }
    }
    return nullptr;
}

uint32_t RequestOrchestrator::next_backoff_ms(const DomainState& state) {
    uint32_t backoff = state.current_backoff_ms == 0
        ? state.policy.circuit_base_backoff_ms
        : state.current_backoff_ms * 2;
    if (backoff > state.policy.circuit_max_backoff_ms) {
        backoff = state.policy.circuit_max_backoff_ms;
    }
    return backoff;
}

bool RequestOrchestrator::rate_limited(const DomainState& state, uint64_t now_ms) {
    if (state.rate_window_start_ms == 0 || now_ms >= state.rate_window_start_ms + 60000) {
        return false;
    }
    return state.starts_in_window >= state.policy.rate_limit_per_minute;
}

bool RequestOrchestrator::circuit_allows_start(const DomainState& state, uint64_t now_ms) {
    if (state.circuit_state == CircuitState::Closed || state.circuit_state == CircuitState::HalfOpen) {
        return true;
    }
    return now_ms >= state.opened_at_ms + state.current_backoff_ms;
}

}  // namespace nova
