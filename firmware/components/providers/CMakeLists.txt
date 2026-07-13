idf_component_register(
    SRCS
        "src/awesomeapi_provider.cpp"
        "src/coingecko_provider.cpp"
        "src/provider_interfaces.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        models
        utils
)
