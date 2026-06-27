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

#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "esp_ldo_regulator.h"

namespace nova {

namespace {
constexpr const char* kTag = "WaveshareBoard";

// bsp_display_start_with_config() is the BSP's all-in-one entry point: it
// brings up the panel + backlight pin, initializes LVGL (lvgl_port), wires
// the GT911 touch controller as the LVGL input device automatically, and
// starts the BSP's own LVGL task ("taskLVGL"). There is no separate touch
// bring-up call (unlike Fase 0's gate15 harness, which called
// bsp_touch_new() directly because it didn't use LVGL at all).
//
// Fase 10 (ADR-0031): draw buffer moved to PSRAM (buff_spiram=true).
// The original blocker (Fase 4): LV_USE_BUILTIN_MALLOC reserved a static
// 64KB pool in internal RAM at boot, leaving too little DMA-capable internal
// RAM for FreeRTOS/ESP-Hosted bookkeeping when the display buffer also tried
// to live in PSRAM. That was fixed in sdkconfig.defaults (LV_USE_CLIB_MALLOC,
// ADR-0018). With that gone, PSRAM buffers no longer starve the DMA pool.
//
// Quarter-height single buffer in PSRAM (~300KB): large enough that LVGL needs
// only ~4 flush calls per full frame instead of ~24 with BSP_LCD_DRAW_BUFF_SIZE;
// small enough that the LVGL dirty-rect flush still skips unchanged rows.
//
// double_buffer=false: required with sw_rotate+PSRAM. With double_buffer=true
// LVGL starts writing the next render region to the "other" buffer before the
// current flush completes. The flush callback reads from that buffer for SW
// rotation; with PSRAM's higher latency vs internal RAM, the rotation read and
// LVGL's next write overlap, producing one corrupted frame every few minutes
// (visible as a brief screen flash). Single buffer forces LVGL to wait for
// lv_disp_flush_ready() before reusing the buffer; no overlap, no flash.
//
// task_stack overridden (7168 -> 16384): found necessary on real hardware for
// the wizard's touch callbacks (stack overflow as taskLVGL panic+reboot) - kept
// as headroom for future screens.
bool bring_up_display_and_touch() {
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * (BSP_LCD_V_RES / 4),  // quarter-height, ~300KB
        .double_buffer = false,  // must be false with sw_rotate+PSRAM (see comment above)
        .flags = {
            .buff_dma    = true,
            .buff_spiram = true,   // Fase 10: was false (internal RAM); PSRAM now safe (ADR-0031)
            .sw_rotate   = true,
        },
    };
    cfg.lvgl_port_cfg.task_stack = 16384;

    lv_display_t* disp = bsp_display_start_with_config(&cfg);
    if (!disp) {
        ESP_LOGE(kTag, "bsp_display_start_with_config failed");
        return false;
    }
    // The BSP's hardcoded panel mirroring (bsp_display_lcd_init) renders
    // upside down on this physical unit - confirmed by eye on the real
    // display (Fase 4). Correct it via LVGL's own rotation instead of
    // patching the BSP.
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);
    if (bsp_display_backlight_on() != ESP_OK) {
        ESP_LOGE(kTag, "backlight on failed");
        return false;
    }
    ESP_LOGI(kTag, "display (EK79007) + touch (GT911) ready via BSP LVGL port");
    return true;
}

// Pins confirmed via the official BSP - docs/HARDWARE.md /
// docs/waveshare_esp32_p4_wifi6_touch_lcd_7b_hardware.md.
constexpr gpio_num_t kSdClk = GPIO_NUM_43;
constexpr gpio_num_t kSdCmd = GPIO_NUM_44;
constexpr gpio_num_t kSdD0  = GPIO_NUM_39;
constexpr gpio_num_t kSdD1  = GPIO_NUM_40;
constexpr gpio_num_t kSdD2  = GPIO_NUM_41;
constexpr gpio_num_t kSdD3  = GPIO_NUM_42;
constexpr const char* kSdMountPoint = "/sdcard";

// Per docs/HARDWARE.md (Gate 7 finding), the SD slot is powered from the
// P4's internal LDO, channel 4 ("VO4") - same idea as the MIPI-DSI PHY
// needing LDO channel 3 (handled internally by bsp_display_new). Without
// this the card never responds (sdmmc_init_ocr times out) even when
// physically inserted.
constexpr int kSdLdoChannel = 4;
constexpr int kSdLdoMillivolts = 3300;

bool bring_up_sd_card() {
    esp_ldo_channel_handle_t ldo = nullptr;
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = kSdLdoChannel,
        .voltage_mv = kSdLdoMillivolts,
        .flags = {},
    };
    esp_err_t ldo_err = esp_ldo_acquire_channel(&ldo_cfg, &ldo);
    if (ldo_err != ESP_OK) {
        ESP_LOGW(kTag, "SD LDO channel %d enable failed: %s - trying mount anyway",
                 kSdLdoChannel, esp_err_to_name(ldo_err));
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = kSdClk;
    slot_config.cmd = kSdCmd;
    slot_config.d0  = kSdD0;
    slot_config.d1  = kSdD1;
    slot_config.d2  = kSdD2;
    slot_config.d3  = kSdD3;
    slot_config.width = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false,
        .use_one_fat = false,
    };

    sdmmc_card_t* card = nullptr;
    esp_err_t err = esp_vfs_fat_sdmmc_mount(kSdMountPoint, &host, &slot_config,
                                             &mount_config, &card);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "SD mount failed: %s (no card inserted? - degrades gracefully)",
                 esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(kTag, "SD card mounted at %s", kSdMountPoint);
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

    // Order matters: bring up the ESP-Hosted/SDIO transport BEFORE the LVGL
    // display buffers. Fase 0 (gate15 harness) validated this order; doing
    // display first starves the SDIO driver's DMA-capable memory pool and
    // sdio_mempool_create() asserts (found while wiring Fase 4).
    status_.network_ready = bring_up_network_transport();

    const bool display_and_touch_ok = bring_up_display_and_touch();
    // bsp_display_start_with_config() brings up both atomically (touch is
    // wired in as the LVGL indev internally) and doesn't report which one
    // failed individually.
    status_.display_ready = display_and_touch_ok;
    status_.touch_ready = display_and_touch_ok;

    status_.board_ready = display_and_touch_ok && status_.network_ready;

    // SD card is allowed to be absent/fail without blocking board_ready -
    // it degrades gracefully (no card inserted is a normal, expected case;
    // see docs/HARDWARE.md Gate 7).
    status_.sd_ready = bring_up_sd_card();

    ESP_LOGI(kTag, "bring-up complete: board=%d display=%d touch=%d network=%d sd=%d",
             status_.board_ready, status_.display_ready, status_.touch_ready,
             status_.network_ready, status_.sd_ready);
    return status_;
}

bool WaveshareBoard::lock(uint32_t timeout_ms) {
    return bsp_display_lock(timeout_ms);
}

void WaveshareBoard::unlock() {
    bsp_display_unlock();
}

bool WaveshareBoard::enable_backlight() {
    if (bsp_display_backlight_on() != ESP_OK) {
        ESP_LOGE(kTag, "backlight on failed");
        return false;
    }
    return true;
}

}  // namespace nova
