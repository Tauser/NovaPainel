// NovaPainel - board/mock_board.cpp
#include "mock_board.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "MockBoard";
}

BoardStatus MockBoard::bring_up() {
    ESP_LOGI(kTag, "bring-up: simulating board power-on");
    ESP_LOGI(kTag, "  - display : MOCK (no real BSP yet)");
    ESP_LOGI(kTag, "  - touch   : MOCK (no real driver yet)");
    ESP_LOGI(kTag, "  - network : MOCK (real board uses ESP32-C6 / ESP-Hosted)");
    ESP_LOGI(kTag, "  - sd card : MOCK (no real SDMMC yet)");
    status_ = BoardStatus{
        /*board_ready=*/true,
        /*display_ready=*/true,
        /*touch_ready=*/true,
        /*network_ready=*/false,  // network is a Fase-0 risk; mock leaves it down
        /*sd_ready=*/true,
    };
    ESP_LOGI(kTag, "bring-up complete (network intentionally not ready in mock)");
    return status_;
}

}  // namespace nova
