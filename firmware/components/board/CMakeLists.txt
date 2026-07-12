idf_component_register(
    SRCS
        "src/mock_board.cpp"
        "src/waveshare_board.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        utils
        models
        esp_lcd
        esp_timer
        esp_event
        esp_netif
        espressif__esp_hosted
        esp_wifi
        nvs_flash
        waveshare__esp32_p4_wifi6_touch_lcd_7b
)
