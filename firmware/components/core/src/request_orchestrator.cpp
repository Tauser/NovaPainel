#include "request_orchestrator.hpp"

namespace nova {

Status RequestOrchestrator::configure(RequestPolicy policy) {
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

bool RequestOrchestrator::can_start(RequestDomain domain, uint64_t now_ms) const {
    const DomainState* state = find(domain);
    if (state == nullptr || !state->policy.enabled) {
        return false;
    }
    if (state->last_start_ms == 0) {
        return true;
    }
    return now_ms >= state->last_start_ms + state->policy.min_interval_ms;
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
    return Status::success();
}

void RequestOrchestrator::mark_finished(RequestDomain domain, bool ok, uint64_t /*now_ms*/) {
    DomainState* state = find(domain);
    if (state == nullptr) {
        return;
    }
    state->consecutive_failures = ok ? 0 : state->consecutive_failures + 1;
}

void RequestOrchestrator::tick(uint64_t /*now_ms*/) {}

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

}  // namespace nova
