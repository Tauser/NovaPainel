#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <functional>

#include "bsp/esp32_p4_wifi6_touch_lcd_7b.h"
#include "event_bus.hpp"
#include "clock_service.hpp"
#include "bootstrap_service.hpp"
#include "notification_service.hpp"
#include "request_orchestrator.hpp"
#include "cache_store.hpp"
#include "market_service.hpp"
#include "forex_service.hpp"
#include "weather_service.hpp"
#include "network_worker.hpp"
#include "coingecko_provider.hpp"
#include "forex_provider.hpp"
#include "open_meteo_provider.hpp"
#include "novapanel_ui.hpp"
#include "setup_service.hpp"
#include "system_service.hpp"
#include "service_manager.hpp"
#include "state_store.hpp"
#include "ui_dispatcher.hpp"
#include <cmath>

#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#if defined(ESP_PLATFORM) && __has_include("cJSON.h")
#include "cJSON.h"
#define NOVAPANEL_HAS_CJSON_HOOKS 1
#else
#define NOVAPANEL_HAS_CJSON_HOOKS 0
#endif

#if __has_include("esp_wifi.h") && __has_include("esp_netif.h") && __has_include("esp_event.h")
#define NOVAPANEL_HAS_WIFI_TRANSPORT 1
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#else
#define NOVAPANEL_HAS_WIFI_TRANSPORT 0
#endif

using namespace nova;

static const char *TAG = "NovaPanel";

// Rotacao 180 por PPA (sw_rotate). Este painel EK79007 (DSI video-mode) NAO
// gira por mirror de hardware (MADCTL sem efeito visivel), entao a rotacao
// precisa ser por software/PPA. Por isso nao usamos avoid_tearing aqui.
#define NOVAPANEL_DISPLAY_ROTATION LV_DISPLAY_ROTATION_180
#define NOVAPANEL_LVGL_LOCK_MS 1000
#define NOVAPANEL_LVGL_MAX_TIMEOUTS 5

static void log_heap_snapshot(const char *phase)
{
    ESP_LOGI(TAG,
             "%s: uptime_ms=%" PRId64 " free_internal=%u min_internal=%u free_spiram=%u min_spiram=%u",
             phase,
             esp_timer_get_time() / 1000,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
}

static void start_display(void)
{
    ESP_LOGI(TAG, "starting Waveshare BSP display");

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        // Config de producao: double buffer em PSRAM + rotacao 180 por PPA.
        //
        // Display: MIPI DSI, EK79007 controller, 1024x600 @ 60 Hz.
        // sw_rotate=true usa o PPA do ESP32-P4 p/ girar 180 (o painel nao gira
        // por mirror de hardware). avoid_tearing NAO e usado porque e
        // incompativel com sw_rotate neste BSP, e o flicker observado e do
        // backlight (GPIO32), nao tearing.
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = true,
        .flags = {
            .buff_dma    = true,
            .buff_spiram = true,
            .sw_rotate   = true,
        },
    };
    cfg.lvgl_port_cfg.task_stack = 16384;

    lv_display_t *display = bsp_display_start_with_config(&cfg);
    if (display == NULL) {
        ESP_LOGE(TAG, "display start failed");
        abort();
    }

    bsp_display_rotate(display, NOVAPANEL_DISPLAY_ROTATION);
    // NOTA: NAO forcar RENDER_MODE_FULL. Em modo parcial o LVGL so redesenha
    // a regiao suja, reduzindo muito a banda de PSRAM gasta por update (PPA +
    // escrita no framebuffer). Isso deixa banda livre para o DSI ler o
    // framebuffer e evita o underrun (flash branco) quando a rede esta ativa.
    ESP_LOGI(TAG, "display rotation=%d sw_rotate=1(PPA) render=partial buf=%ux%u", (int)NOVAPANEL_DISPLAY_ROTATION, (unsigned)BSP_LCD_H_RES, (unsigned)BSP_LCD_V_RES);
}

