cmake_minimum_required(VERSION 3.16)

# ================================================================
# NOVAPANEL — ESP-IDF Component CMakeLists
# LVGL v9.5  |  ESP32-P4
#
# Place this folder as a component:
#   <project>/components/novapanel/
# ================================================================

idf_component_register(
    SRCS
        "novapanel.c"
        "screens/screen_home.c"
        "screens/screen_agenda.c"
        "screens/screen_mercado.c"
        "screens/screen_casa.c"
        "screens/screen_alarmes.c"
        "screens/screen_clima.c"
        "screens/screen_timer.c"
        "screens/screen_config.c"
        "screens/screen_notif.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        lvgl
)
