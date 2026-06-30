#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>

namespace nova {

namespace {

static lv_obj_t* g_weather_city = nullptr;
static lv_obj_t* g_weather_condition_icon = nullptr;
static lv_obj_t* g_weather_temp = nullptr;
static lv_obj_t* g_weather_desc = nullptr;
static lv_obj_t* g_weather_range_max = nullptr;
static lv_obj_t* g_weather_range_min = nullptr;
static lv_obj_t* g_weather_feels = nullptr;
static lv_obj_t* g_weather_sunrise = nullptr;
static lv_obj_t* g_weather_sunset = nullptr;
static lv_obj_t* g_weather_hourly_icon[kWeatherHourlySlots] = { nullptr };
static lv_obj_t* g_weather_hourly_temp[kWeatherHourlySlots] = { nullptr };
static lv_obj_t* g_weather_hourly_hour[kWeatherHourlySlots] = { nullptr };
static lv_obj_t* g_weather_hourly_precip[kWeatherHourlySlots] = { nullptr };
static lv_obj_t* g_weather_day_name[kWeatherDailySlots] = { nullptr };
static lv_obj_t* g_weather_day_icon[kWeatherDailySlots] = { nullptr };
static lv_obj_t* g_weather_day_precip[kWeatherDailySlots] = { nullptr };
static lv_obj_t* g_weather_day_min[kWeatherDailySlots] = { nullptr };
static lv_obj_t* g_weather_day_max[kWeatherDailySlots] = { nullptr };
static lv_obj_t* g_weather_detail_wind = nullptr;
static lv_obj_t* g_weather_detail_humidity = nullptr;
static lv_obj_t* g_weather_detail_pressure = nullptr;
static lv_obj_t* g_weather_detail_dew = nullptr;
static lv_obj_t* g_weather_detail_visibility = nullptr;
static lv_obj_t* g_weather_detail_clouds = nullptr;
static lv_obj_t* g_weather_detail_uv = nullptr;
static lv_obj_t* g_weather_detail_precip_today = nullptr;
static lv_obj_t* g_weather_source = nullptr;
static lv_obj_t* g_weather_updated = nullptr;

const char* weather_name(WeatherCondition condition)
{
    switch (condition) {
        case WeatherCondition::Clear: return "Ensolarado";
        case WeatherCondition::Cloudy: return "Parcial nublado";
        case WeatherCondition::Fog: return "Neblina";
        case WeatherCondition::Drizzle: return "Chuva fraca";
        case WeatherCondition::Rain: return "Chuva";
        case WeatherCondition::Snow: return "Neve";
        case WeatherCondition::Thunderstorm: return "Tempestade";
        case WeatherCondition::Unknown: return "Clima indisponivel";
    }
    return "Clima indisponivel";
}

const char* weather_icon(WeatherCondition condition)
{
    switch (condition) {
        case WeatherCondition::Clear: return NP_I_WEATHER_SUNNY;
        case WeatherCondition::Cloudy: return NP_I_WEATHER_PARTLY_CLOUDY_DAY;
        case WeatherCondition::Fog: return NP_I_WEATHER_FOG;
        case WeatherCondition::Drizzle: return NP_I_WEATHER_RAIN_LIGHT;
        case WeatherCondition::Rain: return NP_I_WEATHER_RAIN;
        case WeatherCondition::Snow: return NP_I_WEATHER_SNOW;
        case WeatherCondition::Thunderstorm: return NP_I_WEATHER_THUNDER;
        case WeatherCondition::Unknown: return NP_I_WEATHER_CLOUDY;
    }
    return NP_I_WEATHER_CLOUDY;
}

const char* day_name_short(int weekday)
{
    static const char* names[] = { "Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab" };
    return (weekday >= 0 && weekday < 7) ? names[weekday] : "--";
}

const char* source_name(DataSource source)
{
    switch (source) {
        case DataSource::Live: return "live";
        case DataSource::Cache: return "cache";
        case DataSource::Mock: return "mock";
    }
    return "mock";
}

void set_line(lv_obj_t* label, const char* text)
{
    if (!label) return;
    const char* current = lv_label_get_text(label);
    if (!current || std::strcmp(current, text) != 0) {
        lv_label_set_text(label, text);
    }
}

void set_color(lv_obj_t* label, lv_color_t color)
{
    if (!label) return;
    const lv_color_t current = lv_obj_get_style_text_color(label, LV_PART_MAIN);
    if (current.red != color.red || current.green != color.green || current.blue != color.blue) {
        lv_obj_set_style_text_color(label, color, 0);
    }
}

void set_degree_line(lv_obj_t* label, double value)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.0f°", value);
    set_line(label, buf);
}

void set_percent_line(lv_obj_t* label, int value)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d%%", value);
    set_line(label, buf);
}

