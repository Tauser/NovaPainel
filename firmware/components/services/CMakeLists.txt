idf_component_register(
    SRCS
        "src/clock_service.cpp"
        "src/market_service.cpp"
        "src/network_worker.cpp"
        "src/setup_service.cpp"
        "src/setup_storage_logic.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        board
        cache
        core
        esp_netif
        esp_timer
        models
        nvs_flash
        providers
)
