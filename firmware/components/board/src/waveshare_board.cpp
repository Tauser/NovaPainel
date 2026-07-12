#include "waveshare_board.hpp"

#include "bsp/esp32_p4_wifi6_touch_lcd_7b.h"
#include "esp_log.h"

#include <algorithm>
#include <cstring>
#include <ctime>

#if defined(ESP_PLATFORM) && __has_include("esp_event.h") && __has_include("esp_netif.h") && __has_include("esp_wifi.h")
#define NOVA_HAS_WIFI_TRANSPORT 1
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#define NOVA_HAS_WIFI_TRANSPORT 0
#endif

namespace nova {
namespace {
constexpr const char* kTag = "WaveshareBoard";
constexpr uint32_t kPartialRenderRows = BSP_LCD_V_RES / 10;
constexpr uint16_t kMaxScanResults = 12;

#if NOVA_HAS_WIFI_TRANSPORT
void network_transport_task(void* context) {
    auto* board = static_cast<WaveshareBoard*>(context);
    if (board != nullptr) {
        board->start_network_transport();
    }
    vTaskDelete(nullptr);
}

// Single trampoline for both WIFI_EVENT and IP_EVENT, bound to a WaveshareBoard
// instance via arg. Not a static std::function (PROIBIDO per CLAUDE.md) --
// plain function pointer + instance context.
void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void*) {
    auto* board = static_cast<WaveshareBoard*>(arg);
    if (board == nullptr) {
        return;
    }
    board->on_wifi_event(base, id);
}
#endif
}  // namespace

#if NOVA_HAS_WIFI_TRANSPORT
void WaveshareBoard::on_wifi_event(const void* base, int32_t id) {
    const auto event_base = static_cast<esp_event_base_t>(base);
    if (event_base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        wifi_scan_done_pending_.store(true);
    } else if (event_base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_link_event_.store(static_cast<uint8_t>(WifiLinkEvent::Disconnected));
    } else if (event_base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        wifi_link_event_.store(static_cast<uint8_t>(WifiLinkEvent::Connected));
    }
}
#else
void WaveshareBoard::on_wifi_event(const void*, int32_t) {}
#endif

bool WaveshareBoard::start_network_transport() {
#if NOVA_HAS_WIFI_TRANSPORT
#if !defined(CONFIG_ESP_HOSTED_ENABLED)
    ESP_LOGW(kTag,
             "ESP-Hosted disabled in sdkconfig; remote Wi-Fi backend for ESP32-C6 is not active");
    network_ready_.store(false);
    return false;
#else
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_err = nvs_flash_erase();
        if (nvs_err != ESP_OK) {
            ESP_LOGE(kTag, "nvs_flash_erase failed: %s", esp_err_to_name(nvs_err));
            network_ready_.store(false);
            return false;
        }
        nvs_err = nvs_flash_init();
    }
    if (nvs_err != ESP_OK && nvs_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(kTag, "nvs_flash_init failed: %s", esp_err_to_name(nvs_err));
        network_ready_.store(false);
        return false;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(kTag, "esp_netif_init failed: %s", esp_err_to_name(err));
        network_ready_.store(false);
        return false;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(kTag, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        network_ready_.store(false);
        return false;
    }

    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == nullptr) {
        if (esp_netif_create_default_wifi_sta() == nullptr) {
            ESP_LOGE(kTag, "esp_netif_create_default_wifi_sta failed");
            network_ready_.store(false);
            return false;
        }
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
        ESP_LOGE(kTag, "esp_wifi_init failed: %s", esp_err_to_name(err));
        network_ready_.store(false);
        return false;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        network_ready_.store(false);
        return false;
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        ESP_LOGE(kTag, "esp_wifi_start failed: %s", esp_err_to_name(err));
        network_ready_.store(false);
        return false;
    }

    if (!wifi_events_registered_) {
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                                            this, nullptr);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler,
                                            this, nullptr);
        wifi_events_registered_ = true;
    }

    ESP_LOGI(kTag, "ESP-Hosted/SDIO transport requested through esp_wifi_remote");
    network_ready_.store(true);
    return true;
#endif
#else
    ESP_LOGW(kTag, "Wi-Fi transport support not compiled for this target");
    network_ready_.store(false);
    return false;
#endif
}

void WaveshareBoard::start_network_transport_async() {
#if NOVA_HAS_WIFI_TRANSPORT && defined(CONFIG_ESP_HOSTED_ENABLED)
    if (network_task_started_) {
        return;
    }
    network_task_started_ = true;
    if (xTaskCreate(&network_transport_task, "nova_net_xport", 4096, this, tskIDLE_PRIORITY + 1,
                     nullptr) != pdPASS) {
        ESP_LOGE(kTag, "failed to spawn network transport task");
        network_task_started_ = false;
        network_ready_.store(false);
    }
#else
    network_ready_.store(start_network_transport());
#endif
}

bool WaveshareBoard::init_display() {
    if (display_initialized_) {
        return status_.display_ready;
    }

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * kPartialRenderRows,
        .double_buffer = true,
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .sw_rotate = true,
        },
    };
    cfg.lvgl_port_cfg.task_stack = 16 * 1024;

    lv_display_t* display = bsp_display_start_with_config(&cfg);
    if (display == nullptr) {
        ESP_LOGE(kTag, "display init failed");
        status_.display_ready = false;
        status_.touch_ready = false;
        return false;
    }

    bsp_display_rotate(display, LV_DISPLAY_ROTATION_180);
    bsp_display_backlight_off();
    status_.display_ready = true;
    status_.touch_ready = true;
    display_initialized_ = true;
    ESP_LOGI(kTag, "display initialized; backlight deferred until first UI frame");
    return true;
}

