idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES core models cache services ui
    PRIV_REQUIRES esp_timer heap esp_wifi esp_event esp_netif nvs_flash json esp_driver_i2s espressif__esp_codec_dev
)
