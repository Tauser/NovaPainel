// NovaPainel - services/setup_service.cpp
// Real NVS persistence + Wi-Fi connect for the onboarding wizard (ADR-0017).
// Hardware-only: pulls in nvs_flash.h/esp_wifi.h/esp_event.h, none of which
// have a host shim (see tools/scripts/host_check.sh SKIP_FILES). NVS flash
// and the Wi-Fi STA driver are already initialized/started by
// WaveshareBoard::bring_up() (ESP-Hosted transport, ADR-0016) before any
// service runs - this only opens the NVS namespace and configures/connects
// once the wizard actually reaches the Wifi step.
#include "setup_service.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace nova {

namespace {
constexpr const char* kTag = "SetupService";
constexpr const char* kNvsNamespace = "novapanel";

constexpr const char* kKeyOnboardingDone = "onb_done";
constexpr const char* kKeyDisplayName    = "disp_name";
constexpr const char* kKeyTimezone       = "timezone";
constexpr const char* kKeyTimeFormat24h  = "time_24h";
constexpr const char* kKeyTheme          = "theme";
constexpr const char* kKeyWifiSsid       = "wifi_ssid";
constexpr const char* kKeyWifiPassword   = "wifi_pass";

bool read_string(nvs_handle_t handle, const char* key, std::string& out) {
    size_t len = 0;
    if (nvs_get_str(handle, key, nullptr, &len) != ESP_OK || len == 0) return false;
    out.resize(len);
    if (nvs_get_str(handle, key, out.data(), &len) != ESP_OK) return false;
    while (!out.empty() && out.back() == '\0') out.pop_back();  // nvs_get_str counts the terminator
    return true;
}

// Trampoline for esp_event_handler_instance_register() - kept as a free
// function (not a class member) so setup_service.hpp doesn't need to know
// about esp_event_base_t.
void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* /*data*/) {
    auto* self = static_cast<SetupService*>(arg);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        self->on_wifi_disconnected();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        self->on_wifi_connected();
    }
}
}  // namespace

bool SetupService::init() {
    // Idempotent - WaveshareBoard::bring_up() already called this for the
    // ESP-Hosted transport; calling it again here is a documented no-op.
    nvs_flash_init();

    nvs_handle_t handle{};
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGE(kTag, "nvs_open failed");
        return false;
    }

    uint8_t onboarding_done = 0;
    nvs_get_u8(handle, kKeyOnboardingDone, &onboarding_done);

    read_string(handle, kKeyDisplayName, prefs_.display_name);
    read_string(handle, kKeyTimezone, prefs_.timezone);

    uint8_t fmt24h = 1;
    nvs_get_u8(handle, kKeyTimeFormat24h, &fmt24h);
    prefs_.time_format_24h = (fmt24h != 0);

    uint8_t theme = static_cast<uint8_t>(ThemeMode::Auto);
    nvs_get_u8(handle, kKeyTheme, &theme);
    prefs_.theme = static_cast<ThemeMode>(theme);

    // Read while the wizard isn't needed anymore - saved credentials are
    // reused below to reconnect automatically on every boot (the wizard's
    // esp_wifi_connect() call only happens once, during onboarding itself).
    std::string saved_ssid, saved_password;
    const bool have_saved_wifi = onboarding_done &&
        read_string(handle, kKeyWifiSsid, saved_ssid) && !saved_ssid.empty();
    if (have_saved_wifi) read_string(handle, kKeyWifiPassword, saved_password);

    nvs_close(handle);

    store_.set_preferences(prefs_);
    store_.set_onboarding_needed(onboarding_done == 0);

    sub_id_ = bus_.subscribe([this](const Event& e) {
        if (e.type == EventType::OnboardingStepSubmitted) {
            handle_submission(store_.state().onboarding.pending_submission);
        } else if (e.type == EventType::WifiScanRequested) {
            handle_wifi_scan_request();
        }
    });

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, this, nullptr);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, this, nullptr);

    // Auto-reconnect on every boot after onboarding is done - the wizard's
    // esp_wifi_connect() (in handle_submission) only ever runs once, during
    // the Wifi step itself. Without this, every subsequent boot would sit on
    // the ESP-Hosted/SDIO transport with no AP association (found while
    // wiring CoinGeckoProvider: no real network -> no HTTPS -> no NTP).
    if (have_saved_wifi) {
        wifi_config_t wifi_config{};
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
                    saved_ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
                    saved_password.c_str(), sizeof(wifi_config.sta.password) - 1);
        store_.set_wifi_status(WifiConnectStatus::Connecting);
        if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK ||
            esp_wifi_connect() != ESP_OK) {
            ESP_LOGW(kTag, "auto-reconnect esp_wifi_set_config/connect failed");
            store_.set_wifi_status(WifiConnectStatus::Failed);
        }
    }

    // ADR-0021: HTTPS (CoinGeckoProvider) needs a correct system clock to
    // validate TLS certs - there's no battery-backed RTC (Fase 0), so NTP is
    // the only source. start=false here: actually syncing only makes sense
    // once Wi-Fi is up, so on_wifi_connected() calls esp_netif_sntp_start().
    esp_sntp_config_t sntp_config = {};
    sntp_config.start = false;
    sntp_config.wait_for_sync = false;  // never block this task waiting for sync
    sntp_config.num_of_servers = 1;
    sntp_config.servers[0] = "pool.ntp.org";
    esp_netif_sntp_init(&sntp_config);

    ESP_LOGI(kTag, "onboarding_needed=%d display_name='%s'",
             onboarding_done == 0, prefs_.display_name.c_str());
    return true;
}

