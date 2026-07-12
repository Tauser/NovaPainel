idf_component_register(
    SRCS "src/boot_breadcrumb_store.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        utils
        nvs_flash
)
