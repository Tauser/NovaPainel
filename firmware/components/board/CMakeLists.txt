idf_component_register(
    SRCS "src/mock_board.cpp" "src/waveshare_board.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        utils
        nvs_flash       # WaveshareBoard: Wi-Fi needs NVS
        esp_netif       # WaveshareBoard: network transport bring-up
        esp_event       # WaveshareBoard: default event loop
        esp_wifi        # WaveshareBoard: served by esp_wifi_remote on P4
        esp_lcd         # WaveshareBoard: esp_lcd_panel_disp_on_off
        esp_hw_support  # WaveshareBoard: esp_ldo_regulator (SD slot power)
        driver          # WaveshareBoard: gpio_num_t
        fatfs           # WaveshareBoard: SD card filesystem (Gate 7)
        sdmmc           # WaveshareBoard: SD card SDMMC driver (Gate 7)
        esp_driver_sdmmc # WaveshareBoard: SD card host (Gate 7)
        waveshare__esp32_p4_wifi6_touch_lcd_7b    # official BSP (Fase 0, Gates 5/6)
        espressif__esp_lcd_touch                  # GT911 driver API
        espressif__esp_lcd_touch_gt911            # GT911 driver
)
