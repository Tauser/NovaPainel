#include "waveshare_board.hpp"

#include "bsp/esp32_p4_wifi6_touch_lcd_7b.h"
#include "esp_log.h"

namespace nova {
namespace {
constexpr const char* kTag = "WaveshareBoard";
}

bool WaveshareBoard::init_display() {
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * (BSP_LCD_V_RES / 8),
        .double_buffer = false,
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
    bsp_display_backlight_on();
    status_.display_ready = true;
    status_.touch_ready = true;
    return true;
}

BoardStatus WaveshareBoard::bring_up() {
    status_ = BoardStatus{};
    init_display();
    status_.board_ready = status_.display_ready;
    return status_;
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

}  // namespace nova
