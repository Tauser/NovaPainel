// NovaPainel - Gate 15 coexistence harness
// ---------------------------------------------------------------------------
// Goal (docs/FASE0-CHECKLIST.md, Gate 15): prove that, on the real ESP32-P4 +
// ESP32-C6 board, the following can run AT THE SAME TIME for at least 8h
// (reduced from 72h, decision 2026-06-23) with ZERO unintended resets and
// without the UI ever freezing:
//   - a display framebuffer churned in PSRAM (render load),
//   - touch polling,
//   - sustained Wi-Fi + periodic HTTPS (the MarketService profile, ADR-0006).
//
// This is a Fase 0 experiment, NOT product code. It is plain C (the ESP-IDF
// bring-up idiom) and depends on no NovaPainel component, so it can be flashed
// and iterated independently from firmware/main.
//
// WHAT IS REAL: PSRAM framebuffer allocation + per-second churn pushed to the
// real EK79007 panel via the official Waveshare BSP (Gate 5), real GT911
// touch polling via the same BSP (Gate 6), Wi-Fi connect with
// disconnect/reconnect counting, HTTPS GET loop, NTP sync (Gate 11), SD card
// mount + periodic write/read (Gates 7/13), and full instrumentation
// (uptime, reset reason, reboot counter in NVS, heap/PSRAM watermark).
// No remaining hooks/stubs - everything this harness logs is a real driver
// call on real hardware.
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "esp_ldo_regulator.h"

#include "bsp/esp32_p4_wifi6_touch_lcd_7b.h"
#include "bsp/touch.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

// --- Configuration ----------------------------------------------------------
// Wi-Fi credentials come from `idf.py menuconfig` (Gate 15 coexistence harness
// menu -> Kconfig.projbuild) and live only in the local, gitignored sdkconfig.
// Never hardcode real credentials here or in sdkconfig.defaults.
#define GATE15_WIFI_SSID        CONFIG_GATE15_WIFI_SSID
#define GATE15_WIFI_PASS        CONFIG_GATE15_WIFI_PASS

// HTTPS target: matches the real MVP data source (CoinGecko REST). A small,
// cache-friendly endpoint is enough to exercise the TLS + SDIO path.
#define GATE15_HTTPS_URL        "https://api.coingecko.com/api/v3/ping"

// Cadences (ms).
#define GATE15_HTTPS_PERIOD_MS  60000   // MarketService profile: 1 req / 60s
#define GATE15_STATS_PERIOD_MS  60000   // log instrumentation every 60s
#define GATE15_RENDER_PERIOD_MS 1000    // churn the framebuffer ~1x/s
#define GATE15_BURST_EVERY_MS   3600000 // once an hour...
#define GATE15_BURST_COUNT      12      // ...send a 12-req burst (1 / 5s) to stress SDIO
#define GATE15_BURST_GAP_MS     5000

// Display (1024x600, RGB565 = 2 bytes/px ~= 1.2 MB). Confirm real resolution and
// color format (MIPI-DSI RGB565/666/888) in Fase 0 (Gate 5).
#define GATE15_FB_WIDTH         1024
#define GATE15_FB_HEIGHT        600
#define GATE15_FB_BYTES_PER_PX  2
#define GATE15_FB_SIZE          ((size_t)GATE15_FB_WIDTH * GATE15_FB_HEIGHT * GATE15_FB_BYTES_PER_PX)

// SD card pins (Gate 7, confirmed via official Waveshare BSP -
// docs/waveshare_esp32_p4_wifi6_touch_lcd_7b_hardware.md).
#define GATE15_SD_CLK GPIO_NUM_43
#define GATE15_SD_CMD GPIO_NUM_44
#define GATE15_SD_D0  GPIO_NUM_39
#define GATE15_SD_D1  GPIO_NUM_40
#define GATE15_SD_D2  GPIO_NUM_41
#define GATE15_SD_D3  GPIO_NUM_42
#define GATE15_SD_MOUNT_POINT "/sdcard"
#define GATE15_SD_PERIOD_MS   60000  // exercise SD + Wi-Fi at the same time (Gate 13)

