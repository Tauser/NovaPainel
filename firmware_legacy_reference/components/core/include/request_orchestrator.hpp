// NovaPainel - core/request_orchestrator.hpp
// Central gate for ALL outbound requests (see ADR-0004). Services must ask
// can_request(domain) before hitting a provider. This is where per-screen
// priority, update intervals, API rate limits and circuit-breaker state are
// enforced (Fase 8, ADR-0012).
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

// Per-domain circuit breaker state (Fase 8, ADR-0012).
// Closed  - normal operation.
// Open    - provider failing; requests blocked until open_until_ms elapses.
// HalfOpen- one probe request allowed; success -> Closed, failure -> Open
//           with doubled backoff.
enum class CircuitState { Closed, Open, HalfOpen };

const char* to_string(DataDomain domain);
const char* to_string(RequestPriority priority);
const char* to_string(CircuitState state);

class RequestOrchestrator {
public:
    RequestOrchestrator();

    // "Focused mode" raises the active screen's domain priority and pauses the
    // heavy/background ones. Wired to ScreenChanged in a later phase.
    void set_focused_mode(bool enabled);
    bool focused_mode() const { return focused_mode_; }

    // True if the domain is enabled AND its interval has elapsed AND it is
    // within its rate-limit budget AND the circuit breaker is not Open.
    // In HalfOpen state returns true once (the probe); subsequent calls return
    // true again only after note_request() closes or re-opens the circuit.
    bool can_request(DataDomain domain) const;

    RequestPriority priority_for(DataDomain domain) const;
    uint32_t        interval_ms_for(DataDomain domain) const;
    bool            within_rate_limit(DataDomain domain) const;

    // Circuit breaker state for the domain. Services may check is_degraded()
    // to decide whether to serve stale cache rather than waiting for a live fetch.
    CircuitState circuit_state(DataDomain domain) const;
    bool         is_degraded(DataDomain domain) const;  // Open || HalfOpen

    // Services call this right after a request attempt so the orchestrator
    // can track interval + rate-limit windows and circuit breaker transitions.
    // `now_ms` is monotonic ms.
    // `success=false` still counts against the rate-limit budget but does NOT
    // push out the interval gate. After kFailureThreshold consecutive failures
    // the circuit opens; a successful probe in HalfOpen closes it again.
    void note_request(DataDomain domain, uint32_t now_ms, bool success = true);

    // Lets the time-based checks know "now". Called once per loop tick.
    // Also drives the Open->HalfOpen timeout transition.
    void update_clock(uint32_t now_ms);

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

        // Circuit breaker (Fase 8, ADR-0012)
        CircuitState circuit{CircuitState::Closed};
        uint32_t     consec_failures{0};   // consecutive failures since last success
        uint32_t     open_until_ms{0};     // when Open transitions to HalfOpen
        uint32_t     backoff_ms{0};        // current backoff; doubles on each open
    };

    const Policy&  policy(DataDomain domain) const;
    Policy&        mutable_policy(DataDomain domain);
    Runtime&       runtime(DataDomain domain);
    const Runtime& runtime(DataDomain domain) const;

    void     open_circuit(DataDomain domain, uint32_t now_ms);
    uint32_t jittered_backoff(uint32_t current_ms);

    static constexpr int      kDomainCount      = 9;
    static constexpr uint32_t kFailureThreshold  = 3;
    static constexpr uint32_t kBackoffInitialMs  = 10u * 1000u;       //  10 s
    static constexpr uint32_t kBackoffMaxMs      = 5u * 60u * 1000u;  //   5 min

    Policy   policies_[kDomainCount];
    Runtime  runtime_[kDomainCount];
    bool     focused_mode_{false};
    uint32_t now_ms_{0};
    uint32_t prng_{0x9e3779b9u};  // XOR-shift seed for jitter
};

}  // namespace nova
