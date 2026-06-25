idf_component_register(
    SRCS "src/mock_market_provider.cpp" "src/coingecko_provider.cpp" "src/open_meteo_provider.cpp" "src/forex_provider.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        models utils
        esp_http_client   # CoinGeckoProvider: HTTPS GET
        mbedtls           # CoinGeckoProvider: esp_crt_bundle_attach (cert validation)
        json              # CoinGeckoProvider: cJSON
)
