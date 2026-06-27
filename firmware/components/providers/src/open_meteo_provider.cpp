#include "open_meteo_provider.hpp"

#include <string>
#include <cstdio>

#include <cJSON.h>

#include "esp_log.h"
#include "http_client.hpp"

namespace nova {

namespace {
constexpr const char* kTag = "OpenMeteoProvider";

WeatherCondition weather_from_code(int code);

bool read_object_double(const cJSON* object, const char* key, double& value)
{
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(object), key);
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    value = item->valuedouble;
    return true;
}

bool read_object_int(const cJSON* object, const char* key, int& value)
{
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(object), key);
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    value = item->valueint;
    return true;
}

bool read_first_array_double(const cJSON* object, const char* key, double& value)
{
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(object), key);
    if (!cJSON_IsArray(item)) {
        return false;
    }

    const cJSON* first = cJSON_GetArrayItem(const_cast<cJSON*>(item), 0);
    if (!cJSON_IsNumber(first)) {
        return false;
    }
    value = first->valuedouble;
    return true;
}

bool parse_weather_payload(const std::string& body, WeatherSummary& out)
{
    cJSON* root = cJSON_ParseWithLength(body.c_str(), body.size());
    if (!root) {
        const char* parse_error = cJSON_GetErrorPtr();
        if (parse_error) {
            ESP_LOGW(kTag, "JSON parse failed near: %.120s", parse_error);
        } else {
            ESP_LOGW(kTag, "JSON parse failed");
        }
        return false;
    }

    const cJSON* current = cJSON_GetObjectItemCaseSensitive(root, "current");
    const cJSON* daily = cJSON_GetObjectItemCaseSensitive(root, "daily");
    if (!cJSON_IsObject(current) || !cJSON_IsObject(daily)) {
        ESP_LOGW(kTag, "JSON payload missing current/daily objects");
        cJSON_Delete(root);
        return false;
    }

    double temperature = 0.0;
    double feels_like = 0.0;
    double humidity = 0.0;
    double wind = 0.0;
    double uv = 0.0;
    double temp_max = 0.0;
    double temp_min = 0.0;
    int code = 0;
    if (!read_object_double(current, "temperature_2m", temperature) ||
        !read_object_double(current, "apparent_temperature", feels_like) ||
        !read_object_double(current, "relative_humidity_2m", humidity) ||
        !read_object_double(current, "wind_speed_10m", wind) ||
        !read_object_double(current, "uv_index", uv) ||
        !read_first_array_double(daily, "temperature_2m_max", temp_max) ||
        !read_first_array_double(daily, "temperature_2m_min", temp_min) ||
        !read_object_int(current, "weather_code", code)) {
        ESP_LOGW(kTag, "JSON payload missing required weather fields");
        cJSON_Delete(root);
        return false;
    }

    out.temperature_c = temperature;
    out.feels_like_c = feels_like;
    out.humidity_pct = static_cast<int>(humidity);
    out.wind_speed_kmh = wind;
    out.uv_index = uv;
    out.temp_max_c = temp_max;
    out.temp_min_c = temp_min;
    out.condition = weather_from_code(code);
    out.valid = true;
    out.stale = false;
    out.source = DataSource::Live;

    cJSON_Delete(root);
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
                  "current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code,uv_index&"
                  "daily=temperature_2m_max,temperature_2m_min&"
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

    if (!parse_weather_payload(body, out)) {
        return false;
    }

    out.last_update_ms = now_ms;

    ESP_LOGI(kTag, "temp=%.1f min=%.1f max=%.1f humidity=%d wind=%.1f code=%d",
             out.temperature_c, out.temp_min_c, out.temp_max_c,
             out.humidity_pct, out.wind_speed_kmh,
             static_cast<int>(out.condition));
    return true;
}

}  // namespace nova