void SetupService::handle_submission(const OnboardingSubmission& submission) {
    nvs_handle_t handle{};
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGE(kTag, "nvs_open (write) failed");
        return;
    }

    switch (submission.step) {
        case OnboardingStep::DisplayName: {
            prefs_.display_name = submission.display_name;
            nvs_set_str(handle, kKeyDisplayName, prefs_.display_name.c_str());
            nvs_commit(handle);
            store_.set_preferences(prefs_);
            store_.set_onboarding_step(OnboardingStep::Wifi);
            break;
        }
        case OnboardingStep::Wifi: {
            if (submission.wifi_ssid.empty()) {
                // Empty SSID = disconnect request from SettingsScreen modal
                esp_wifi_disconnect();
                nvs_erase_key(handle, kKeyWifiSsid);
                nvs_erase_key(handle, kKeyWifiPassword);
                nvs_commit(handle);
                nvs_close(handle);
                store_.set_wifi_status(WifiConnectStatus::Idle);
                return;
            }
            nvs_set_str(handle, kKeyWifiSsid, submission.wifi_ssid.c_str());
            nvs_set_str(handle, kKeyWifiPassword, submission.wifi_password.c_str());
            nvs_commit(handle);

            wifi_config_t wifi_config{};
            std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
                        submission.wifi_ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
            std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
                        submission.wifi_password.c_str(), sizeof(wifi_config.sta.password) - 1);

            store_.set_wifi_status(WifiConnectStatus::Connecting);
            if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK ||
                esp_wifi_connect() != ESP_OK) {
                ESP_LOGW(kTag, "esp_wifi_set_config/connect failed");
                store_.set_wifi_status(WifiConnectStatus::Failed);
            }
            // Step only advances to Timezone once on_wifi_connected() fires
            // (or the user retries) - never optimistically, unlike the other
            // steps, since this one depends on a real network result.
            break;
        }
        case OnboardingStep::TimezoneAndFormat: {
            prefs_.timezone = submission.timezone;
            prefs_.time_format_24h = submission.time_format_24h;
            nvs_set_str(handle, kKeyTimezone, prefs_.timezone.c_str());
            nvs_set_u8(handle, kKeyTimeFormat24h, prefs_.time_format_24h ? 1 : 0);
            nvs_commit(handle);
            store_.set_preferences(prefs_);
            store_.set_onboarding_step(OnboardingStep::Confirmation);
            break;
        }
        case OnboardingStep::Confirmation: {
            // Nothing new to persist - everything was already written by
            // the previous steps. This just means "user pressed Iniciar
            // NovaPanel on the summary screen".
            nvs_set_u8(handle, kKeyOnboardingDone, 1);
            nvs_commit(handle);
            store_.set_onboarding_step(OnboardingStep::Done);
            store_.set_onboarding_needed(false);
            break;
        }
        case OnboardingStep::Done:
            break;
    }

    nvs_close(handle);
}

