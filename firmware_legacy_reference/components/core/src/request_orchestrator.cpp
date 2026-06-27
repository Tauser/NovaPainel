// NovaPainel - core/request_orchestrator.cpp
#include "request_orchestrator.hpp"

#include <algorithm>

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "RequestOrchestrator";
constexpr uint32_t kMinuteMs = 60u * 1000u;
}

const char* to_string(DataDomain domain) {
    switch (domain) {
        case DataDomain::Weather:        return "Weather";
        case DataDomain::MarketSummary:  return "MarketSummary";
        case DataDomain::MarketRealtime: return "MarketRealtime";
        case DataDomain::MarketCandles:  return "MarketCandles";
        case DataDomain::Forex:          return "Forex";
        case DataDomain::Calendar:       return "Calendar";
        case DataDomain::Notifications:  return "Notifications";
        case DataDomain::Photos:         return "Photos";
        case DataDomain::System:         return "System";
    }
    return "Unknown";
}

const char* to_string(RequestPriority priority) {
    switch (priority) {
        case RequestPriority::Critical: return "Critical";
        case RequestPriority::High:     return "High";
        case RequestPriority::Normal:   return "Normal";
        case RequestPriority::Low:      return "Low";
        case RequestPriority::Paused:   return "Paused";
    }
    return "Unknown";
}

const char* to_string(CircuitState state) {
    switch (state) {
        case CircuitState::Closed:   return "closed";
        case CircuitState::Open:     return "open";
        case CircuitState::HalfOpen: return "half-open";
    }
    return "unknown";
}

RequestOrchestrator::RequestOrchestrator() {
    // ---- MVP policy table (see docs/PLANEJAMENTO.md sec. "Market Strategy") ----
    // MarketSummary: 60s interval, max 6 calls/min, Normal priority.
    // MarketRealtime / MarketCandles: disabled in the MVP, reserved for future.
    policies_[static_cast<int>(DataDomain::Weather)]        = {true,  RequestPriority::Normal, 10u * 60u * 1000u, 6};
    policies_[static_cast<int>(DataDomain::MarketSummary)]  = {true,  RequestPriority::Normal, 60u * 1000u,       6};
    policies_[static_cast<int>(DataDomain::MarketRealtime)] = {false, RequestPriority::Paused, 0,                0};
    policies_[static_cast<int>(DataDomain::MarketCandles)]  = {false, RequestPriority::Low,    60u * 1000u,      6};
    policies_[static_cast<int>(DataDomain::Forex)]          = {true,  RequestPriority::Normal, 120u * 1000u,     6};
    policies_[static_cast<int>(DataDomain::Calendar)]       = {true,  RequestPriority::Low,    5u * 60u * 1000u, 0};
    policies_[static_cast<int>(DataDomain::Notifications)]  = {true,  RequestPriority::Low,    0,                0};
    policies_[static_cast<int>(DataDomain::Photos)]         = {true,  RequestPriority::Low,    0,                0};
    policies_[static_cast<int>(DataDomain::System)]         = {true,  RequestPriority::Normal, 1000u,            0};
}

const RequestOrchestrator::Policy& RequestOrchestrator::policy(DataDomain domain) const {
    return policies_[static_cast<int>(domain)];
}
RequestOrchestrator::Policy& RequestOrchestrator::mutable_policy(DataDomain domain) {
    return policies_[static_cast<int>(domain)];
}
RequestOrchestrator::Runtime& RequestOrchestrator::runtime(DataDomain domain) {
    return runtime_[static_cast<int>(domain)];
}
const RequestOrchestrator::Runtime& RequestOrchestrator::runtime(DataDomain domain) const {
    return runtime_[static_cast<int>(domain)];
}

void RequestOrchestrator::set_focused_mode(bool enabled) {
    focused_mode_ = enabled;
    // In focused mode the heavy market domains become available; when not
    // focused they stay paused. This is a coarse MVP stand-in for the full
    // per-screen policy matrix described in the planning doc.
    mutable_policy(DataDomain::MarketCandles).enabled = enabled;
    ESP_LOGI(kTag, "focused_mode=%s", enabled ? "on" : "off");
}

uint32_t RequestOrchestrator::interval_ms_for(DataDomain domain) const {
    return policy(domain).interval_ms;
}

RequestPriority RequestOrchestrator::priority_for(DataDomain domain) const {
    return policy(domain).priority;
}

CircuitState RequestOrchestrator::circuit_state(DataDomain domain) const {
    return runtime(domain).circuit;
}

bool RequestOrchestrator::is_degraded(DataDomain domain) const {
    const CircuitState s = runtime(domain).circuit;
    return s == CircuitState::Open || s == CircuitState::HalfOpen;
}

