file(GLOB NOVA_FONT_SRCS "fonts/*.c")

idf_component_register(
    SRCS "src/screens/home_screen.cpp" "src/screens/boot_screen.cpp" "src/screens/wizard_screen.cpp" "src/main_shell.cpp" ${NOVA_FONT_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES models utils lvgl__lvgl
)
