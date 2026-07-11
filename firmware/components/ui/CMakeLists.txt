idf_component_register(
    SRCS
        "src/boot_view_model.cpp"
        "src/builtin_screens.cpp"
        "src/home_view_model.cpp"
        "src/screen_registry.cpp"
        "src/ui_shell.cpp"
        "src/screens/boot_screen.cpp"
        "src/screens/home_screen.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        core
        lvgl
        models
)
