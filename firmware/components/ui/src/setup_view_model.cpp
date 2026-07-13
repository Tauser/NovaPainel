#include "setup_view_model.hpp"

#include "strings_ptbr.hpp"
#include "timezone_catalog.hpp"

#include <cstdio>

namespace nova {
namespace {

const char* scan_status_label(WifiScanStatus status) {
    switch (status) {
        case WifiScanStatus::Idle:
            return "scan parado";
        case WifiScanStatus::Scanning:
            return "buscando redes";
        case WifiScanStatus::Done:
            return "scan concluido";
        case WifiScanStatus::Failed:
            return "scan falhou";
    }
    return "scan desconhecido";
}

const char* connect_status_label(WifiConnectStatus status) {
    switch (status) {
        case WifiConnectStatus::Idle:
            return "conexao parada";
        case WifiConnectStatus::Connecting:
            return "conectando";
        case WifiConnectStatus::Connected:
            return "conectado";
        case WifiConnectStatus::Failed:
            return "conexao falhou";
    }
    return "conexao desconhecida";
}

}  // namespace

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

    char wifi_summary[96];
    std::snprintf(wifi_summary, sizeof(wifi_summary), "%u rede(s) encontrada(s) - %s",
                  static_cast<unsigned>(state.setup.wifi_networks.size()),
                  scan_status_label(state.setup.wifi_scan_status));

    char wifi_runtime[128];
    std::snprintf(wifi_runtime, sizeof(wifi_runtime),
                  "Transporte: %s - Wi-Fi: %s - tentativas: %u",
                  state.setup.transport_ready ? "ativo" : "indisponivel",
                  connect_status_label(state.setup.wifi_connect_status),
                  static_cast<unsigned>(state.setup.reconnect_attempts));

    const TimezoneOption& timezone =
        timezone_option_at(find_timezone_option_index(state.preferences.timezone));
    char timezone_summary[128];
    std::snprintf(timezone_summary, sizeof(timezone_summary), "%s - %s",
                  timezone.name, timezone.label);

    return SetupViewModel{
        strings_ptbr::kSetupTitle,
        state.setup.onboarding_required ? detail : "Setup concluido",
        wifi_summary,
        wifi_runtime,
        timezone_summary,
        state.preferences.use_24h ? "24 horas - Ex: 14:30" : "12 horas - Ex: 2:30 PM",
    };
}

}  // namespace nova
