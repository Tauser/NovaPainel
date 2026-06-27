#include "open_meteo_provider.hpp"

#include <cerrno>
#include <cstdlib>
#include <string>
#include <cstdio>

#include "esp_log.h"
#include "http_client.hpp"

namespace nova {

namespace {
constexpr const char* kTag = "OpenMeteoProvider";

const char* find_value_start(const std::string& json, const char* key)
{
    const std::string needle = std::string("\"") + key + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return nullptr;
    }

    const std::size_t colon_pos = json.find(':', key_pos + needle.size());
    if (colon_pos == std::string::npos) {
        return nullptr;
    }

    const char* value = json.c_str() + colon_pos + 1;
    while (*value == ' ' || *value == '\n' || *value == '\r' || *value == '\t') {
        ++value;
    }
    return value;
}

bool parse_double(const std::string& json, const char* key, double& value)
{
    const char* start = find_value_start(json, key);
    if (!start) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(start, &end);
    if (end == start || errno == ERANGE) {
        return false;
    }
    value = parsed;
    return true;
}

bool parse_int(const std::string& json, const char* key, int& value)
{
    const char* start = find_value_start(json, key);
    if (!start) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(start, &end, 10);
    if (end == start || errno == ERANGE) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

WeatherCondition weather_from_code(int code)
{
    switch (code) {
        case 0: return WeatherCondition::Clear;
        case 1:
        case 2:
        case 3: return WeatherCondition::Cloudy;
        case 45:
        case 48: return WeatherCondition::Fog;
        case 51:
        case 53:
        case 55:
        case 56:
        case 57: return WeatherCondition::Drizzle;
        case 61:
        case 63:
        case 65:
        case 80:
        case 81:
        case 82: return WeatherCondition::Rain;
        case 71:
        case 73:
        case 75:
        case 77:
        case 85:
        case 86: return WeatherCondition::Snow;
        case 95:
        case 96:
        case 99: return WeatherCondition::Thunderstorm;
        default:  return WeatherCondition::Unknown;
    }
}

std::string encode_timezone(const std::string& timezone)
{
    std::string encoded;
    encoded.reserve(timezone.size() + 8);
    for (const char ch : timezone) {
        if (ch == '/') {
            encoded += "%2F";
        } else {
            encoded.push_back(ch);
        }
    }
    return encoded;
}

std::string build_url(const UserPreferences& preferences)
{
    const char* timezone = preferences.timezone.empty()
        ? "America/Sao_Paulo"
        : preferences.timezone.c_str();
    const std::string encoded_timezone = encode_timezone(timezone);

    char url[384];
    std::snprintf(url, sizeof(url),
                  "https://api.open-meteo.com/v1/forecast?"
                  "latitude=%.6f&longitude=%.6f&"
                  "current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code&"
                  "timezone=%s",
                  preferences.latitude,
                  preferences.longitude,
                  encoded_timezone.c_str());
    return url;
}

}  // namespace

bool OpenMeteoProvider::fetch(WeatherSummary& out, uint32_t now_ms,
                              const UserPreferences& preferences)
{
    std::string body;
    const std::string url = build_url(preferences);
    if (!http_get(kTag, url.c_str(), body)) {
        return false;
    }

    double temperature = 0.0;
    double feels_like = 0.0;
    double humidity = 0.0;
    double wind = 0.0;
    double uv = 0.0;
    int code = 0;
    if (!parse_double(body, "temperature_2m", temperature) ||
        !parse_double(body, "apparent_temperature", feels_like) ||
        !parse_double(body, "relative_humidity_2m", humidity) ||
        !parse_double(body, "wind_speed_10m", wind) ||
        !parse_int(body, "weather_code", code)) {
        ESP_LOGW(kTag, "JSON parse failed");
        return false;
    }

    if (!parse_double(body, "uv_index", uv)) {
        uv = 0.0;
    }

    out.temperature_c = temperature;
    out.feels_like_c = feels_like;
    out.humidity_pct = static_cast<int>(humidity);
    out.wind_speed_kmh = wind;
    out.uv_index = uv;
    out.temp_max_c = temperature;
    out.temp_min_c = temperature;
    out.condition = weather_from_code(code);
    out.valid = true;
    out.stale = false;
    out.source = DataSource::Live;
    out.last_update_ms = now_ms;

    ESP_LOGI(kTag, "temp=%.1f feels=%.1f humidity=%d wind=%.1f code=%d",
             out.temperature_c, out.feels_like_c, out.humidity_pct, out.wind_speed_kmh, code);
    return true;
}

}  // namespace nova