void SetupService::handle_wifi_scan_request() {
    constexpr uint16_t kMaxResults = 24;

    // Blocking scan: this runs off the lvgl_task (deferred via app_main's
    // action queue, same fix as the Wifi-connect freeze) so it's fine to
    // block here for the ~hundreds of ms a scan takes.
    if (esp_wifi_scan_start(nullptr, true) != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_scan_start failed");
        store_.set_wifi_networks({}, WifiScanStatus::Failed);
        return;
    }

    uint16_t found = 0;
    esp_wifi_scan_get_ap_num(&found);
    found = std::min(found, kMaxResults);

    std::vector<wifi_ap_record_t> records(found);
    if (esp_wifi_scan_get_ap_records(&found, records.data()) != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_scan_get_ap_records failed");
        store_.set_wifi_networks({}, WifiScanStatus::Failed);
        return;
    }

    std::vector<WifiNetwork> networks;
    for (uint16_t i = 0; i < found; ++i) {
        const auto& rec = records[i];
        const char* ssid = reinterpret_cast<const char*>(rec.ssid);
        if (ssid[0] == '\0') continue;  // hidden network - nothing to show/tap

        auto existing = std::find_if(networks.begin(), networks.end(),
            [&](const WifiNetwork& n) { return n.ssid == ssid; });
        if (existing != networks.end()) {
            if (rec.rssi > existing->rssi) existing->rssi = rec.rssi;  // same SSID, multiple APs/channels - keep strongest
            continue;
        }
        networks.push_back(WifiNetwork{ssid, rec.rssi, rec.authmode != WIFI_AUTH_OPEN});
    }
    std::sort(networks.begin(), networks.end(),
             [](const WifiNetwork& a, const WifiNetwork& b) { return a.rssi > b.rssi; });

    ESP_LOGI(kTag, "wifi scan: %u network(s) after de-dup", static_cast<unsigned>(networks.size()));
    store_.set_wifi_networks(networks, WifiScanStatus::Done);
}

void SetupService::on_wifi_connected() {
    ESP_LOGI(kTag, "Wi-Fi connected (got IP)");
    wifi_retry_count_ = 0;
    store_.set_wifi_status(WifiConnectStatus::Connected);
    if (store_.state().onboarding.step == OnboardingStep::Wifi) {
        store_.set_onboarding_step(OnboardingStep::TimezoneAndFormat);
    }
    // Fire-and-forget: ClockService picks up the synced time via time() once
    // it lands (ADR-0021), no need to wait for it here.
    esp_netif_sntp_start();
}

void SetupService::on_wifi_disconnected() {
    // Only retry/report while actively trying to connect - a disconnect
    // once already Connected (e.g. router reboot) is the resilience layer's
    // concern (Fase 8 circuit breaker), not this service's.
    if (store_.state().onboarding.wifi_status != WifiConnectStatus::Connecting) return;

    constexpr int kMaxRetries = 5;
    if (wifi_retry_count_ < kMaxRetries) {
        ++wifi_retry_count_;
        ESP_LOGW(kTag, "Wi-Fi disconnected while connecting - retry %d/%d",
                 wifi_retry_count_, kMaxRetries);
        esp_wifi_connect();  // status stays Connecting; on_wifi_connected()/another
                              // disconnect will resolve it - the same flow as the
                              // first attempt, found necessary on real hardware
                              // where the boot-time auto-reconnect doesn't always
                              // succeed on its first try even with correct creds.
    } else {
        ESP_LOGW(kTag, "Wi-Fi disconnected while connecting - giving up after %d retries",
                 kMaxRetries);
        store_.set_wifi_status(WifiConnectStatus::Failed);
    }
}

}  // namespace nova
