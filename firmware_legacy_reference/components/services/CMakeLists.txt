idf_component_register(
    SRCS
        "src/clock_service.cpp"
        "src/market_service.cpp"
        "src/weather_service.cpp"
        "src/forex_service.cpp"
        "src/system_service.cpp"
        "src/notification_service.cpp"
        "src/setup_service.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        core models providers utils cache
        nvs_flash   # SetupService/SystemService: persists preferences/Wi-Fi creds, reboot counter
        esp_wifi    # SetupService: esp_wifi_set_config/esp_wifi_connect (Wifi step)
        esp_event   # SetupService: WIFI_EVENT/IP_EVENT handlers
        esp_netif   # SetupService: esp_netif_sntp_* (ADR-0021)
        esp_timer   # SetupService: connect timeout / delayed reconnect
)