static bool with_lvgl_lock(const char* phase, int& consecutive_failures,
                           const std::function<void()>& fn)
{
    if (!bsp_display_lock(NOVAPANEL_LVGL_LOCK_MS)) {
        ++consecutive_failures;
        ESP_LOGE(TAG, "LVGL lock timeout during %s (%d/%d)",
                 phase, consecutive_failures, NOVAPANEL_LVGL_MAX_TIMEOUTS);
        if (consecutive_failures >= NOVAPANEL_LVGL_MAX_TIMEOUTS) {
            ESP_LOGE(TAG, "LVGL remained unavailable after repeated timeouts; keeping system alive");
        }
        return false;
    }

    consecutive_failures = 0;
    fn();
    bsp_display_unlock();
    return true;
}

// ---------------------------------------------------------------------------
// Áudio: ES8311 via I2C/I2S — beep de feedback no slider de volume.
// ---------------------------------------------------------------------------
// ES8311 (codec) e GT911 (touch) compartilham o mesmo barramento I2C.
// O LVGL port faz polling de touch dentro do lv_timer_handler, segurando o
// display lock. Para serializar o acesso ao I2C, qualquer operação de registro
// do codec (init, set_vol, open) precisa adquirir esse mesmo lock antes.
// Operações I2S (esp_codec_dev_write) são independentes e não precisam do lock.

static esp_codec_dev_handle_t g_spk_dev = nullptr;
static int16_t*               g_beep_buf = nullptr;
static i2s_chan_handle_t      g_audio_tx_chan = nullptr;
static i2s_chan_handle_t      g_audio_rx_chan = nullptr;

namespace {
constexpr int kBeepSampleRate = 22050;
constexpr int kBeepHz         = 880;   // A5 — claro num speaker pequeno
constexpr int kBeepDurationMs = 160;
constexpr int kBeepSamples    = kBeepSampleRate * kBeepDurationMs / 1000; // ~1764
constexpr int kCodecVolumeFloor = 30;
constexpr double kPi          = 3.14159265358979323846;
} // namespace

static int audio_codec_volume_from_ui(int ui_pct)
{
    if (ui_pct <= 0) {
        return 0;
    }
    if (ui_pct >= 100) {
        return 100;
    }

    const double normalized = static_cast<double>(ui_pct) / 100.0;
    const double curved = std::pow(normalized, 0.75);
    return kCodecVolumeFloor +
           static_cast<int>(std::lround((100 - kCodecVolumeFloor) * curved));
}

static bool prepare_beep_buffer()
{
    if (g_beep_buf) {
        return true;
    }

    g_beep_buf = static_cast<int16_t*>(
        heap_caps_malloc(kBeepSamples * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!g_beep_buf) {
        ESP_LOGW(TAG, "audio: sem RAM interna para beep");
        return false;
    }

    for (int i = 0; i < kBeepSamples; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kBeepSampleRate);
        const double env = 1.0 - (static_cast<double>(i) / static_cast<double>(kBeepSamples));
        const double sample = std::sin(2.0 * kPi * static_cast<double>(kBeepHz) * t) * env;
        g_beep_buf[i] = static_cast<int16_t>(sample * 18000.0);
    }
    return true;
}

static int audio_write_beep()
{
    if (!g_spk_dev || !g_beep_buf) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    return esp_codec_dev_write(g_spk_dev,
                               g_beep_buf,
                               kBeepSamples * static_cast<int>(sizeof(int16_t)));
}

static void release_audio_tx_channel()
{
    if (!g_audio_tx_chan && !g_audio_rx_chan) {
        return;
    }

    if (g_audio_tx_chan) {
        i2s_channel_disable(g_audio_tx_chan);
        i2s_del_channel(g_audio_tx_chan);
        g_audio_tx_chan = nullptr;
    }
    if (g_audio_rx_chan) {
        i2s_channel_disable(g_audio_rx_chan);
        i2s_del_channel(g_audio_rx_chan);
        g_audio_rx_chan = nullptr;
    }
}

