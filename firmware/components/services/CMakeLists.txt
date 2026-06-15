idf_component_register(
    SRCS
        "src/clock_service.cpp"
        "src/market_service.cpp"
        "src/notification_service.cpp"
    INCLUDE_DIRS "include"
    REQUIRES core models providers utils
)