void set_minutes_as_time(lv_obj_t* label, int minutes)
{
    if (minutes < 0) {
        set_line(label, "--:--");
        return;
    }
    char buf[12];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes / 60, minutes % 60);
    set_line(label, buf);
}

}  // namespace

void np_update_weather(const AppState& state)
{
    if (!g_weather_temp || !g_weather_desc) {
        return;
    }

    set_line(g_weather_city, "SÃO PAULO, SP");

    if (state.weather.valid) {
        set_degree_line(g_weather_temp, state.weather.temperature_c);
        set_line(g_weather_desc, weather_name(state.weather.condition));
        set_line(g_weather_condition_icon, weather_icon(state.weather.condition));

        char high[16];
        char low[16];
        char feels[24];
        std::snprintf(high, sizeof(high), "%.0f°", state.weather.temp_max_c);
        std::snprintf(low, sizeof(low), "%.0f°", state.weather.temp_min_c);
        std::snprintf(feels, sizeof(feels), "Sensacao %.0f°", state.weather.feels_like_c);
        set_line(g_weather_range_max, high);
        set_line(g_weather_range_min, low);
        set_line(g_weather_feels, feels);
        set_minutes_as_time(g_weather_sunrise, state.weather.sunrise_minutes);
        set_minutes_as_time(g_weather_sunset, state.weather.sunset_minutes);

        for (std::size_t i = 0; i < kWeatherHourlySlots; ++i) {
            if (state.weather.hourly[i].valid) {
                char hour[8];
                std::snprintf(hour, sizeof(hour), "%02dh", state.weather.hourly[i].hour);
                set_line(g_weather_hourly_hour[i], hour);
                set_line(g_weather_hourly_icon[i], weather_icon(state.weather.hourly[i].condition));
                set_degree_line(g_weather_hourly_temp[i], state.weather.hourly[i].temperature_c);
                set_percent_line(g_weather_hourly_precip[i], state.weather.hourly[i].precip_pct);
            } else {
                set_line(g_weather_hourly_hour[i], "--");
                set_line(g_weather_hourly_icon[i], NP_I_WEATHER_CLOUDY);
                set_line(g_weather_hourly_temp[i], "--");
                set_line(g_weather_hourly_precip[i], "--");
            }
        }

        for (std::size_t i = 0; i < kWeatherDailySlots; ++i) {
            if (state.weather.daily[i].valid) {
                set_line(g_weather_day_name[i], day_name_short(state.weather.daily[i].weekday));
                set_line(g_weather_day_icon[i], weather_icon(state.weather.daily[i].condition));
                set_percent_line(g_weather_day_precip[i], state.weather.daily[i].precip_pct);
                set_degree_line(g_weather_day_min[i], state.weather.daily[i].temp_min_c);
                set_degree_line(g_weather_day_max[i], state.weather.daily[i].temp_max_c);
            } else {
                set_line(g_weather_day_name[i], "--");
                set_line(g_weather_day_icon[i], NP_I_WEATHER_CLOUDY);
                set_line(g_weather_day_precip[i], "--");
                set_line(g_weather_day_min[i], "--");
                set_line(g_weather_day_max[i], "--");
            }
        }

        char wind[24];
        char humidity[16];
        char uv[24];
        char clouds[16];
        std::snprintf(wind, sizeof(wind), "%.0f km/h", state.weather.wind_speed_kmh);
        std::snprintf(humidity, sizeof(humidity), "%d%%", state.weather.humidity_pct);
        std::snprintf(uv, sizeof(uv), "%.0f", state.weather.uv_index);
        std::snprintf(clouds, sizeof(clouds), "%d%%",
                      state.weather.condition == WeatherCondition::Cloudy ? 42 :
                      state.weather.condition == WeatherCondition::Rain ? 80 : 10);
        set_line(g_weather_detail_wind, wind);
        set_line(g_weather_detail_humidity, humidity);
        set_line(g_weather_detail_pressure, "1013 hPa");
        set_line(g_weather_detail_dew, "14°");
        set_line(g_weather_detail_visibility, "10 km");
        set_line(g_weather_detail_clouds, clouds);
        set_line(g_weather_detail_uv, uv);
        set_line(g_weather_detail_precip_today, "0 mm");

        char source[48];
        char updated[40];
        std::snprintf(source, sizeof(source), "Open-Meteo · %s", source_name(state.weather.source));
        std::snprintf(updated, sizeof(updated), "Atualizado %lus",
                      static_cast<unsigned long>(state.weather.last_update_ms / 1000UL));
        set_line(g_weather_source, source);
        set_line(g_weather_updated, updated);
        set_color(g_weather_source, state.weather.stale ? NP_C_ACCENT : lv_color_hex(0x7A8298));
    } else {
        set_line(g_weather_temp, "24°");
        set_line(g_weather_desc, "Parcial nublado");
        set_line(g_weather_condition_icon, NP_I_WEATHER_PARTLY_CLOUDY_DAY);
    }
}