static bool start_audio()
{
    if (g_spk_dev) {
        return true;
    }

    ESP_LOGI(TAG, "audio: iniciando I2S TX manual + ES8311");

    if (!bsp_display_lock(200)) {
        ESP_LOGW(TAG, "audio: sem lock do display/I2C; audio desabilitado");
        return false;
    }

    esp_err_t err = bsp_i2c_init();
    bsp_display_unlock();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "audio: bsp_i2c_init falhou: %s", esp_err_to_name(err));
        return false;
    }

    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(static_cast<i2s_port_t>(CONFIG_BSP_I2S_NUM), I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    err = i2s_new_channel(&chan_cfg, &g_audio_tx_chan, &g_audio_rx_chan);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "audio: i2s_new_channel TX falhou: %s", esp_err_to_name(err));
        g_audio_tx_chan = nullptr;
        g_audio_rx_chan = nullptr;
        return false;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kBeepSampleRate),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = BSP_I2S_MCLK,
            .bclk = BSP_I2S_SCLK,
            .ws = BSP_I2S_LCLK,
            .dout = BSP_I2S_DOUT,
            .din = BSP_I2S_DSIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(g_audio_tx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "audio: i2s_channel_init_std_mode TX falhou: %s", esp_err_to_name(err));
        release_audio_tx_channel();
        return false;
    }

    err = i2s_channel_init_std_mode(g_audio_rx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "audio: i2s_channel_init_std_mode RX falhou: %s", esp_err_to_name(err));
        release_audio_tx_channel();
        return false;
    }

    err = i2s_channel_enable(g_audio_tx_chan);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "audio: i2s_channel_enable TX falhou: %s", esp_err_to_name(err));
        release_audio_tx_channel();
        return false;
    }

    err = i2s_channel_enable(g_audio_rx_chan);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "audio: i2s_channel_enable RX falhou: %s", esp_err_to_name(err));
        release_audio_tx_channel();
        return false;
    }

    const audio_codec_data_if_t* data_if = nullptr;
    const audio_codec_ctrl_if_t* i2c_ctrl_if = nullptr;
    const audio_codec_gpio_if_t* gpio_if = nullptr;
    const audio_codec_if_t* codec_if = nullptr;

    auto fail_audio_start = [&]() -> bool {
        if (g_spk_dev) {
            esp_codec_dev_delete(g_spk_dev);
            g_spk_dev = nullptr;
        }
        if (codec_if) {
            audio_codec_delete_codec_if(codec_if);
        }
        if (gpio_if) {
            audio_codec_delete_gpio_if(gpio_if);
        }
        if (i2c_ctrl_if) {
            audio_codec_delete_ctrl_if(i2c_ctrl_if);
        }
        if (data_if) {
            audio_codec_delete_data_if(data_if);
        }
        release_audio_tx_channel();
        return false;
    };

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = CONFIG_BSP_I2S_NUM,
        .rx_handle = g_audio_rx_chan,
        .tx_handle = g_audio_tx_chan,
        .clk_src = I2S_CLK_SRC_DEFAULT,
    };
    data_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (!data_if) {
        ESP_LOGW(TAG, "audio: audio_codec_new_i2s_data falhou");
        return fail_audio_start();
    }

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = bsp_i2c_get_handle(),
    };
    i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (!i2c_ctrl_if) {
        ESP_LOGW(TAG, "audio: audio_codec_new_i2c_ctrl ES8311 falhou");
        return fail_audio_start();
    }

    gpio_if = audio_codec_new_gpio();
    if (!gpio_if) {
        ESP_LOGW(TAG, "audio: audio_codec_new_gpio falhou");
        return fail_audio_start();
    }

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
        .pa_gain = 0.0,
    };
    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
        .no_dac_ref = false,
        .mclk_div = 256,
    };
    codec_if = es8311_codec_new(&es8311_cfg);
    if (!codec_if) {
        ESP_LOGW(TAG, "audio: es8311_codec_new falhou");
        return fail_audio_start();
    }

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    g_spk_dev = esp_codec_dev_new(&codec_dev_cfg);
    if (!g_spk_dev) {
        ESP_LOGW(TAG, "audio: esp_codec_dev_new falhou");
        return fail_audio_start();
    }

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .sample_rate = kBeepSampleRate,
        .mclk_multiple = 256,
    };

    int ret = esp_codec_dev_open(g_spk_dev, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGW(TAG, "audio: esp_codec_dev_open falhou: %d", ret);
        return fail_audio_start();
    }

    prepare_beep_buffer();
    const int ui_vol = 70;
    const int codec_vol = audio_codec_volume_from_ui(ui_vol);
    int vol_ret = ESP_CODEC_DEV_INVALID_ARG;
    int mute_ret = ESP_CODEC_DEV_INVALID_ARG;
    if (bsp_display_lock(200)) {
        vol_ret = esp_codec_dev_set_out_vol(g_spk_dev, codec_vol);
        mute_ret = esp_codec_dev_set_out_mute(g_spk_dev, false);
        bsp_display_unlock();
    }
    const int write_ret = audio_write_beep();
    ESP_LOGI(TAG, "audio: beep inicial ui_vol=%d codec_vol=%d vol_ret=%d mute_ret=%d write_ret=%d",
             ui_vol, codec_vol, vol_ret, mute_ret, write_ret);
    ESP_LOGI(TAG, "audio: ES8311 speaker pronto");
    return true;
}

