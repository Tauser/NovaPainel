#pragma once

#include <cstdint>

#include "app_state.hpp"

namespace nova {

class CacheStore final {
public:
    struct Header {
        uint32_t magic;
        uint16_t version;
        uint16_t reserved;
        uint32_t size;
    };

    bool mount();
    bool ready() const { return ready_; }

    bool load_crypto(CryptoSummary& out) const;
    bool save_crypto(const CryptoSummary& market) const;

    bool load_forex(ForexSummary& out) const;
    bool save_forex(const ForexSummary& market) const;

    bool load_weather(WeatherSummary& out) const;
    bool save_weather(const WeatherSummary& weather) const;

private:
    template <typename T>
    bool load_blob(const char* path, T& out) const;

    template <typename T>
    bool save_blob(const char* path, const T& value) const;

    bool ready_{false};
};

}  // namespace nova