#define GATE15_NTP_SERVER "pool.ntp.org"

static const char* TAG = "gate15";

// --- Shared instrumentation counters (single-writer per field; volatile is
//     enough for periodic logging, no strict ordering needed) ----------------
static volatile uint32_t s_wifi_connects    = 0;
static volatile uint32_t s_wifi_disconnects = 0;  // proxy for "spurious reassoc"
static volatile uint32_t s_https_ok         = 0;
static volatile uint32_t s_https_fail       = 0;
static volatile uint32_t s_render_ticks     = 0;
static volatile bool     s_wifi_connected   = false;
static volatile bool     s_ntp_synced       = false;
static volatile uint32_t s_sd_ok            = 0;
static volatile uint32_t s_sd_fail          = 0;
static volatile bool     s_sd_mounted       = false;

static uint16_t* s_framebuffer = NULL;
static uint32_t  s_reboot_count = 0;

static EventGroupHandle_t s_wifi_eg;
#define WIFI_CONNECTED_BIT BIT0

// --- Helpers ----------------------------------------------------------------
static const char* reset_reason_str(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_SW:        return "SW";
        case ESP_RST_PANIC:     return "PANIC";       // <-- red flag for Gate 15
        case ESP_RST_INT_WDT:   return "INT_WDT";     // <-- red flag
        case ESP_RST_TASK_WDT:  return "TASK_WDT";    // <-- red flag
        case ESP_RST_WDT:       return "OTHER_WDT";   // <-- red flag
        case ESP_RST_BROWNOUT:  return "BROWNOUT";    // <-- Gate 16 (power/thermal)
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_EXT:       return "EXT";
        default:                return "UNKNOWN";
    }
}

static uint32_t uptime_s(void) {
    return (uint32_t)(esp_timer_get_time() / 1000000LL);
}

// Persist a reboot counter in NVS so a long run shows total resets at a glance.
static void load_and_bump_reboot_count(void) {
    nvs_handle_t h;
    if (nvs_open("gate15", NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open failed; reboot counter unavailable");
        return;
    }
    nvs_get_u32(h, "reboots", &s_reboot_count);  // stays 0 if absent
    s_reboot_count++;
    nvs_set_u32(h, "reboots", s_reboot_count);
    nvs_commit(h);
    nvs_close(h);
}

// --- Wi-Fi (served by esp_wifi_remote / ESP-Hosted on the C6) ---------------
static void wifi_event_handler(void* arg, esp_event_base_t base,
                               int32_t id, void* data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_disconnects++;
        s_wifi_connected = false;
        xEventGroupClearBits(s_wifi_eg, WIFI_CONNECTED_BIT);
        // Reconnect in background; never block the render path (offline-first).
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_wifi_connects++;
        s_wifi_connected = true;
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi connected (got IP)");
    }
}

