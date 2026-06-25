// NovaPainel - core/request_orchestrator.cpp
#include "request_orchestrator.hpp"

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
    if (rt.last_request_ms != 0 && (now_ms_ - rt.last_request_ms) < p.interval_ms) {
        return false;  // too soon
    }
    return within_rate_limit(domain);
}

void RequestOrchestrator::note_request(DataDomain domain, uint32_t now_ms, bool success) {
    Runtime& rt = runtime(domain);
    if (success) rt.last_request_ms = now_ms;
    if (now_ms - rt.window_start_ms >= kMinuteMs) {
        rt.window_start_ms = now_ms;
        rt.calls_in_window = 0;
    }
    rt.calls_in_window++;
    ESP_LOGD(kTag, "note %s success=%d (calls_in_window=%lu)", to_string(domain), success,
             static_cast<unsigned long>(rt.calls_in_window));
}

}  // namespace nova
