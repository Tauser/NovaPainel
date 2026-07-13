#include "open_meteo_provider.hpp"

#include "http_client.hpp"
#include "json_value.hpp"

#include <cstdio>

namespace nova {
namespace {

// Só o subconjunto que WeatherState guarda hoje (temperature_c,
// precipitation_mm, summary) -- payload minúsculo por design, bem abaixo do
// cap de 48 KB. Nada de hourly/daily: isso é Fase 5 (tela Weather real).
const char* summary_for_code(int code) {
    switch (code) {
        case 0: return "Céu limpo";
        case 1:
        case 2:
        case 3: return "Nublado";
        case 45:
        case 48: return "Neblina";
        case 51:
        case 53:
        case 55:
        case 56:
        case 57: return "Garoa";
        case 61:
        case 63:
        case 65:
        case 80:
        case 81:
        case 82: return "Chuva";
        case 71:
        case 73:
        case 75:
        case 77:
        case 85:
        case 86: return "Neve";
        case 95:
        case 96:
        case 99: return "Trovoada";
        default: return "Indefinido";
    }
}

}  // namespace

std::string OpenMeteoProvider::build_url(double latitude, double longitude) {
    char url[192];
    std::snprintf(url, sizeof(url),
                  "https://api.open-meteo.com/v1/forecast?"
                  "latitude=%.4f&longitude=%.4f&"
                  "current=temperature_2m,precipitation,weather_code&timezone=auto",
                  latitude, longitude);
    return url;
}

Result<WeatherState> OpenMeteoProvider::parse_weather_payload(const std::string& body) {
    const auto parsed = parse_json(body);
    if (!parsed.ok()) {
        return Result<WeatherState>::failure(parsed.status());
    }
    if (!parsed.value().is_object()) {
        return Result<WeatherState>::failure(
            Status::error(StatusCode::InvalidArgument, "open-meteo payload is not an object"));
    }
    const JsonValue* current = parsed.value().find("current");
    if (current == nullptr || !current->is_object()) {
        return Result<WeatherState>::failure(
            Status::error(StatusCode::InvalidArgument, "open-meteo payload missing current object"));
    }

    const JsonValue* temperature = current->find("temperature_2m");
    const JsonValue* precipitation = current->find("precipitation");
    const JsonValue* code = current->find("weather_code");
    if (temperature == nullptr || !temperature->is_number() ||
        precipitation == nullptr || !precipitation->is_number() ||
        code == nullptr || !code->is_number()) {
        return Result<WeatherState>::failure(
            Status::error(StatusCode::InvalidArgument, "open-meteo payload missing current fields"));
    }

    WeatherState state{};
    state.temperature_c = temperature->as_number();
    state.precipitation_mm = precipitation->as_number();
    state.summary = summary_for_code(static_cast<int>(code->as_number()));
    state.valid = true;
    return Result<WeatherState>::success(state);
}

Result<WeatherState> OpenMeteoProvider::fetch_weather(double latitude, double longitude) {
    HttpClient http_client;
    const auto response = http_client.get(build_url(latitude, longitude));
    if (!response.ok()) {
        return Result<WeatherState>::failure(response.status());
    }
    return parse_weather_payload(response.value().body);
}

}  // namespace nova
