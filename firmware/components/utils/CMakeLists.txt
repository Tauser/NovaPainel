idf_component_register(
    SRCS
        "src/boot_recovery_policy.cpp"
        "src/http_client.cpp"
        "src/json_value.cpp"
        "src/status.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        models
        esp_http_client
        mbedtls
)
