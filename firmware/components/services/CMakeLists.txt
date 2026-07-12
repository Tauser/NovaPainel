idf_component_register(
    SRCS
        "src/clock_service.cpp"
        "src/setup_service.cpp"
        "src/setup_storage_logic.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        board
        core
        models
        nvs_flash
)