static void audio_set_volume_and_beep(int vol_pct)
{
    if (!g_spk_dev) {
        ESP_LOGW(TAG, "audio: volume %d ignorado; speaker ainda nulo", vol_pct);
        return;
    }

    const int codec_vol = audio_codec_volume_from_ui(vol_pct);
    const bool muted = vol_pct <= 0;
    int vol_ret = ESP_CODEC_DEV_INVALID_ARG;
    int mute_ret = ESP_CODEC_DEV_INVALID_ARG;
    // set_out_vol escreve registrador I2C → precisa do display lock.
    if (bsp_display_lock(200)) {
        vol_ret = esp_codec_dev_set_out_vol(g_spk_dev, codec_vol);
        mute_ret = esp_codec_dev_set_out_mute(g_spk_dev, muted);
        bsp_display_unlock();
    } else {
        ESP_LOGW(TAG, "audio: sem lock para set_out_vol(%d)", vol_pct);
    }

    int write_ret = ESP_CODEC_DEV_INVALID_ARG;
    // write envia dados via I2S (independente do I2C) → sem lock.
    if (g_beep_buf) {
        write_ret = audio_write_beep();
    }
    ESP_LOGI(TAG, "audio: volume=%d codec_vol=%d muted=%d vol_ret=%d mute_ret=%d write_ret=%d",
             vol_pct, codec_vol, muted ? 1 : 0, vol_ret, mute_ret, write_ret);
}

static bool start_network_transport(void)
{
#if NOVAPANEL_HAS_WIFI_TRANSPORT
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    if (nvs_err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(nvs_err));
        return false;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "ESP-Hosted/SDIO transport requested for C6");
    return true;
#else
    ESP_LOGW(TAG, "network transport skipped in host build");
    return false;
#endif
}