bool RequestOrchestrator::within_rate_limit(DataDomain domain) const {
    const Policy& p = policy(domain);
    if (p.max_per_minute == 0) return true;  // unlimited
    const Runtime& rt = runtime(domain);
    // If the 60s window has rolled over, the counter is effectively reset.
    if (now_ms_ - rt.window_start_ms >= kMinuteMs) return true;
    return rt.calls_in_window < p.max_per_minute;
}

bool RequestOrchestrator::can_request(DataDomain domain) const {
    const Policy& p = policy(domain);
    if (!p.enabled) return false;
    const Runtime& rt = runtime(domain);

    switch (rt.circuit) {
        case CircuitState::Open:
            return false;  // blocked; update_clock() handles Open->HalfOpen
        case CircuitState::HalfOpen:
            // Waive the interval gate for the probe - we want to try immediately
            // after the backoff timeout, not wait another interval_ms on top.
            return within_rate_limit(domain);
        case CircuitState::Closed:
            break;
    }

    if (rt.last_request_ms != 0 && (now_ms_ - rt.last_request_ms) < p.interval_ms) {
        return false;  // too soon
    }
    return within_rate_limit(domain);
}

void RequestOrchestrator::note_request(DataDomain domain, uint32_t now_ms, bool success) {
    Runtime& rt = runtime(domain);

    if (success) {
        rt.last_request_ms = now_ms;
        if (rt.circuit != CircuitState::Closed) {
            ESP_LOGI(kTag, "%s circuit -> closed (recovered after %lu failures)",
                     to_string(domain), static_cast<unsigned long>(rt.consec_failures));
            rt.circuit          = CircuitState::Closed;
            rt.consec_failures  = 0;
            rt.backoff_ms       = 0;
        } else {
            rt.consec_failures = 0;
        }
    } else {
        ++rt.consec_failures;
        if (rt.circuit == CircuitState::Closed) {
            if (rt.consec_failures >= kFailureThreshold) {
                open_circuit(domain, now_ms);
            }
        } else if (rt.circuit == CircuitState::HalfOpen) {
            // Probe failed - reopen with doubled backoff.
            open_circuit(domain, now_ms);
        }
        // While already Open: failures don't reset the backoff timer (the
        // circuit stays open until open_until_ms; only probes count).
    }

    // Rate-limit window accounting (applies regardless of circuit state so
    // the budget is still consumed even when open - matches the existing
    // "failure counts against rate limit" contract above).
    if (now_ms - rt.window_start_ms >= kMinuteMs) {
        rt.window_start_ms = now_ms;
        rt.calls_in_window = 0;
    }
    ++rt.calls_in_window;

    ESP_LOGD(kTag, "note %s success=%d circuit=%s consec_fail=%lu calls_in_win=%lu",
             to_string(domain), success, to_string(rt.circuit),
             static_cast<unsigned long>(rt.consec_failures),
             static_cast<unsigned long>(rt.calls_in_window));
}

void RequestOrchestrator::update_clock(uint32_t now_ms) {
    now_ms_ = now_ms;
    for (int i = 0; i < kDomainCount; ++i) {
        Runtime& rt = runtime_[i];
        if (rt.circuit == CircuitState::Open && now_ms >= rt.open_until_ms) {
            rt.circuit = CircuitState::HalfOpen;
            ESP_LOGI(kTag, "%s circuit -> half-open (probing)",
                     to_string(static_cast<DataDomain>(i)));
        }
    }
}

void RequestOrchestrator::open_circuit(DataDomain domain, uint32_t now_ms) {
    Runtime& rt = runtime(domain);
    rt.backoff_ms = jittered_backoff(rt.backoff_ms);
    rt.circuit       = CircuitState::Open;
    rt.open_until_ms = now_ms + rt.backoff_ms;
    ESP_LOGW(kTag, "%s circuit -> open (consec_fail=%lu retry_in=%lus)",
             to_string(domain),
             static_cast<unsigned long>(rt.consec_failures),
             static_cast<unsigned long>(rt.backoff_ms / 1000));
}

uint32_t RequestOrchestrator::jittered_backoff(uint32_t current_ms) {
    // Double from initial; cap at max.
    const uint32_t base = (current_ms == 0)
                          ? kBackoffInitialMs
                          : std::min(current_ms * 2u, kBackoffMaxMs);
    // ±20% jitter via a fast XOR-shift PRNG (no stdlib random needed).
    prng_ ^= prng_ << 13;
    prng_ ^= prng_ >> 17;
    prng_ ^= prng_ << 5;
    const uint32_t jitter_range = base / 5u;  // 20% of base
    const uint32_t jitter       = (jitter_range > 0) ? (prng_ % jitter_range) : 0;
    return (prng_ & 1u) ? base + jitter : base - jitter;
}

}  // namespace nova