static void wifi_start(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wc = { 0 };
    strncpy((char*)wc.sta.ssid, GATE15_WIFI_SSID, sizeof(wc.sta.ssid) - 1);
    strncpy((char*)wc.sta.password, GATE15_WIFI_PASS, sizeof(wc.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi started (via ESP-Hosted/C6); connecting to '%s'", GATE15_WIFI_SSID);
}

// --- HTTPS ------------------------------------------------------------------
static void do_one_https_get(void) {
    esp_http_client_config_t cfg = {
        .url = GATE15_HTTPS_URL,
        .timeout_ms = 8000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) { s_https_fail++; return; }
    esp_err_t err = esp_http_client_perform(c);
    if (err == ESP_OK && esp_http_client_get_status_code(c) == 200) {
        s_https_ok++;
    } else {
        s_https_fail++;
        ESP_LOGW(TAG, "HTTPS failed: %s (status %d)",
                 esp_err_to_name(err), esp_http_client_get_status_code(c));
    }
    esp_http_client_cleanup(c);
}

static void https_task(void* arg) {
    int64_t last_burst = 0;
    for (;;) {
        // Only attempt when connected; otherwise count nothing and wait (the UI
        // must keep running regardless - that is the whole point).
        if (s_wifi_connected) {
            do_one_https_get();

            int64_t now = esp_timer_get_time() / 1000;
            if (now - last_burst >= GATE15_BURST_EVERY_MS) {
                ESP_LOGI(TAG, "SDIO burst: %d requests", GATE15_BURST_COUNT);
                for (int i = 0; i < GATE15_BURST_COUNT && s_wifi_connected; ++i) {
                    do_one_https_get();
                    vTaskDelay(pdMS_TO_TICKS(GATE15_BURST_GAP_MS));
                }
                last_burst = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(GATE15_HTTPS_PERIOD_MS));
    }
}

// --- NTP (Gate 11) -----------------------------------------------------------
static void ntp_sync_notification_cb(struct timeval* tv) {
    (void)tv;
    s_ntp_synced = true;
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    ESP_LOGW(TAG, "NTP synced: %s", buf);
}

static void ntp_start(void) {
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(GATE15_NTP_SERVER);
    cfg.sync_cb = ntp_sync_notification_cb;
    ESP_ERROR_CHECK(esp_netif_sntp_init(&cfg));
}

// --- SD card (Gate 7, exercised alongside Wi-Fi/HTTPS for Gate 13) ----------
static sdmmc_card_t* s_sd_card = NULL;

// Per docs/waveshare_esp32_p4_wifi6_touch_lcd_7b_hardware.md (section 8), the
// SD slot is powered from the P4's internal LDO, channel 4 ("VO4") - same
// idea as the MIPI-DSI PHY needing LDO channel 3. Without this the card never
// responds (sdmmc_init_ocr times out) even with a card physically inserted.
#define GATE15_SD_LDO_CHAN    4
#define GATE15_SD_LDO_MV      3300

static void sd_mount(void) {
    esp_ldo_channel_handle_t ldo = NULL;
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = GATE15_SD_LDO_CHAN,
        .voltage_mv = GATE15_SD_LDO_MV,
    };
    esp_err_t ldo_err = esp_ldo_acquire_channel(&ldo_cfg, &ldo);
    if (ldo_err != ESP_OK) {
        ESP_LOGW(TAG, "SD LDO channel %d enable failed: %s",
                 GATE15_SD_LDO_CHAN, esp_err_to_name(ldo_err));
        // Keep going - some boards may not need/have this LDO; let the SDMMC
        // init below fail on its own if power really is the issue.
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = GATE15_SD_CLK;
    slot_config.cmd = GATE15_SD_CMD;
    slot_config.d0  = GATE15_SD_D0;
    slot_config.d1  = GATE15_SD_D1;
    slot_config.d2  = GATE15_SD_D2;
    slot_config.d3  = GATE15_SD_D3;
    slot_config.width = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

    esp_err_t err = esp_vfs_fat_sdmmc_mount(GATE15_SD_MOUNT_POINT, &host, &slot_config,
                                             &mount_config, &s_sd_card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD mount failed: %s (Gate 7 not ready / no card inserted?)",
                 esp_err_to_name(err));
        return;
    }
    s_sd_mounted = true;
    ESP_LOGI(TAG, "SD card mounted at %s", GATE15_SD_MOUNT_POINT);
}

// Write + read back a small file every cycle - cheap, real I/O that overlaps
// with Wi-Fi/HTTPS traffic on purpose (Gate 13: SD + Wi-Fi simultaneous).
static void sd_task(void* arg) {
    if (!s_sd_mounted) {
        vTaskDelete(NULL);
        return;
    }
    char path[64];
    snprintf(path, sizeof(path), "%s/gate15.txt", GATE15_SD_MOUNT_POINT);

    for (;;) {
        FILE* f = fopen(path, "a");
        bool ok = false;
        if (f) {
            if (fprintf(f, "tick up=%" PRIu32 "s\n", uptime_s()) > 0) {
                ok = (fclose(f) == 0);
            } else {
                fclose(f);
            }
        }
        if (ok) {
            // Read back to also exercise the read path, not just append.
            FILE* rf = fopen(path, "r");
            if (rf) {
                char line[64];
                ok = (fgets(line, sizeof(line), rf) != NULL);
                fclose(rf);
            } else {
                ok = false;
            }
        }
        if (ok) { s_sd_ok++; } else {
            s_sd_fail++;
            ESP_LOGW(TAG, "SD write/read failed");
        }
        vTaskDelay(pdMS_TO_TICKS(GATE15_SD_PERIOD_MS));
    }
}

// --- Display + touch (Gates 5/6, via the official Waveshare BSP) -----------
static esp_lcd_panel_handle_t s_panel = NULL;
static esp_lcd_touch_handle_t s_touch = NULL;
static volatile uint32_t s_touch_points = 0;

static void display_touch_init(void) {
    bsp_display_config_t disp_cfg = {0};
    esp_lcd_panel_io_handle_t io = NULL;
    esp_err_t err = bsp_display_new(&disp_cfg, &s_panel, &io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_new failed: %s (Gate 5)", esp_err_to_name(err));
        s_panel = NULL;
    } else {
        // On this board's EK79007 driver (espressif/esp_lcd_ek79007 v2.0.2),
        // the panel->disp_on_off function pointer is left NULL, so the
        // esp_lcd framework itself returns ESP_ERR_NOT_SUPPORTED (see
        // esp_lcd_panel_ops.c) - it's not a generic failure. The panel is
        // effectively always "on" once esp_lcd_panel_init() succeeds (done
        // inside bsp_display_new); visibility is controlled by the
        // backlight below. Only ESP_ERR_NOT_SUPPORTED is expected here -
        // anything else is a real error and should still abort.
        esp_err_t disp_err = esp_lcd_panel_disp_on_off(s_panel, true);
        if (disp_err == ESP_ERR_NOT_SUPPORTED) {
            ESP_LOGW(TAG, "disp_on_off not supported by this panel/driver - expected on the Waveshare 7B, using backlight instead");
        } else {
            ESP_ERROR_CHECK(disp_err);
        }
        ESP_ERROR_CHECK(bsp_display_brightness_init());
        ESP_ERROR_CHECK(bsp_display_backlight_on());
        ESP_LOGI(TAG, "Display (EK79007) initialized, backlight on (Gate 5)");
    }

    bsp_touch_config_t touch_cfg = {0};
    err = bsp_touch_new(&touch_cfg, &s_touch);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_touch_new failed: %s (Gate 6)", esp_err_to_name(err));
        s_touch = NULL;
    } else {
        ESP_LOGI(TAG, "Touch (GT911) initialized (Gate 6)");
    }
}

// Push the framebuffer to the real panel.
static void panel_blit(const uint16_t* fb) {
    if (!s_panel) { return; }
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, GATE15_FB_WIDTH, GATE15_FB_HEIGHT, fb);
}

