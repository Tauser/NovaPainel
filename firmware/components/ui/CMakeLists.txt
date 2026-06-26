file(GLOB NOVA_FONT_SRCS "fonts/*.c")

idf_component_register(
    SRCS
        "src/screens/home_screen.cpp"
        "src/screens/boot_screen.cpp"
        "src/screens/wizard_screen.cpp"
        "src/screens/system_screen.cpp"
        "src/screens/settings_screen.cpp"
        "src/common/nova_keyboard_manager.cpp"
        "src/main_shell.cpp"
        ${NOVA_FONT_SRCS}

    INCLUDE_DIRS
        "include"

    REQUIRES
        models
        utils
        lvgl__lvgl
)
