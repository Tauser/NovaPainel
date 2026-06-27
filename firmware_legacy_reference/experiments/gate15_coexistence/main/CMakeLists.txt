idf_component_register(
    SRCS "gate15_main.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash          # reboot counter persisted across resets
        esp_psram          # framebuffer in PSRAM
        esp_timer          # uptime
        esp_event          # wifi event loop
        esp_netif          # wifi
        esp_wifi           # standard Wi-Fi API (served by esp_wifi_remote on P4)
        esp_http_client    # HTTPS GET
        esp-tls            # TLS
        mbedtls            # esp_crt_bundle_attach (Gate 12: TLS server verification)
        lwip               # SNTP (Gate 11)
        fatfs              # SD card filesystem (Gate 7)
        sdmmc               # SD card SDMMC driver (Gate 7)
        esp_driver_sdmmc    # SD card host (Gate 7)
        esp_hw_support      # esp_ldo_regulator (SD slot power, Gate 7)
        esp_lcd             # esp_lcd_panel_draw_bitmap/disp_on_off (Gate 5)
        waveshare__esp32_p4_wifi6_touch_lcd_7b  # official board BSP (Gates 5/6)
        espressif__esp_lcd_touch        # esp_lcd_touch_read_data/get_coordinates (Gate 6)
        espressif__esp_lcd_touch_gt911  # GT911 driver pulled by the BSP
)
