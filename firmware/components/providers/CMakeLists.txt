idf_component_register(
    SRCS
        "src/coingecko_provider.cpp"
        "src/forex_provider.cpp"
        "src/open_meteo_provider.cpp"
    INCLUDE_DIRS "include"
    REQUIRES models utils
)
