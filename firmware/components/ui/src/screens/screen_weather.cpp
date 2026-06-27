#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>

namespace nova {

namespace {

static lv_obj_t* g_weather_city = nullptr;
static lv_obj_t* g_weather_temp = nullptr;
static lv_obj_t* g_weather_desc = nullptr;
static lv_obj_t* g_weather_range_max = nullptr;
static lv_obj_t* g_weather_range_min = nullptr;
static lv_obj_t* g_weather_feels = nullptr;
static lv_obj_t* g_weather_hourly_temp[8] = { nullptr };
static lv_obj_t* g_weather_hourly_precip[8] = { nullptr };
static lv_obj_t* g_weather_day_name[5] = { nullptr };
static lv_obj_t* g_weather_day_cond[5] = { nullptr };
static lv_obj_t* g_weather_day_precip[5] = { nullptr };
static lv_obj_t* g_weather_day_min[5] = { nullptr };
static lv_obj_t* g_weather_day_max[5] = { nullptr };
static lv_obj_t* g_weather_detail_wind = nullptr;
static lv_obj_t* g_weather_detail_humidity = nullptr;
static lv_obj_t* g_weather_detail_uv = nullptr;
static lv_obj_t* g_weather_detail_clouds = nullptr;
static lv_obj_t* g_weather_source = nullptr;
static lv_obj_t* g_weather_updated = nullptr;

const char* weather_name(WeatherCondition condition)
{
    switch (condition) {
        case WeatherCondition::Clear: return "Sol";
        case WeatherCondition::Cloudy: return "Nublado";
        case WeatherCondition::Fog: return "Neblina";
        case WeatherCondition::Drizzle: return "Garoa";
        case WeatherCondition::Rain: return "Chuva";
        case WeatherCondition::Snow: return "Neve";
        case WeatherCondition::Thunderstorm: return "Tempestade";
        case WeatherCondition::Unknown: return "Clima indisponivel";
    }
    return "Clima indisponivel";
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
    if (!label) {
        return;
    }

    const char* current = lv_label_get_text(label);
    if (!current || std::strcmp(current, text) != 0) {
        lv_label_set_text(label, text);
    }
}

void set_color(lv_obj_t* label, lv_color_t color)
{
    if (!label) {
        return;
    }

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

void set_range_line(lv_obj_t* label, const char* prefix, double value)
{
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%s %.0f°", prefix, value);
    set_line(label, buf);
}

void set_percent_line(lv_obj_t* label, int value)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d%%", value);
    set_line(label, buf);
}

void set_text_and_width(lv_obj_t* label, const char* text, lv_coord_t width)
{
    if (!label) {
        return;
    }
    set_line(label, text);
    lv_obj_set_width(label, width);
}

}  // namespace

void np_update_weather(const AppState& state)
{
    if (!g_weather_temp || !g_weather_desc) {
        return;
    }

    const bool valid = state.weather.valid;

    set_line(g_weather_city, "BRASILIA, DF");

    if (valid) {
        set_degree_line(g_weather_temp, state.weather.temperature_c);
        set_line(g_weather_desc, weather_name(state.weather.condition));
        set_range_line(g_weather_range_max, "max", state.weather.temp_max_c);
        set_range_line(g_weather_range_min, "min", state.weather.temp_min_c);

        char feels[24];
        std::snprintf(feels, sizeof(feels), "Sensacao %.0f°", state.weather.feels_like_c);
        set_line(g_weather_feels, feels);

        char wind[24];
        std::snprintf(wind, sizeof(wind), "%.0f km/h", state.weather.wind_speed_kmh);
        set_line(g_weather_detail_wind, wind);
        set_percent_line(g_weather_detail_humidity, state.weather.humidity_pct);

        char uv[24];
        std::snprintf(uv, sizeof(uv), "%.0f", state.weather.uv_index);
        set_line(g_weather_detail_uv, uv);

        set_percent_line(g_weather_detail_clouds,
                         state.weather.condition == WeatherCondition::Cloudy ? 70 :
                         state.weather.condition == WeatherCondition::Rain ? 85 :
                         state.weather.condition == WeatherCondition::Fog ? 90 : 25);

        char source[48];
        std::snprintf(source, sizeof(source), "Open-Meteo / %s", source_name(state.weather.source));
        set_line(g_weather_source, source);
        set_color(g_weather_source, state.weather.stale ? NP_C_ACCENT : lv_color_hex(0x7A8298));

        char updated[40];
        std::snprintf(updated, sizeof(updated), "Atualizado %lus",
                      static_cast<unsigned long>(state.weather.last_update_ms / 1000UL));
        set_line(g_weather_updated, updated);
    } else {
        set_line(g_weather_temp, "--");
        set_line(g_weather_desc, "Aguardando clima");
        set_line(g_weather_range_max, "max --");
        set_line(g_weather_range_min, "min --");
        set_line(g_weather_feels, "Sem sensacao termica");
        set_line(g_weather_detail_wind, "--");
        set_line(g_weather_detail_humidity, "--");
        set_line(g_weather_detail_uv, "--");
        set_line(g_weather_detail_clouds, "--");
        set_line(g_weather_source, "Open-Meteo / mock");
        set_color(g_weather_source, lv_color_hex(0x7A8298));
        set_line(g_weather_updated, "Sem atualizacao");
    }

    for (int i = 0; i < 8; ++i) {
        const double temp = valid ? (state.weather.temperature_c + ((i < 3) ? i : 2 - (i - 2) * 0.8)) : 0.0;
        const int precip = valid ? ((state.weather.condition == WeatherCondition::Rain || state.weather.condition == WeatherCondition::Thunderstorm)
                                      ? (20 + i * 8) : (5 + i * 2))
                                 : 0;
        set_degree_line(g_weather_hourly_temp[i], temp);
        set_percent_line(g_weather_hourly_precip[i], precip);
    }

    static const char* days[5] = { "Qua", "Qui", "Sex", "Sab", "Dom" };
    for (int i = 0; i < 5; ++i) {
        set_line(g_weather_day_name[i], days[i]);
        set_text_and_width(g_weather_day_cond[i], weather_name(state.weather.condition), 140);

        const int precip = valid ? ((state.weather.condition == WeatherCondition::Rain) ? (40 + i * 6) : (5 + i * 3)) : 0;
        set_percent_line(g_weather_day_precip[i], precip);
        set_degree_line(g_weather_day_min[i], valid ? state.weather.temp_min_c - (i % 2) : 0.0);
        set_degree_line(g_weather_day_max[i], valid ? state.weather.temp_max_c + (i % 2) : 0.0);
    }
}

void np_screen_weather(lv_obj_t* parent)
{
    const auto index = static_cast<std::size_t>(ScreenId::Weather);

    lv_obj_t* scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[index] = scr;

    lv_obj_t* left = np_card(scr);
    lv_obj_set_size(left, 220, lv_pct(100));
    lv_obj_set_style_pad_all(left, 18, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);

    g_weather_city = np_label(left, "BRASILIA, DF", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t* big = lv_obj_create(left);
    np_obj_clear_style(big);
    lv_obj_set_size(big, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(big, 1, 0);
    lv_obj_set_flex_flow(big, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(big,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(big, 6, 0);

    g_weather_temp = np_label(big, "--", NP_F_TEMP, NP_C_TEXT);
    g_weather_desc = np_label(big, "Aguardando clima", NP_F_SM, NP_C_TEXT_DIM);
    lv_label_set_long_mode(g_weather_desc, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_weather_desc, 160);

    lv_obj_t* mm = np_row(big);
    lv_obj_set_size(mm, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(mm, 12, 0);
    g_weather_range_max = np_label(mm, "max --", NP_F_SM, NP_C_BLUE);
    g_weather_range_min = np_label(mm, "min --", NP_F_SM, lv_color_hex(0x5A6478));

    g_weather_feels = np_label(big, "Sem sensacao termica", NP_F_XS, lv_color_hex(0x5A6478));

    lv_obj_t* sun_row = np_row(left);
    lv_obj_set_flex_align(sun_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_side(sun_row, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(sun_row, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(sun_row, 1, 0);
    lv_obj_set_style_pad_top(sun_row, 14, 0);

    const char* sun_times[2] = { "06:18", "18:43" };
    const char* sun_labels[2] = { "Nascer", "Por" };
    for (int i = 0; i < 2; ++i) {
        lv_obj_t* col = np_col(sun_row);
        lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(col,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(col, 3, 0);
        np_label(col, sun_times[i], NP_F_MD, NP_C_TEXT);
        np_label(col, sun_labels[i], NP_F_XS, NP_C_TEXT_MUTED);
    }

    lv_obj_t* mid = lv_obj_create(scr);
    np_obj_clear_style(mid);
    lv_obj_set_style_flex_grow(mid, 1, 0);
    lv_obj_set_height(mid, lv_pct(100));
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mid, 12, 0);

    lv_obj_t* hr = np_card(mid);
    lv_obj_set_size(hr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(hr, 14, 0);
    np_label(hr, "PROXIMAS HORAS", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t* hr_row = np_row(hr);
    lv_obj_set_style_margin_top(hr_row, 12, 0);
    lv_obj_set_flex_align(hr_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    for (int i = 0; i < 8; ++i) {
        lv_obj_t* col = np_col(hr_row);
        lv_obj_set_size(col, 40, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(col,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(col, 5, 0);

        char hour_buf[8];
        std::snprintf(hour_buf, sizeof(hour_buf), "%02dh", 10 + i);
        np_label(col, hour_buf, NP_F_XS, lv_color_hex(0x5A6478));
        g_weather_hourly_temp[i] = np_label(col, "--", NP_F_MD, NP_C_ACCENT);

        lv_obj_t* bar = lv_bar_create(col);
        lv_obj_set_size(bar, 6, 32);
        lv_bar_set_range(bar, 0, 100);
        lv_bar_set_value(bar, 15, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, NP_C_BORDER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, NP_C_BLUE, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);

        g_weather_hourly_precip[i] = np_label(col, "--", NP_F_XS, NP_C_BLUE);
    }

    lv_obj_t* fd = np_card(mid);
    lv_obj_set_size(fd, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(fd, 1, 0);
    lv_obj_set_style_pad_all(fd, 14, 0);
    np_label(fd, "PROXIMOS 5 DIAS", NP_F_XS, NP_C_TEXT_MUTED);

    for (int i = 0; i < 5; ++i) {
        lv_obj_t* row = np_row(fd);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, NP_C_SEP, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 7, 0);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 10, 0);

        g_weather_day_name[i] = np_label(row, "--", NP_F_SM, NP_C_TEXT_MED);
        lv_obj_set_width(g_weather_day_name[i], 32);
        g_weather_day_cond[i] = np_label(row, "--", NP_F_XS, lv_color_hex(0x7A8298));
        lv_label_set_long_mode(g_weather_day_cond[i], LV_LABEL_LONG_DOT);
        lv_obj_set_width(g_weather_day_cond[i], 92);
        g_weather_day_precip[i] = np_label(row, "--", NP_F_XS, lv_color_hex(0x5A6478));
        lv_obj_set_width(g_weather_day_precip[i], 36);
        lv_obj_set_style_text_align(g_weather_day_precip[i], LV_TEXT_ALIGN_CENTER, 0);
        g_weather_day_min[i] = np_label(row, "--", NP_F_SM, NP_C_BLUE);
        lv_obj_set_width(g_weather_day_min[i], 32);
        lv_obj_set_style_text_align(g_weather_day_min[i], LV_TEXT_ALIGN_RIGHT, 0);
        g_weather_day_max[i] = np_label(row, "--", NP_F_SM, NP_C_ACCENT);
        lv_obj_set_width(g_weather_day_max[i], 32);
        lv_obj_set_style_text_align(g_weather_day_max[i], LV_TEXT_ALIGN_RIGHT, 0);
    }

    lv_obj_t* right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_size(right, 200, lv_pct(100));
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 10, 0);

    lv_obj_t* det = np_card(right);
    lv_obj_set_size(det, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(det, 1, 0);
    lv_obj_set_style_pad_all(det, 14, 0);
    np_label(det, "DETALHES", NP_F_XS, NP_C_TEXT_MUTED);

    const char* detail_keys[8] = {
        "Vento", "Umidade", "Pressao", "Ponto orv.", "Visibilidade", "Nuvens", "Indice UV", "Precip. hoje"
    };
    lv_obj_t** detail_values[8] = {
        &g_weather_detail_wind, &g_weather_detail_humidity, nullptr, nullptr,
        nullptr, &g_weather_detail_clouds, &g_weather_detail_uv, nullptr
    };
    const char* defaults[8] = { "--", "--", "1013 hPa", "14°", "10 km", "--", "--", "0 mm" };
    const uint32_t colors[8] = { 0xE8EAF2, 0x4F7ECB, 0xE8EAF2, 0xE8EAF2, 0xE8EAF2, 0x7A8298, 0xE8A83C, 0x4F7ECB };

    for (int i = 0; i < 8; ++i) {
        lv_obj_t* row = np_row(det);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_top(row, 11, 0);
        np_label(row, detail_keys[i], NP_F_XS, lv_color_hex(0x7A8298));
        lv_obj_t* value = np_label(row, defaults[i], NP_F_SM, lv_color_hex(colors[i]));
        if (detail_values[i]) {
            *detail_values[i] = value;
        }
    }

    lv_obj_t* src = np_card(right);
    lv_obj_set_size(src, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(src, 12, 0);
    np_label(src, "FONTE", NP_F_XS, NP_C_TEXT_MUTED);
    g_weather_source = np_label(src, "Open-Meteo / mock", NP_F_SM, lv_color_hex(0x7A8298));
    lv_label_set_long_mode(g_weather_source, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_weather_source, 150);
    lv_obj_set_style_margin_top(g_weather_source, 8, 0);
    g_weather_updated = np_label(src, "Sem atualizacao", NP_F_XS, lv_color_hex(0x363C52));
}

}  // namespace nova
