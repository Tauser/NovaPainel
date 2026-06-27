idf_component_register(
    SRCS
        "src/bootstrap_service.cpp"
        "src/setup_service.cpp"
        "src/system_service.cpp"
        "src/clock_service.cpp"
        "src/market_service.cpp"
        "src/forex_service.cpp"
        "src/weather_service.cpp"
    INCLUDE_DIRS "include"
    REQUIRES core models cache providers esp_system esp_timer nvs_flash esp_wifi esp_event esp_netif
)
