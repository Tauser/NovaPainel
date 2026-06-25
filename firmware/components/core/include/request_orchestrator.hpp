// NovaPainel - core/request_orchestrator.hpp
// Central gate for ALL outbound requests (see ADR-0004). Services must ask
// can_request(domain) before hitting a provider. This is where per-screen
// priority, update intervals and API rate limits are enforced.
//
// The MVP implementation is deliberately simple but the intent (and the public
// surface) is the real, long-term contract.
#pragma once

#include <cstdint>

namespace nova {

enum class RequestPriority {
    Critical,
    High,
    Normal,
    Low,
    Paused,
};

enum class DataDomain {
    Weather,
    MarketSummary,
    MarketRealtime,
    MarketCandles,
    Forex,
    Calendar,
    Notifications,
    Photos,
    System,
};

const char* to_string(DataDomain domain);
const char* to_string(RequestPriority priority);

class RequestOrchestrator {
public:
    RequestOrchestrator();

    // "Focused mode" raises the active screen's domain priority and pauses the
    // heavy/background ones. Wired to ScreenChanged in a later phase.
    void set_focused_mode(bool enabled);
    bool focused_mode() const { return focused_mode_; }

    // True if the domain is enabled AND its interval has elapsed AND it is
    // within its rate-limit budget.
    bool can_request(DataDomain domain) const;

    RequestPriority priority_for(DataDomain domain) const;
    uint32_t        interval_ms_for(DataDomain domain) const;
    bool            within_rate_limit(DataDomain domain) const;

    // Services call this right after a request attempt so the orchestrator
    // can track interval + rate-limit windows. `now_ms` is monotonic ms.
    // `success=false` (e.g. no network yet right after boot) still counts
    // against the per-minute rate-limit budget, but does NOT push out the
    // interval gate - the next attempt is allowed as soon as the next tick,
    // instead of waiting the full interval (60s/10min) after a failure that
    // never actually used the budget productively.
    void note_request(DataDomain domain, uint32_t now_ms, bool success = true);

    // Lets the time-based checks know "now". Called once per loop tick.
    void update_clock(uint32_t now_ms) { now_ms_ = now_ms; }

private:
    struct Policy {
        bool            enabled;
        RequestPriority priority;
        uint32_t        interval_ms;
        uint32_t        max_per_minute;   // 0 = unlimited
    };

    struct Runtime {
        uint32_t last_request_ms{0};
        uint32_t window_start_ms{0};
        uint32_t calls_in_window{0};
    };

    const Policy& policy(DataDomain domain) const;
    Policy&       mutable_policy(DataDomain domain);
    Runtime&      runtime(DataDomain domain);
    const Runtime& runtime(DataDomain domain) const;

    static constexpr int kDomainCount = 9;
    Policy   policies_[kDomainCount];
    Runtime  runtime_[kDomainCount];
    bool     focused_mode_{false};
    uint32_t now_ms_{0};
};

}  // namespace nova
