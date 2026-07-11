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

const cJSON* read_array(const cJSON* object, const char* key)
{
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(object), key);
    return cJSON_IsArray(item) ? item : nullptr;
}

bool parse_hour_from_iso8601(const char* text, int& hour)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int parsed_hour = 0;
    if (!text || std::sscanf(text, "%d-%d-%dT%d", &year, &month, &day, &parsed_hour) != 4) {
        return false;
    }
    hour = parsed_hour;
    return true;
}

bool parse_minutes_from_iso8601(const char* text, int& minutes)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    if (!text || std::sscanf(text, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) != 5) {
        return false;
    }
    minutes = (hour * 60) + minute;
    return true;
}

int weekday_from_ymd(int year, int month, int day)
{
    if (month < 3) {
        month += 12;
        --year;
    }
    const int k = year % 100;
    const int j = year / 100;
    const int h = (day + ((13 * (month + 1)) / 5) + k + (k / 4) + (j / 4) + (5 * j)) % 7;
    return (h + 6) % 7;
}

bool parse_weekday_from_iso8601_date(const char* text, int& weekday)
{
    int year = 0;
    int month = 0;
    int day = 0;
    if (!text || std::sscanf(text, "%d-%d-%d", &year, &month, &day) != 3) {
        return false;
    }
    weekday = weekday_from_ymd(year, month, day);
    return true;
}

void reset_forecasts(WeatherSummary& out)
{
    for (std::size_t i = 0; i < kWeatherHourlySlots; ++i) {
        out.hourly[i] = WeatherHourlyPoint{};
    }
    for (std::size_t i = 0; i < kWeatherDailySlots; ++i) {
        out.daily[i] = WeatherDailyPoint{};
    }
    out.sunrise_minutes = -1;
    out.sunset_minutes = -1;
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
    reset_forecasts(out);
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

    const cJSON* hourly = cJSON_GetObjectItemCaseSensitive(root, "hourly");
    if (cJSON_IsObject(hourly)) {
        const cJSON* hourly_times = read_array(hourly, "time");
        const cJSON* hourly_temps = read_array(hourly, "temperature_2m");
        const cJSON* hourly_precip = read_array(hourly, "precipitation_probability");
        const cJSON* hourly_codes = read_array(hourly, "weather_code");
        if (hourly_times && hourly_temps && hourly_precip && hourly_codes) {
            for (std::size_t i = 0; i < kWeatherHourlySlots; ++i) {
                const cJSON* time_item = cJSON_GetArrayItem(const_cast<cJSON*>(hourly_times), static_cast<int>(i));
                const cJSON* temp_item = cJSON_GetArrayItem(const_cast<cJSON*>(hourly_temps), static_cast<int>(i));
                const cJSON* precip_item = cJSON_GetArrayItem(const_cast<cJSON*>(hourly_precip), static_cast<int>(i));
                const cJSON* code_item = cJSON_GetArrayItem(const_cast<cJSON*>(hourly_codes), static_cast<int>(i));
                if (!cJSON_IsString(time_item) || !cJSON_IsNumber(temp_item) ||
                    !cJSON_IsNumber(precip_item) || !cJSON_IsNumber(code_item)) {
                    break;
                }

                int hour = 0;
                if (!parse_hour_from_iso8601(time_item->valuestring, hour)) {
                    break;
                }

                out.hourly[i].hour = hour;
                out.hourly[i].temperature_c = temp_item->valuedouble;
                out.hourly[i].precip_pct = precip_item->valueint;
                out.hourly[i].condition = weather_from_code(code_item->valueint);
                out.hourly[i].valid = true;
            }
        }
    }

    const cJSON* daily_times = read_array(daily, "time");
    const cJSON* daily_codes = read_array(daily, "weather_code");
    const cJSON* daily_precip = read_array(daily, "precipitation_probability_max");
    const cJSON* daily_sunrise = read_array(daily, "sunrise");
    const cJSON* daily_sunset = read_array(daily, "sunset");
    if (daily_times && daily_codes && daily_precip && daily_sunrise && daily_sunset) {
        const cJSON* sunrise_item = cJSON_GetArrayItem(const_cast<cJSON*>(daily_sunrise), 0);
        const cJSON* sunset_item = cJSON_GetArrayItem(const_cast<cJSON*>(daily_sunset), 0);
        if (cJSON_IsString(sunrise_item)) {
            parse_minutes_from_iso8601(sunrise_item->valuestring, out.sunrise_minutes);
        }
        if (cJSON_IsString(sunset_item)) {
            parse_minutes_from_iso8601(sunset_item->valuestring, out.sunset_minutes);
        }

        for (std::size_t i = 0; i < kWeatherDailySlots; ++i) {
            const cJSON* time_item = cJSON_GetArrayItem(const_cast<cJSON*>(daily_times), static_cast<int>(i));
            const cJSON* min_item = cJSON_GetArrayItem(const_cast<cJSON*>(cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(daily), "temperature_2m_min")), static_cast<int>(i));
            const cJSON* max_item = cJSON_GetArrayItem(const_cast<cJSON*>(cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(daily), "temperature_2m_max")), static_cast<int>(i));
            const cJSON* precip_item = cJSON_GetArrayItem(const_cast<cJSON*>(daily_precip), static_cast<int>(i));
            const cJSON* code_item = cJSON_GetArrayItem(const_cast<cJSON*>(daily_codes), static_cast<int>(i));
            if (!cJSON_IsString(time_item) || !cJSON_IsNumber(min_item) || !cJSON_IsNumber(max_item) ||
                !cJSON_IsNumber(precip_item) || !cJSON_IsNumber(code_item)) {
                break;
            }

            int weekday = 0;
            if (!parse_weekday_from_iso8601_date(time_item->valuestring, weekday)) {
                break;
            }

            out.daily[i].weekday = weekday;
            out.daily[i].temp_min_c = min_item->valuedouble;
            out.daily[i].temp_max_c = max_item->valuedouble;
            out.daily[i].precip_pct = precip_item->valueint;
            out.daily[i].condition = weather_from_code(code_item->valueint);
            out.daily[i].valid = true;
        }
    }

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

    char url[512];
    std::snprintf(url, sizeof(url),
                  "https://api.open-meteo.com/v1/forecast?"
                  "latitude=%.6f&longitude=%.6f&"
                  "current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code,uv_index&"
                  // 'time' NAO e variavel valida em hourly/daily: o Open-Meteo
                  // ja retorna o array 'time' automaticamente. Inclui-lo causa
                  // HTTP 400 (Invalid parameter). O parser le 'time' do JSON
                  // mesmo sem pedir.
                  "hourly=temperature_2m,precipitation_probability,weather_code&"
                  "daily=temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max,sunrise,sunset&"
                  "forecast_hours=8&forecast_days=5&"
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