void np_screen_weather(lv_obj_t *parent)
{
    const auto index = static_cast<std::size_t>(ScreenId::Weather);

    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[index] = scr;

    lv_obj_t *left = np_card(scr);
    lv_obj_set_size(left, 220, lv_pct(100));
    lv_obj_set_style_pad_all(left, 18, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);

    g_weather_city = np_label(left, "SÃO PAULO, SP", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *big = lv_obj_create(left);
    np_obj_clear_style(big);
    lv_obj_set_size(big, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(big, 1, 0);
    lv_obj_set_flex_flow(big, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(big,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(big, 6, 0);

    g_weather_condition_icon = np_label(big, NP_I_WEATHER_PARTLY_CLOUDY_DAY, NP_F_ICON_WEATHER_LG, NP_C_TEXT);
    g_weather_temp = np_label(big, "24°", NP_F_HERO, NP_C_TEXT);
    g_weather_desc = np_label(big, "Parcial nublado", NP_F_SM, NP_C_TEXT_DIM);

    lv_obj_t *mm = np_row(big);
    lv_obj_set_size(mm, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(mm, 12, 0);
    lv_obj_t *max_row = np_row(mm);
    lv_obj_set_size(max_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(max_row, 4, 0);
    np_label(max_row, NP_I_VALUE_UP, NP_F_ICON_SM, NP_C_BLUE);
    g_weather_range_max = np_label(max_row, "27°", NP_F_LG, NP_C_BLUE);

    lv_obj_t *min_row = np_row(mm);
    lv_obj_set_size(min_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(min_row, 4, 0);
    np_label(min_row, NP_I_VALUE_DOWN, NP_F_ICON_SM, lv_color_hex(0x5A6478));
    g_weather_range_min = np_label(min_row, "18°", NP_F_LG, lv_color_hex(0x5A6478));
    g_weather_feels = np_label(big, "Sensacao 26°", NP_F_XS, lv_color_hex(0x5A6478));

    lv_obj_t *sun_row = np_row(left);
    lv_obj_set_flex_align(sun_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_side(sun_row, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(sun_row, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(sun_row, 1, 0);
    lv_obj_set_style_pad_top(sun_row, 14, 0);

    lv_obj_t *sc1 = np_col(sun_row);
    lv_obj_set_size(sc1, 0, LV_SIZE_CONTENT);
    lv_obj_set_style_flex_grow(sc1, 1, 0);
    lv_obj_set_flex_align(sc1,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    np_label(sc1, NP_I_VALUE_UP, NP_F_ICON_SM, NP_C_ACCENT);
    g_weather_sunrise = np_label(sc1, "06:18", NP_F_MD, NP_C_TEXT);
    np_label(sc1, "Nascer", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *sc2 = np_col(sun_row);
    lv_obj_set_size(sc2, 0, LV_SIZE_CONTENT);
    lv_obj_set_style_flex_grow(sc2, 1, 0);
    lv_obj_set_flex_align(sc2,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    np_label(sc2, NP_I_VALUE_DOWN, NP_F_ICON_SM, NP_C_ACCENT);
    g_weather_sunset = np_label(sc2, "18:43", NP_F_MD, NP_C_TEXT);
    np_label(sc2, "Pôr", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *mid = lv_obj_create(scr);
    np_obj_clear_style(mid);
    lv_obj_set_style_flex_grow(mid, 1, 0);
    lv_obj_set_height(mid, lv_pct(100));
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mid, 12, 0);

    lv_obj_t *hr = np_card(mid);
    lv_obj_set_size(hr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(hr, 14, 0);
    lv_obj_set_flex_flow(hr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    np_label(hr, "PRÓXIMAS HORAS", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *hr_row = np_row(hr);
    lv_obj_set_size(hr_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(hr_row, 12, 0);
    lv_obj_set_flex_align(hr_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    for (std::size_t i = 0; i < kWeatherHourlySlots; ++i) {
        lv_obj_t *hc = np_col(hr_row);
        lv_obj_set_size(hc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(hc,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(hc, 5, 0);
        char hour_label[8];
        std::snprintf(hour_label, sizeof(hour_label), "%02uh", static_cast<unsigned>(10 + i));
        g_weather_hourly_hour[i] = np_label(hc, hour_label, NP_F_XS, lv_color_hex(0x5A6478));
        g_weather_hourly_icon[i] = np_label(hc, NP_I_WEATHER_PARTLY_CLOUDY_DAY, NP_F_ICON_WEATHER, NP_C_TEXT_MED);
        g_weather_hourly_temp[i] = np_label(hc, "25°", NP_F_MD, NP_C_ACCENT);
        lv_obj_t *bar = lv_bar_create(hc);
        lv_obj_set_size(bar, 6, 32);
        lv_bar_set_range(bar, 0, 100);
        lv_bar_set_value(bar, 15, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, NP_C_BORDER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, NP_C_BLUE, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
        g_weather_hourly_precip[i] = np_label(hc, "5%", NP_F_XS, NP_C_BLUE);
    }

    lv_obj_t *fd = np_card(mid);
    lv_obj_set_size(fd, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(fd, 1, 0);
    lv_obj_set_style_pad_all(fd, 14, 0);
    lv_obj_set_flex_flow(fd, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(fd,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    np_label(fd, "PRÓXIMOS 5 DIAS", NP_F_XS, NP_C_TEXT_MUTED);

    for (std::size_t i = 0; i < kWeatherDailySlots; ++i) {
        lv_obj_t *dr = np_row(fd);
        lv_obj_set_size(dr, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_border_side(dr, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(dr, NP_C_SEP, 0);
        lv_obj_set_style_border_width(dr, 1, 0);
        lv_obj_set_style_pad_ver(dr, 7, 0);
        lv_obj_set_flex_align(dr,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(dr, 8, 0);

        g_weather_day_name[i] = np_label(dr, "Qua", NP_F_SM, NP_C_TEXT_MED);
        lv_obj_set_width(g_weather_day_name[i], 32);
        g_weather_day_icon[i] = np_label(dr, NP_I_WEATHER_PARTLY_CLOUDY_DAY, NP_F_ICON_WEATHER, lv_color_hex(0x7A8298));
        lv_obj_set_width(g_weather_day_icon[i], 24);
        g_weather_day_precip[i] = np_label(dr, "10%", NP_F_XS_BOLD, NP_C_BLUE);
        lv_obj_set_width(g_weather_day_precip[i], 30);
        lv_label_set_long_mode(g_weather_day_precip[i], LV_LABEL_LONG_CLIP);
        g_weather_day_min[i] = np_label(dr, "18°", NP_F_SM_BOLD, NP_C_BLUE);
        lv_obj_set_width(g_weather_day_min[i], 32);
        lv_obj_set_style_text_align(g_weather_day_min[i], LV_TEXT_ALIGN_RIGHT, 0);
        g_weather_day_max[i] = np_label(dr, "27°", NP_F_SM_BOLD, NP_C_ACCENT);
        lv_obj_set_width(g_weather_day_max[i], 32);
        lv_obj_set_style_text_align(g_weather_day_max[i], LV_TEXT_ALIGN_RIGHT, 0);
    }

    lv_obj_t *right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_size(right, 200, lv_pct(100));
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 10, 0);

    lv_obj_t *det = np_card(right);
    lv_obj_set_size(det, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(det, 1, 0);
    lv_obj_set_style_pad_all(det, 14, 0);
    lv_obj_set_flex_flow(det, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(det,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    np_label(det, "DETALHES", NP_F_XS, NP_C_TEXT_MUTED);

    auto detail_row = [&](const char* k, lv_obj_t** out, const char* v, uint32_t c) {
        lv_obj_t *dr = np_row(det);
        lv_obj_set_size(dr, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_align(dr,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_top(dr, 11, 0);
        np_label(dr, k, NP_F_XS, lv_color_hex(0x7A8298));
        *out = np_label(dr, v, NP_F_SM, lv_color_hex(c));
    };
    detail_row("Vento", &g_weather_detail_wind, "12 km/h NE", 0xE8EAF2);
    detail_row("Umidade", &g_weather_detail_humidity, "54%", 0x4F7ECB);
    detail_row("Pressão", &g_weather_detail_pressure, "1013 hPa", 0xE8EAF2);
    detail_row("Ponto orv.", &g_weather_detail_dew, "14°", 0xE8EAF2);
    detail_row("Visibilidade", &g_weather_detail_visibility, "10 km", 0xE8EAF2);
    detail_row("Nuvens", &g_weather_detail_clouds, "42%", 0x7A8298);
    detail_row("Índice UV", &g_weather_detail_uv, "5 — Mod.", 0xE8A83C);
    detail_row("Precip. hoje", &g_weather_detail_precip_today, "0 mm", 0x4F7ECB);

    lv_obj_t *src = np_card(right);
    lv_obj_set_size(src, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(src, 12, 0);
    lv_obj_set_flex_flow(src, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(src,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    np_label(src, "FONTE", NP_F_XS, NP_C_TEXT_MUTED);
    g_weather_source = np_label(src, "Open-Meteo · cache", NP_F_SM, lv_color_hex(0x7A8298));
    lv_obj_set_style_margin_top(g_weather_source, 8, 0);
    g_weather_updated = np_label(src, "Atualizado 09:42", NP_F_XS, lv_color_hex(0x363C52));
}

}  // namespace nova
