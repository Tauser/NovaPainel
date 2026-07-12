#include "app_state.hpp"

namespace nova {

const char* to_string(ScreenId screen_id) {
    switch (screen_id) {
        case ScreenId::Boot:
            return "boot";
        case ScreenId::Home:
            return "home";
        case ScreenId::Placeholder:
            return "placeholder";
        case ScreenId::Setup:
            return "setup";
    }
    return "unknown";
}

const char* to_string(DataSource source) {
    switch (source) {
        case DataSource::None:
            return "none";
        case DataSource::Live:
            return "live";
        case DataSource::Cache:
            return "cache";
        case DataSource::Mock:
            return "mock";
    }
    return "unknown";
}

const char* to_string(WifiConnectStatus status) {
    switch (status) {
        case WifiConnectStatus::Idle:
            return "idle";
        case WifiConnectStatus::Connecting:
            return "connecting";
        case WifiConnectStatus::Connected:
            return "connected";
        case WifiConnectStatus::Failed:
            return "failed";
    }
    return "unknown";
}

const char* to_string(WifiScanStatus status) {
    switch (status) {
        case WifiScanStatus::Idle:
            return "idle";
        case WifiScanStatus::Scanning:
            return "scanning";
        case WifiScanStatus::Done:
            return "done";
        case WifiScanStatus::Failed:
            return "failed";
    }
    return "unknown";
}

const char* to_string(OnboardingStep step) {
    switch (step) {
        case OnboardingStep::Wifi:
            return "wifi";
        case OnboardingStep::TimezoneAndFormat:
            return "timezone_and_format";
        case OnboardingStep::Confirmation:
            return "confirmation";
        case OnboardingStep::Done:
            return "done";
    }
    return "unknown";
}

}  // namespace nova