#if NOVAPANEL_HAS_CJSON_HOOKS
// Aloca nós cJSON na SRAM interna em vez da PSRAM.
// Evita contenção de barramento entre parsing HTTP e DMA do display (DSI underrun).
// free() nativo do FreeRTOS funciona corretamente com heap_caps_malloc.
static void* json_malloc_internal(size_t n)
{
    return heap_caps_malloc(n, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
#endif

static void set_display_brightness_safe(int pct)
{
#if defined(ESP_PLATFORM)
    bsp_display_brightness_set(pct);
#else
    (void)pct;
#endif
}

extern "C" void app_main(void)
{
#if NOVAPANEL_HAS_CJSON_HOOKS
    // Redireciona alocações do cJSON para SRAM interna antes de qualquer request.
    cJSON_Hooks json_hooks{};
    json_hooks.malloc_fn = json_malloc_internal;
    json_hooks.free_fn   = free;
    cJSON_InitHooks(&json_hooks);
#endif

    ESP_LOGI(TAG, "app_main entry");
    log_heap_snapshot("app_main_entry");

    EventBus bus;
    ESP_LOGI(TAG, "boot trace: EventBus ok");
    StateStore state(bus);
    ESP_LOGI(TAG, "boot trace: StateStore ok");
    UiDispatcher ui(bus);
    ESP_LOGI(TAG, "boot trace: UiDispatcher ok");
    RequestOrchestrator requests;
    ESP_LOGI(TAG, "boot trace: RequestOrchestrator ok");
    CacheStore cache;
    ESP_LOGI(TAG, "boot trace: CacheStore ok");
    ServiceManager services;
    ESP_LOGI(TAG, "boot trace: ServiceManager ok");
    BootstrapService bootstrap(state, bus);
    ESP_LOGI(TAG, "boot trace: BootstrapService ok");
    SetupService setup(state, bus);
    ESP_LOGI(TAG, "boot trace: SetupService ok");
    SystemService system_service(state);
    ESP_LOGI(TAG, "boot trace: SystemService ok");
    NotificationService notification_service(state, bus);
    ESP_LOGI(TAG, "boot trace: NotificationService ok");
    ClockService clock_service(state);
    ESP_LOGI(TAG, "boot trace: ClockService ok");
    CoinGeckoProvider coingecko_provider;
    ESP_LOGI(TAG, "boot trace: CoinGeckoProvider ok");
    ForexProvider forex_provider;
    ESP_LOGI(TAG, "boot trace: ForexProvider ok");
    OpenMeteoProvider open_meteo_provider;
    ESP_LOGI(TAG, "boot trace: OpenMeteoProvider ok");
    MarketService market_service(state, requests, cache, coingecko_provider);
    ESP_LOGI(TAG, "boot trace: MarketService ok");
    ForexService forex_service(state, requests, cache, forex_provider);
    ESP_LOGI(TAG, "boot trace: ForexService ok");
    WeatherService weather_service(state, requests, cache, open_meteo_provider);
    ESP_LOGI(TAG, "boot trace: WeatherService ok");

    // Fase 2 (ADR-0035): uma task de rede unica dirige os refresh() por
    // prioridade, serializado e escalonado. Os services de dados nao tem mais
    // task propria; o worker chama o refresh de cada dominio "due".
    NetworkWorker network_worker(state, requests);
    ESP_LOGI(TAG, "boot trace: NetworkWorker ok");
    network_worker.register_fetcher(DataDomain::MarketSummary,
        [&market_service](uint32_t t) { return market_service.refresh(t); });
    network_worker.register_fetcher(DataDomain::Forex,
        [&forex_service](uint32_t t) { return forex_service.refresh(t); });
    network_worker.register_fetcher(DataDomain::Weather,
        [&weather_service](uint32_t t) { return weather_service.refresh(t); });
    ESP_LOGI(TAG, "boot trace: fetchers registered");

    services.add(&setup);
    services.add(&bootstrap);
    services.add(&system_service);
    services.add(&notification_service);
    services.add(&clock_service);
    services.add(&market_service);
    services.add(&forex_service);
    services.add(&weather_service);
    services.add(&network_worker);
    ESP_LOGI(TAG, "boot trace: services registered");

    QueueHandle_t action_queue = xQueueCreate(4, sizeof(std::function<void()>*));
    ESP_LOGI(TAG, "boot trace: action queue created");
    auto enqueue_action = [action_queue](std::function<void()> fn) {
        auto* copy = new std::function<void()>(std::move(fn));
        if (xQueueSend(action_queue, &copy, 0) != pdTRUE) {
            delete copy;
        }
    };

    np_bind_boot_skip([&]() {
        state.request_boot_skip();
    });
    np_bind_setup_submit([&](const OnboardingSubmission& submission) {
        enqueue_action([&state, submission]() {
            state.submit_onboarding(submission);
        });
    });
    np_bind_setup_scan([&]() {
        enqueue_action([&state]() {
            state.request_wifi_scan();
        });
    });
    np_bind_setup_step([&](OnboardingStep step) {
        state.set_onboarding_step(step);
    });
    np_bind_notifications_open([&]() {
        enqueue_action([&state]() {
            state.mark_all_notifications_read();
        });
    });
    // Navegar pela UI passa pelo StateStore (publica ScreenChanged) para o
    // app repintar a tela de destino com o estado atual imediatamente, sem
    // esperar o proximo evento periodico do dominio. Nao dispara fetch novo.
    np_bind_navigate([&](ScreenId screen) {
        state.set_screen(screen);
    });
    np_bind_ohlc_period([&](OhlcPeriod period) {
        enqueue_action([&market_service, period]() {
            market_service.request_ohlc_period(period);
        });
    });
    // Preferências: cada callback envia só o campo que mudou.
    // app_main lê as prefs atuais do StateStore, altera o campo e salva.
    np_bind_settings_time_format([&](bool fmt24h) {
        enqueue_action([&state, fmt24h]() {
            UserPreferences prefs = state.preferences();
            prefs.time_format_24h = fmt24h;
            state.set_preferences(prefs);
        });
    });
    np_bind_settings_theme([&](ThemeMode theme) {
        enqueue_action([&state, theme]() {
            UserPreferences prefs = state.preferences();
            prefs.theme = theme;
            state.set_preferences(prefs);
        });
    });
    np_bind_settings_reboot([&]() {
        enqueue_action([]() {
            esp_restart();
        });
    });
    // Preview: só PWM, chamado a cada pixel de drag — sem NVS write.
    // Sem enqueue, sem state change → zero flash na tela durante o arrasto.
    np_bind_settings_brightness_preview([](int pct) {
        set_display_brightness_safe(pct);
    });
    // Save: chamado só no RELEASED — aplica PWM final e persiste.
    np_bind_settings_brightness([&](int pct) {
        set_display_brightness_safe(pct);
        enqueue_action([&state, pct]() {
            UserPreferences prefs = state.preferences();
            prefs.brightness = pct;
            state.set_preferences(prefs);
        });
    });
    // Notificação de confirmação ao salvar qualquer setting.
    np_bind_settings_notify([&](const char* msg) {
        std::string text = msg;  // copia antes do enqueue (ptr aponta para stack do callback)
        enqueue_action([&state, text]() {
            NotificationItem item;
            item.title    = "Configurações";
            item.body     = text;
            item.level    = NotificationLevel::Info;
            item.category = NotificationCategory::System;
            state.push_notification(item);
        });
    });
    // Volume: usa display lock como mutex de I2C (serializa acesso com GT911 touch).
    // Ver start_audio() para detalhes do mecanismo e inicialização do codec.
    np_bind_settings_volume([](int pct) {
        audio_set_volume_and_beep(pct);
    });

    // Flag compartilhado entre o render callback e o loop principal (mesma
    // thread). Evita múltiplas chamadas a np_update_home() dentro da mesma
    // janela de process_pending() quando vários providers publicam em burst
    // (Market, Weather, Clock, System na mesma frame de 200ms → flicker).
    bool home_needs_update = false;
    bool market_needs_update = false;
    bool shell_needs_update = false;
    bool settings_needs_update = false;
    bool weather_needs_update = false;

    ui.bind_render([&](const UiEvent& event) {
        if (event.source == EventType::ScreenChanged) {
            const auto screen = static_cast<ScreenId>(event.i32);
            np_navigate_to(screen);
            if (screen == ScreenId::Home) {
                home_needs_update = true;  // atualiza após navegar, fora do loop de eventos
            } else if (screen == ScreenId::Market) {
                market_needs_update = true;
            } else if (screen == ScreenId::Settings) {
                settings_needs_update = true;
            } else if (screen == ScreenId::Weather) {
                weather_needs_update = true;
            }
        } else if (event.source == EventType::BootStateChanged) {
            np_update_boot(state.state().boot);
        } else if (event.source == EventType::ClockUpdated       ||
                   event.source == EventType::MarketUpdated      ||
                   event.source == EventType::ForexUpdated       ||
                   event.source == EventType::WeatherUpdated) {
            home_needs_update = true;  // coalesced: chamada única após drenar a fila
            if (event.source == EventType::MarketUpdated ||
                event.source == EventType::ForexUpdated) {
                market_needs_update = true;
            } else if (event.source == EventType::WeatherUpdated) {
                weather_needs_update = true;
            }
        } else if (event.source == EventType::OhlcUpdated) {
            market_needs_update = true;  // so chart refreshes; no home update needed
        } else if (event.source == EventType::NotificationCreated) {
            shell_needs_update = true;
            home_needs_update = true;
            settings_needs_update = true;
            weather_needs_update = true;
            // Toast apenas para notificações novas (i32 = ID atribuído > 0).
            // i32 == 0 é o evento de mark_all_read — só atualiza o badge, sem toast.
            if (event.i32 > 0) {
                const AppState snap = state.state();
                for (const NotificationItem& n : snap.notifications.items) {
                    if (n.id == static_cast<uint32_t>(event.i32)) {
                        np_show_toast(n.title.c_str(), n.body.c_str(), n.level);
                        break;
                    }
                }
            }
        } else if (event.source == EventType::SystemStatusChanged) {
            shell_needs_update = true;
            home_needs_update = true;
            settings_needs_update = true;
            weather_needs_update = true;
        } else if (event.source == EventType::OnboardingStateChanged) {
            shell_needs_update = true;
            settings_needs_update = true;
            weather_needs_update = true;
            np_update_setup(state.state());
        }
    });
    ESP_LOGI(TAG, "boot trace: UI render bound");

    ESP_LOGI(TAG, "booting rewritten NovaPanel firmware");
    log_heap_snapshot("boot");

    const bool network_transport_started = start_network_transport();
    const bool cache_ready = cache.mount();

    if (!services.init_all()) {
        ESP_LOGW(TAG, "one or more services failed to init");
    }
    services.start_all();

    start_display();
    int lvgl_lock_failures = 0;
    with_lvgl_lock("initializing UI", lvgl_lock_failures, [&]() {
        const AppState current = state.state();
        np_init();
        np_update_shell(current);
        np_update_boot(current.boot);
        np_update_setup(current);
        np_update_home(current);
    });

    // Inicia codec de áudio após o display (display lock disponível para
    // serializar I2C com o polling de touch do GT911).
    start_audio();

    {
        SystemStatus system = state.state().system;
        system.board_ready = true;
        system.display_ready = true;
        system.network_ready = network_transport_started;
        system.cache_ready = cache_ready;
        state.set_system_status(system);
    }

    const uint32_t boot_now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    services.tick_all(boot_now_ms);

    vTaskDelay(pdMS_TO_TICKS(250));
    bsp_display_backlight_on();
    // Restaura brilho salvo nas preferências (default 78 se nunca alterado).
    set_display_brightness_safe(state.preferences().brightness);
    ESP_LOGI(TAG, "backlight enabled after first UI frame");
    with_lvgl_lock("rendering initial screen", lvgl_lock_failures, [&]() {
        ui.process_pending();
    });
    log_heap_snapshot("display_ready");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(200));
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        requests.update_clock(now_ms);

        std::function<void()>* pending_action = nullptr;
        while (xQueueReceive(action_queue, &pending_action, 0) == pdTRUE) {
            (*pending_action)();
            delete pending_action;
        }
        services.tick_all(now_ms);
        with_lvgl_lock("periodic UI tick", lvgl_lock_failures, [&]() {
            np_tick();
            home_needs_update    = false;
            market_needs_update  = false;
            shell_needs_update   = false;
            settings_needs_update = false;
            weather_needs_update = false;
            ui.process_pending();
            const AppState current = state.state();
            if (shell_needs_update) {
                np_update_shell(current);
                shell_needs_update = false;
            }
            if (home_needs_update) {
                np_update_home(current);
                home_needs_update = false;
            }
            if (market_needs_update) {
                np_update_market(current, state.btc_ohlc());
                market_needs_update = false;
            }
            if (settings_needs_update) {
                np_update_settings(current);
                settings_needs_update = false;
            }
            if (weather_needs_update) {
                np_update_weather(current);
                weather_needs_update = false;
            }
        });
    }
}
