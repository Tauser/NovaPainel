#include "cache_store.hpp"

#include <cstdio>
#include <cstring>
#include <type_traits>

#if defined(ESP_PLATFORM)

#include "esp_err.h"
#include "esp_log.h"
#include "esp_littlefs.h"

namespace nova {

namespace {
constexpr const char* kTag = "CacheStore";
constexpr const char* kMountPath = "/littlefs";
constexpr const char* kPartitionLabel = "storage";
constexpr uint32_t kMagic = 0x4E50434Cu;  // NPCL
constexpr uint16_t kVersion = 1;

struct MarketBlob {
    double    btc_usd;
    double    btc_change_24h;
    bool      valid;
    bool      stale;
    DataSource source;
    uint32_t  last_update_ms;
};

struct ForexBlob {
    double    usd_brl;
    bool      usd_brl_valid;
    bool      usd_brl_stale;
    DataSource usd_brl_source;
    uint32_t  usd_brl_last_update_ms;
};

struct WeatherBlob {
    double           temperature_c;
    double           feels_like_c;
    double           temp_max_c;
    double           temp_min_c;
    double           uv_index;
    int              humidity_pct;
    double           wind_speed_kmh;
    WeatherCondition condition;
    bool             valid;
    bool             stale;
    DataSource       source;
    uint32_t         last_update_ms;
};

template <typename T>
struct PayloadTraits;

template <>
struct PayloadTraits<MarketBlob> {
    static constexpr const char* path = "/littlefs/market.bin";
    static constexpr const char* tmp_path = "/littlefs/market.tmp";
};

template <>
struct PayloadTraits<ForexBlob> {
    static constexpr const char* path = "/littlefs/forex.bin";
    static constexpr const char* tmp_path = "/littlefs/forex.tmp";
};

template <>
struct PayloadTraits<WeatherBlob> {
    static constexpr const char* path = "/littlefs/weather.bin";
    static constexpr const char* tmp_path = "/littlefs/weather.tmp";
};

template <typename T>
bool read_file(const char* path, T& out)
{
    std::FILE* f = std::fopen(path, "rb");
    if (!f) {
        return false;
    }

    CacheStore::Header header{};
    if (std::fread(&header, sizeof(header), 1, f) != 1) {
        std::fclose(f);
        return false;
    }

    if (header.magic != kMagic || header.version != kVersion || header.size != sizeof(T)) {
        std::fclose(f);
        return false;
    }

    if (std::fread(&out, sizeof(out), 1, f) != 1) {
        std::fclose(f);
        return false;
    }

    std::fclose(f);
    return true;
}

template <typename T>
bool write_file(const char* path, const char* tmp_path, const T& value)
{
    std::FILE* f = std::fopen(tmp_path, "wb");
    if (!f) {
        return false;
    }

    CacheStore::Header header{ kMagic, kVersion, 0, sizeof(T) };
    const bool ok = std::fwrite(&header, sizeof(header), 1, f) == 1 &&
                    std::fwrite(&value, sizeof(value), 1, f) == 1 &&
                    std::fflush(f) == 0;
    std::fclose(f);
    if (!ok) {
        std::remove(tmp_path);
        return false;
    }

    std::remove(path);
    if (std::rename(tmp_path, path) != 0) {
        std::remove(tmp_path);
        return false;
    }
    return true;
}

}  // namespace

bool CacheStore::mount()
{
    esp_vfs_littlefs_conf_t conf{};
    conf.base_path = kMountPath;
    conf.partition_label = kPartitionLabel;
    conf.format_if_mount_failed = true;
    conf.dont_mount = false;

    const esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "mount failed: %s", esp_err_to_name(err));
        ready_ = false;
        return false;
    }

    size_t total = 0;
    size_t used = 0;
    const esp_err_t info_err = esp_littlefs_info(kPartitionLabel, &total, &used);
    if (info_err != ESP_OK) {
        ESP_LOGW(kTag, "info failed: %s", esp_err_to_name(info_err));
    } else {
        ESP_LOGI(kTag, "mounted %s on %s total=%u used=%u",
                 kPartitionLabel, kMountPath, static_cast<unsigned>(total), static_cast<unsigned>(used));
    }

    ready_ = true;
    ESP_LOGI(kTag, "littlefs ready at %s", kMountPath);
    return true;
}

