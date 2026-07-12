#include "setup_view_model.hpp"

#include "strings_ptbr.hpp"

namespace nova {

SetupViewModel make_setup_view_model(const AppState& state) {
    const char* detail = strings_ptbr::kSetupMessage;
    if (!state.setup.transport_ready) {
        detail = strings_ptbr::kSetupTransportOffline;
    } else if (state.setup.scan_in_progress ||
               state.setup.wifi_scan_status == WifiScanStatus::Scanning) {
        detail = strings_ptbr::kSetupScanning;
    } else if (state.setup.connect_in_progress ||
               state.setup.wifi_connect_status == WifiConnectStatus::Connecting) {
        detail = strings_ptbr::kSetupConnecting;
    } else if (state.setup.wifi_connect_status == WifiConnectStatus::Connected) {
        detail = strings_ptbr::kSetupConnected;
    } else if (state.setup.wifi_scan_status == WifiScanStatus::Failed) {
        detail = strings_ptbr::kSetupScanFailed;
    } else if (state.setup.wifi_connect_status == WifiConnectStatus::Failed) {
        detail = strings_ptbr::kSetupConnectFailed;
    } else if (state.setup.onboarding_required) {
        detail = strings_ptbr::kSetupTransportReady;
    }

    return SetupViewModel{
        strings_ptbr::kSetupTitle,
        state.setup.onboarding_required ? detail : "Setup concluido",
    };
}

}  // namespace nova