BoardStatus WaveshareBoard::bring_up() {
    status_ = BoardStatus{};
    display_initialized_ = false;
    network_ready_.store(false);
    init_display();
    status_.board_ready = status_.display_ready;
    return status_;
}

BoardStatus WaveshareBoard::status() const {
    BoardStatus current = status_;
    current.network_ready = network_ready_.load();
    return current;
}

bool WaveshareBoard::lock_ui(uint32_t timeout_ms) {
    return bsp_display_lock(timeout_ms);
}

void WaveshareBoard::unlock_ui() {
    bsp_display_unlock();
}

bool WaveshareBoard::lock_shared_i2c(uint32_t timeout_ms) {
    return lock_ui(timeout_ms);
}

void WaveshareBoard::unlock_shared_i2c() {
    unlock_ui();
}

void WaveshareBoard::set_brightness(int pct) {
    if (pct > 0) {
        bsp_display_backlight_on();
    } else {
        bsp_display_backlight_off();
    }
}

uint64_t WaveshareBoard::rtc_unix_time_s() const {
    const std::time_t now = std::time(nullptr);
    if (now <= 0) {
        return 0;
    }
    return static_cast<uint64_t>(now);
}

bool WaveshareBoard::wifi_scan_start() {
#if NOVA_HAS_WIFI_TRANSPORT
    if (!network_ready_.load()) {
        return false;
    }
    if (wifi_scan_status_ == WifiScanStatus::Scanning) {
        return true;
    }
    const esp_err_t scan_err = esp_wifi_scan_start(nullptr, false);
    if (scan_err != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_scan_start failed: %s", esp_err_to_name(scan_err));
        return false;
    }
    wifi_scan_done_pending_.store(false);
    wifi_scan_status_ = WifiScanStatus::Scanning;
    ESP_LOGI(kTag, "wifi scan started");
    return true;
#else
    return false;
#endif
}

WifiScanStatus WaveshareBoard::wifi_scan_poll() {
#if NOVA_HAS_WIFI_TRANSPORT
    if (wifi_scan_status_ != WifiScanStatus::Scanning) {
        return wifi_scan_status_;
    }
    if (!wifi_scan_done_pending_.exchange(false)) {
        return wifi_scan_status_;
    }

    uint16_t found = 0;
    esp_wifi_scan_get_ap_num(&found);
    found = std::min(found, kMaxScanResults);

    std::vector<wifi_ap_record_t> records(found);
    if (found > 0 && esp_wifi_scan_get_ap_records(&found, records.data()) != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_scan_get_ap_records failed");
        wifi_scan_status_ = WifiScanStatus::Failed;
        wifi_scan_results_.clear();
        return wifi_scan_status_;
    }

    std::vector<WifiNetwork> networks;
    networks.reserve(found);
    for (uint16_t i = 0; i < found; ++i) {
        const auto& rec = records[i];
        const char* ssid = reinterpret_cast<const char*>(rec.ssid);
        if (ssid[0] == '\0') {
            continue;
        }

        auto it = std::find_if(networks.begin(), networks.end(),
                               [&](const WifiNetwork& network) { return network.ssid == ssid; });
        if (it != networks.end()) {
            if (rec.rssi > it->rssi) {
                it->rssi = rec.rssi;
            }
            continue;
        }

        networks.push_back(
            WifiNetwork{ssid, static_cast<int8_t>(rec.rssi), rec.authmode != WIFI_AUTH_OPEN});
    }

    std::sort(networks.begin(), networks.end(),
              [](const WifiNetwork& a, const WifiNetwork& b) { return a.rssi > b.rssi; });

    wifi_scan_results_ = std::move(networks);
    wifi_scan_status_ = WifiScanStatus::Done;
    return wifi_scan_status_;
#else
    return WifiScanStatus::Idle;
#endif
}

std::vector<WifiNetwork> WaveshareBoard::wifi_take_scan_results() {
    wifi_scan_status_ = WifiScanStatus::Idle;
    return std::move(wifi_scan_results_);
}

bool WaveshareBoard::wifi_connect(const std::string& ssid, const std::string& password) {
#if NOVA_HAS_WIFI_TRANSPORT
    if (!network_ready_.load() || ssid.empty()) {
        return false;
    }

    wifi_config_t wifi_config{};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid.c_str(),
                 sizeof(wifi_config.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password.c_str(),
                 sizeof(wifi_config.sta.password) - 1);

    const esp_err_t config_err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (config_err != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_set_config failed: %s", esp_err_to_name(config_err));
        return false;
    }

    if (esp_wifi_connect() != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_connect failed to start");
        return false;
    }
    return true;
#else
    (void)ssid;
    (void)password;
    return false;
#endif
}

WifiLinkEvent WaveshareBoard::wifi_take_connect_event() {
#if NOVA_HAS_WIFI_TRANSPORT
    return static_cast<WifiLinkEvent>(wifi_link_event_.exchange(0));
#else
    return WifiLinkEvent::None;
#endif
}

}  // namespace nova