bool CacheStore::load_market(MarketSummary& out) const
{
    MarketBlob blob{};
    if (!load_blob(PayloadTraits<MarketBlob>::path, blob)) {
        return false;
    }

    out.btc_usd = blob.btc_usd;
    out.btc_change_24h = blob.btc_change_24h;
    out.valid = blob.valid;
    out.stale = blob.stale;
    out.source = blob.source;
    out.last_update_ms = blob.last_update_ms;
    return true;
}

bool CacheStore::save_market(const MarketSummary& market) const
{
    MarketBlob blob{
        market.btc_usd,
        market.btc_change_24h,
        market.valid,
        market.stale,
        market.source,
        market.last_update_ms,
    };
    return save_blob(PayloadTraits<MarketBlob>::path, blob);
}

bool CacheStore::load_forex(MarketSummary& out) const
{
    ForexBlob blob{};
    if (!load_blob(PayloadTraits<ForexBlob>::path, blob)) {
        return false;
    }

    out.usd_brl = blob.usd_brl;
    out.usd_brl_valid = blob.usd_brl_valid;
    out.usd_brl_stale = blob.usd_brl_stale;
    out.usd_brl_source = blob.usd_brl_source;
    out.usd_brl_last_update_ms = blob.usd_brl_last_update_ms;
    return true;
}

bool CacheStore::save_forex(const MarketSummary& market) const
{
    ForexBlob blob{
        market.usd_brl,
        market.usd_brl_valid,
        market.usd_brl_stale,
        market.usd_brl_source,
        market.usd_brl_last_update_ms,
    };
    return save_blob(PayloadTraits<ForexBlob>::path, blob);
}

bool CacheStore::load_weather(WeatherSummary& out) const
{
    WeatherBlob blob{};
    if (!load_blob(PayloadTraits<WeatherBlob>::path, blob)) {
        return false;
    }

    out.temperature_c = blob.temperature_c;
    out.feels_like_c = blob.feels_like_c;
    out.temp_max_c = blob.temp_max_c;
    out.temp_min_c = blob.temp_min_c;
    out.uv_index = blob.uv_index;
    out.humidity_pct = blob.humidity_pct;
    out.wind_speed_kmh = blob.wind_speed_kmh;
    out.condition = blob.condition;
    out.valid = blob.valid;
    out.stale = blob.stale;
    out.source = blob.source;
    out.last_update_ms = blob.last_update_ms;
    return true;
}

bool CacheStore::save_weather(const WeatherSummary& weather) const
{
    WeatherBlob blob{
        weather.temperature_c,
        weather.feels_like_c,
        weather.temp_max_c,
        weather.temp_min_c,
        weather.uv_index,
        weather.humidity_pct,
        weather.wind_speed_kmh,
        weather.condition,
        weather.valid,
        weather.stale,
        weather.source,
        weather.last_update_ms,
    };
    return save_blob(PayloadTraits<WeatherBlob>::path, blob);
}

template <typename T>
bool CacheStore::load_blob(const char* path, T& out) const
{
    static_assert(std::is_trivially_copyable_v<T>, "cache payload must be trivially copyable");
    if (!ready_) {
        return false;
    }
    return read_file(path, out);
}

template <typename T>
bool CacheStore::save_blob(const char* path, const T& value) const
{
    static_assert(std::is_trivially_copyable_v<T>, "cache payload must be trivially copyable");
    if (!ready_) {
        return false;
    }
    return write_file(path, PayloadTraits<T>::tmp_path, value);
}

}  // namespace nova

#else

namespace nova {

bool CacheStore::mount() { return false; }
bool CacheStore::load_market(MarketSummary&) const { return false; }
bool CacheStore::save_market(const MarketSummary&) const { return false; }
bool CacheStore::load_forex(MarketSummary&) const { return false; }
bool CacheStore::save_forex(const MarketSummary&) const { return false; }
bool CacheStore::load_weather(WeatherSummary&) const { return false; }
bool CacheStore::save_weather(const WeatherSummary&) const { return false; }

template <typename T>
bool CacheStore::load_blob(const char*, T&) const { return false; }

template <typename T>
bool CacheStore::save_blob(const char*, const T&) const { return false; }

}  // namespace nova

#endif