// Read the touch controller.
static void touch_poll(void) {
    if (!s_touch) { return; }
    esp_lcd_touch_read_data(s_touch);
    uint16_t x[1], y[1];
    uint8_t n = 0;
    bool pressed = esp_lcd_touch_get_coordinates(s_touch, x, y, NULL, &n, 1);
    if (pressed && n > 0) { s_touch_points++; }
}

static void render_task(void* arg) {
    uint8_t frame = 0;
    for (;;) {
        // Churn the whole framebuffer in PSRAM each tick: simulates a redraw and
        // stresses PSRAM bandwidth / cache concurrently with Wi-Fi DMA. This is
        // the part that can trigger the reported Wi-Fi+display reset.
        if (s_framebuffer) {
            uint16_t color = (uint16_t)((frame << 8) | frame);  // changing pattern
            for (size_t i = 0; i < (size_t)GATE15_FB_WIDTH * GATE15_FB_HEIGHT; ++i) {
                s_framebuffer[i] = color;
            }
            panel_blit(s_framebuffer);
        }
        touch_poll();
        s_render_ticks++;
        frame++;
        vTaskDelay(pdMS_TO_TICKS(GATE15_RENDER_PERIOD_MS));
    }
}

// --- Instrumentation --------------------------------------------------------
static void stats_task(void* arg) {
    for (;;) {
        size_t free_int   = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t largest_int= heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t largest_ps = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

        ESP_LOGI(TAG,
            "[STAT] up=%" PRIu32 "s reboots=%" PRIu32
            " wifi(conn=%" PRIu32 "/disc=%" PRIu32 " %s)"
            " https(ok=%" PRIu32 "/fail=%" PRIu32 ")"
            " ntp=%s"
            " sd(mounted=%s ok=%" PRIu32 "/fail=%" PRIu32 ")"
            " render=%" PRIu32
            " display=%s touch=%s(points=%" PRIu32 ")"
            " heap_int(free=%u largest=%u)"
            " psram(free=%u largest=%u)",
            uptime_s(), s_reboot_count,
            s_wifi_connects, s_wifi_disconnects,
            s_wifi_connected ? "UP" : "DOWN",
            s_https_ok, s_https_fail,
            s_ntp_synced ? "synced" : "pending",
            s_sd_mounted ? "yes" : "no", s_sd_ok, s_sd_fail,
            s_render_ticks,
            s_panel ? "on" : "FAIL", s_touch ? "ok" : "FAIL", s_touch_points,
            (unsigned)free_int, (unsigned)largest_int,
            (unsigned)free_psram, (unsigned)largest_ps);

        vTaskDelay(pdMS_TO_TICKS(GATE15_STATS_PERIOD_MS));
    }
}

