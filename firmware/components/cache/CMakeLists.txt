idf_component_register(
    SRCS
        "src/boot_breadcrumb_store.cpp"
        "src/cache_store.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        utils
        nvs_flash
        joltwallet__littlefs
)
