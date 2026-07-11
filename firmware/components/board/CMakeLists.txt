idf_component_register(
    SRCS
        "src/mock_board.cpp"
        "src/waveshare_board.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        utils
        esp_lcd
        esp_timer
        waveshare__esp32_p4_wifi6_touch_lcd_7b
)
