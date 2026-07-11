idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        board
        core
        models
        providers
        ui
        utils
)
