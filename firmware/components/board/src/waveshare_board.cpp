// NovaPainel - board/waveshare_board.cpp
// Real bring-up for the Waveshare ESP32-P4-WIFI6-Touch-LCD-7B, validated in
// Fase 0 (docs/FASE0-CHECKLIST.md, gates 5/6/9) via
// firmware/experiments/gate15_coexistence. NOT host-checkable: pulls in the
// real display/touch BSP and Wi-Fi stack, none of which exist as host shims
// (see tools/scripts/host_check.sh, which skips this file on purpose).
#include "waveshare_board.hpp"

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "bsp/esp32_p4_wifi6_touch_lcd_7b.h"
#include "bsp/touch.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

namespace nova {

namespace {
constexpr const char* kTag = "WaveshareBoard";

bool bring_up_display_and_touch() {
    bsp_display_config_t disp_cfg = {};
    esp_lcd_panel_handle_t panel = nullptr;
    esp_lcd_panel_io_handle_t io = nullptr;
    esp_err_t err = bsp_display_new(&disp_cfg, &panel, &io);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "bsp_display_new failed: %s", esp_err_to_name(err));
        return false;
    }

    // On this board's EK79007 driver, panel->disp_on_off is left NULL, so the
    // esp_lcd framework returns ESP_ERR_NOT_SUPPORTED (not a generic
    // failure - see esp_lcd_panel_ops.c). The panel is effectively always
    // "on" once esp_lcd_panel_init() succeeds (done inside bsp_display_new);
    // visibility is controlled by the backlight below. Only
    // ESP_ERR_NOT_SUPPORTED is expected here - anything else is a real error.
    esp_err_t disp_err = esp_lcd_panel_disp_on_off(panel, true);
    if (disp_err != ESP_OK && disp_err != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(kTag, "esp_lcd_panel_disp_on_off failed: %s", esp_err_to_name(disp_err));
        return false;
    }

    if (bsp_display_brightness_init() != ESP_OK || bsp_display_backlight_on() != ESP_OK) {
        ESP_LOGE(kTag, "backlight init/on failed");
        return false;
    }
    ESP_LOGI(kTag, "display (EK79007) ready, backlight on");

    esp_lcd_touch_handle_t touch = nullptr;
    bsp_touch_config_t touch_cfg = {};
    err = bsp_touch_new(&touch_cfg, &touch);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "bsp_touch_new failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(kTag, "touch (GT911) ready");
    return true;
}

// Brings up the ESP-Hosted/SDIO transport to the ESP32-C6 only. Deliberately
// does not set STA credentials or call esp_wifi_connect() - associating with
// a real AP is a service-layer concern (Fase 5), not board bring-up.
bool bring_up_network_transport() {
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK || nvs_flash_init() != ESP_OK) {
            ESP_LOGE(kTag, "nvs_flash_init failed after erase");
            return false;
        }
    } else if (nvs_err != ESP_OK) {
        ESP_LOGE(kTag, "nvs_flash_init failed: %s", esp_err_to_name(nvs_err));
        return false;
    }

    if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) {
        ESP_LOGE(kTag, "netif/event loop init failed");
        return false;
    }
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK ||
        esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_start failed (ESP-Hosted/SDIO link to C6 not up)");
        return false;
    }
    ESP_LOGI(kTag, "ESP-Hosted/SDIO transport to C6 up (no AP association yet)");
    return true;
}

}  // namespace

BoardStatus WaveshareBoard::bring_up() {
    ESP_LOGI(kTag, "bring-up: Waveshare ESP32-P4-WIFI6-Touch-LCD-7B");

    status_ = BoardStatus{};
    status_.display_ready = false;
    status_.touch_ready = false;

    const bool display_and_touch_ok = bring_up_display_and_touch();
    // bsp_display_new/bsp_touch_new don't report which one failed when both
    // run together; treat the pair atomically since both ride the same BSP.
    status_.display_ready = display_and_touch_ok;
    status_.touch_ready = display_and_touch_ok;

    status_.network_ready = bring_up_network_transport();
    status_.board_ready = display_and_touch_ok && status_.network_ready;

    ESP_LOGI(kTag, "bring-up complete: board=%d display=%d touch=%d network=%d",
             status_.board_ready, status_.display_ready, status_.touch_ready,
             status_.network_ready);
    return status_;
}

}  // namespace nova
