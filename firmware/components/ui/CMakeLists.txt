idf_component_register(
    SRCS
        "src/common/nova_keyboard_manager.cpp"
        "src/fonts/fontawesome_16.c"
        "src/fonts/fontawesome_20.c"
        "src/fonts/nova_font_64.c"
        "src/fonts/lv_font_montserrat_extrabold_94.c"
        "src/novapanel_ui.cpp"
        "src/screens/screen_boot.cpp"
        "src/screens/screen_agenda.cpp"
        "src/screens/screen_home.cpp"
        "src/screens/screen_market.cpp"
        "src/screens/screen_settings.cpp"
        "src/screens/screen_weather.cpp"
        "src/screens/screen_placeholders.cpp"
        "src/screens/screen_setup.cpp"
    INCLUDE_DIRS "include"
    REQUIRES models lvgl
)