// --- Entry point ------------------------------------------------------------
void app_main(void) {
    // 1) Report why we (re)booted FIRST - a silent PANIC/WDT/BROWNOUT here is the
    //    headline result of the whole test.
    esp_reset_reason_t rr = esp_reset_reason();

    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    load_and_bump_reboot_count();

    ESP_LOGW(TAG, "==== Gate 15 coexistence harness ====");
    ESP_LOGW(TAG, "boot #%" PRIu32 " reset_reason=%s",
             s_reboot_count, reset_reason_str(rr));

    // 2) Allocate the framebuffer in PSRAM (Gate 4/5). If this fails, PSRAM is the
    //    problem - stop here, that is itself a Fase 0 finding.
    s_framebuffer = (uint16_t*)heap_caps_malloc(GATE15_FB_SIZE, MALLOC_CAP_SPIRAM);
    if (!s_framebuffer) {
        ESP_LOGE(TAG, "PSRAM framebuffer alloc FAILED (%u bytes). Check PSRAM/config.",
                 (unsigned)GATE15_FB_SIZE);
    } else {
        ESP_LOGI(TAG, "framebuffer in PSRAM ok: %u bytes @ %p",
                 (unsigned)GATE15_FB_SIZE, s_framebuffer);
    }

    // 3) Wi-Fi up (ESP-Hosted/C6).
    s_wifi_eg = xEventGroupCreate();
    wifi_start();
    ntp_start();  // Gate 11: SNTP syncs once Wi-Fi has an IP, no extra wiring needed.

    // 3b) SD card (Gate 7). Mount before starting the competing tasks so
    //     sd_task can exercise it alongside Wi-Fi/HTTPS (Gate 13).
    sd_mount();

    // 3c) Display + touch (Gates 5/6), via the official Waveshare BSP.
    display_touch_init();

    // 4) Start the competing loads on their own tasks. Pin render and
    //    network to different cores (P4 is dual-core) to mirror production
    //    (ADR-0013) and to maximize the chance of surfacing a coexistence bug.
    xTaskCreatePinnedToCore(render_task, "render", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(https_task,  "https",  6144, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(sd_task,     "sd",     4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(stats_task,  "stats",  4096, NULL, 4, NULL, 1);

    ESP_LOGW(TAG, "harness running. Let it soak >=8h; watch [STAT] lines and any reset.");
}
