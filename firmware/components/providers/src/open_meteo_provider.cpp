#include "open_meteo_provider.hpp"

#include <cerrno>
#include <cstdlib>
#include <utility>
#include <string>

#include "esp_err.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "OpenMeteoProvider";
constexpr const char* kUrl =
    "https://api.open-meteo.com/v1/forecast?latitude=-15.793889&longitude=-47.882778&"
    "current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code&"
    "timezone=America%2FSao_Paulo";

struct HttpBody {
    std::string data;
};

esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    auto* body = static_cast<HttpBody*>(evt->user_data);
    if (!body) {
        return ESP_OK;
    }

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            body->data.append(static_cast<const char*>(evt->data), evt->data_len);
            break;
        default:
            break;
    }
    return ESP_OK;
}

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

bool fetch_http(const char* url, std::string& body)
{
    HttpBody payload;
    esp_http_client_config_t config{};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;
    config.buffer_size = 1024;
    config.event_handler = http_event_handler;
    config.user_data = &payload;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGW(kTag, "esp_http_client_init failed");
        return false;
    }

    const esp_err_t open_err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (open_err != ESP_OK || status != 200 || payload.data.empty()) {
        ESP_LOGW(kTag, "HTTP failed: err=%s status=%d",
                 esp_err_to_name(open_err), status);
        return false;
    }

    body = std::move(payload.data);
    return true;
}

}  // namespace

bool OpenMeteoProvider::fetch(WeatherSummary& out, uint32_t now_ms)
{
    std::string body;
    if (!fetch_http(kUrl, body)) {
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
