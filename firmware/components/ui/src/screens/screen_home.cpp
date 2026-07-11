#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>

namespace nova {

namespace {

static lv_obj_t* g_clock_time = nullptr;
static lv_obj_t* g_clock_seconds = nullptr;
static lv_obj_t* g_clock_date = nullptr;
static lv_obj_t* g_weather_temp = nullptr;
static lv_obj_t* g_weather_city = nullptr;
static lv_obj_t* g_weather_range = nullptr;
static lv_obj_t* g_weather_desc = nullptr;
static lv_obj_t* g_weather_stat_value[4] = { nullptr };
static lv_obj_t* g_agenda_status = nullptr;
static lv_obj_t* g_market_dollar = nullptr;
static lv_obj_t* g_market_dollar_change = nullptr;
static lv_obj_t* g_market_ibov = nullptr;
static lv_obj_t* g_market_ibov_change = nullptr;
static lv_obj_t* g_market_btc = nullptr;
static lv_obj_t* g_market_btc_change = nullptr;

constexpr int kLeftWidth = 362;
constexpr int kColumnGap = 12;

const char* weekday_name(int weekday)
{
    static const char* names[] = {
        "domingo", "segunda-feira", "terca-feira", "quarta-feira",
        "quinta-feira", "sexta-feira", "sabado",
    };
    return (weekday >= 0 && weekday < 7) ? names[weekday] : "data";
}

const char* month_name(int month)
{
    static const char* names[] = {
        "", "janeiro", "fevereiro", "marco", "abril", "maio", "junho",
        "julho", "agosto", "setembro", "outubro", "novembro", "dezembro",
    };
    return (month >= 1 && month <= 12) ? names[month] : "mes";
}

const char* weather_name(WeatherCondition condition)
{
    switch (condition) {
        case WeatherCondition::Clear: return "Ensolarado";
        case WeatherCondition::Cloudy: return "Parcial nublado";
        case WeatherCondition::Fog: return "Neblina";
        case WeatherCondition::Drizzle: return "Garoa";
        case WeatherCondition::Rain: return "Chuva";
        case WeatherCondition::Snow: return "Neve";
        case WeatherCondition::Thunderstorm: return "Tempestade";
        case WeatherCondition::Unknown: return "Clima indisponivel";
    }
    return "Clima indisponivel";
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

void format_usd_whole(char* out, size_t out_size, double value)
{
    const unsigned long whole = static_cast<unsigned long>(value + 0.5);
    const unsigned long millions = whole / 1000000UL;
    const unsigned long thousands = (whole / 1000UL) % 1000UL;
    const unsigned long units = whole % 1000UL;

    if (millions > 0) {
        std::snprintf(out, out_size, "$ %lu.%03lu.%03lu", millions, thousands, units);
    } else if (thousands > 0) {
        std::snprintf(out, out_size, "$ %lu.%03lu", thousands, units);
    } else {
        std::snprintf(out, out_size, "$ %lu", units);
    }
}

void format_change(char* out, size_t out_size, double value)
{
    std::snprintf(out, out_size, "%s %.2f%%",
                  value >= 0.0 ? NP_I_VALUE_UP : NP_I_VALUE_DOWN,
                  value >= 0.0 ? value : -value);
}

void set_change_line(lv_obj_t* label, double value, bool valid)
{
    if (!valid) {
        set_line(label, "--");
        set_color(label, NP_C_TEXT_MUTED);
        return;
    }

    char text[24];
    format_change(text, sizeof(text), value);
    set_line(label, text);
    set_color(label, value >= 0.0 ? NP_C_GREEN : NP_C_RED);
}

}  // namespace

void np_update_home(const AppState& state)
{
    if (!g_clock_time || !g_clock_seconds || !g_clock_date) {
        return;
    }

    char time_text[8];
    std::snprintf(time_text, sizeof(time_text), "%02d:%02d",
                  state.clock.hour, state.clock.minute);
    set_line(g_clock_time, time_text);

    char seconds_text[4];
    std::snprintf(seconds_text, sizeof(seconds_text), "%02d", state.clock.second);
    set_line(g_clock_seconds, seconds_text);

    char date_text[64];
    std::snprintf(date_text, sizeof(date_text), "%s, %02d de %s",
                  weekday_name(state.clock.weekday),
                  state.clock.day,
                  month_name(state.clock.month));
    set_line(g_clock_date, date_text);

    if (state.weather.valid) {
        char temp_text[16];
        char range_text[32];
        char wind_text[24];
        char humidity_text[16];
        char feels_text[16];
        char uv_text[16];

        std::snprintf(temp_text, sizeof(temp_text), "%.0f°", state.weather.temperature_c);
        std::snprintf(range_text, sizeof(range_text), "%s %.0f°   %s %.0f°",
                      NP_I_VALUE_UP, state.weather.temp_max_c,
                      NP_I_VALUE_DOWN, state.weather.temp_min_c);
        std::snprintf(wind_text, sizeof(wind_text), "%.0f km/h", state.weather.wind_speed_kmh);
        std::snprintf(humidity_text, sizeof(humidity_text), "%d%%", state.weather.humidity_pct);
        std::snprintf(feels_text, sizeof(feels_text), "%.0f°", state.weather.feels_like_c);
        std::snprintf(uv_text, sizeof(uv_text), "%.0f", state.weather.uv_index);

        set_line(g_weather_temp, temp_text);
        set_line(g_weather_city, "Brasilia");
        set_line(g_weather_range, range_text);
        set_line(g_weather_desc, weather_name(state.weather.condition));
        set_line(g_weather_stat_value[0], wind_text);
        set_line(g_weather_stat_value[1], humidity_text);
        set_line(g_weather_stat_value[2], feels_text);
        set_line(g_weather_stat_value[3], uv_text);
        set_color(g_weather_stat_value[3], NP_C_ACCENT);
    } else {
        set_line(g_weather_temp, "--");
        set_line(g_weather_city, "Brasilia");
        set_line(g_weather_range, "--");
        set_line(g_weather_desc, "Aguardando clima");
        set_line(g_weather_stat_value[0], "--");
        set_line(g_weather_stat_value[1], "--");
        set_line(g_weather_stat_value[2], "--");
        set_line(g_weather_stat_value[3], "--");
        set_color(g_weather_stat_value[3], NP_C_TEXT_MUTED);
    }

    set_line(g_agenda_status, "Sem fonte ativa");

    if (state.forex.usd_brl_valid) {
        char dollar_text[24];
        std::snprintf(dollar_text, sizeof(dollar_text), "R$ %.2f", state.forex.usd_brl);
        set_line(g_market_dollar, dollar_text);
    } else {
        set_line(g_market_dollar, "R$ --");
    }
    set_change_line(g_market_dollar_change,
                    state.forex.usd_brl_change_pct_24h,
                    state.forex.usd_brl_valid);

    set_line(g_market_ibov, "--");
    set_line(g_market_ibov_change, "Sem fonte");
    set_color(g_market_ibov_change, NP_C_TEXT_MUTED);

    if (state.crypto.valid) {
        char btc_text[32];
        format_usd_whole(btc_text, sizeof(btc_text), state.crypto.btc_usd);
        set_line(g_market_btc, btc_text);
    } else {
        set_line(g_market_btc, "$ --");
    }
    set_change_line(g_market_btc_change,
                    state.crypto.btc_change_24h,
                    state.crypto.valid);
}

void np_screen_home(lv_obj_t* parent)
{
    const auto screen_index = static_cast<std::size_t>(ScreenId::Home);

    lv_obj_t* scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(scr, kColumnGap, 0);
    np_screens[screen_index] = scr;

    lv_obj_t* left = np_card(scr);
    lv_obj_set_size(left, kLeftWidth, lv_pct(100));
    lv_obj_set_style_pad_all(left, 20, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* clk_row = lv_obj_create(left);
    np_obj_clear_style(clk_row);
    lv_obj_set_size(clk_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(clk_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(clk_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(clk_row, 4, 0);

    g_clock_time = np_label(clk_row, "--:--", NP_F_CLOCK, NP_C_TEXT);
    g_clock_seconds = np_label(clk_row, "--", NP_F_3XL, lv_color_hex(0x3A4252));
    lv_obj_set_style_pad_bottom(g_clock_seconds, 8, 0);

    g_clock_date = np_label(left, "aguardando relogio", NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_margin_top(g_clock_date, 8, 0);

    lv_obj_t* spacer = lv_obj_create(left);
    np_obj_clear_style(spacer);
    lv_obj_set_size(spacer, 1, 1);
    lv_obj_set_style_flex_grow(spacer, 1, 0);

    lv_obj_t* sep = np_hsep(left);
    lv_obj_set_style_margin_bottom(sep, 14, 0);

    lv_obj_t* wx = lv_obj_create(left);
    lv_obj_set_size(wx, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wx, lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_bg_opa(wx, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wx, 0, 0);
    lv_obj_set_style_radius(wx, 10, 0);
    lv_obj_set_style_pad_all(wx, 14, 0);
    lv_obj_set_scrollbar_mode(wx, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wx, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* wr = np_row(wx);
    lv_obj_set_flex_align(wr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    g_weather_temp = np_label(wr, "--", NP_F_4XL, NP_C_TEXT);

    lv_obj_t* city_col = np_col(wr);
    lv_obj_set_size(city_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(city_col,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    g_weather_city = np_label(city_col, "Brasilia", NP_F_SM, lv_color_hex(0xC4C8D6));
    g_weather_range = np_label(city_col, "--", NP_F_XS, NP_C_TEXT_MUTED);

    g_weather_desc = np_label(wx, "Aguardando clima", NP_F_XS, NP_C_TEXT_DIM);

    lv_obj_t* wsp = lv_obj_create(wx);
    lv_obj_set_size(wsp, lv_pct(100), 1);
    lv_obj_set_style_bg_color(wsp, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_bg_opa(wsp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wsp, 0, 0);
    lv_obj_set_style_radius(wsp, 0, 0);
    lv_obj_set_style_margin_top(wsp, 11, 0);
    lv_obj_set_style_margin_bottom(wsp, 11, 0);

    lv_obj_t* wst = np_row(wx);
    lv_obj_set_flex_align(wst,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static const char* stat_labels[] = { "Vento", "Umidade", "Sensacao", "UV" };
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* sc = np_col(wst);
        lv_obj_set_size(sc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(sc,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        np_label(sc, stat_labels[i], NP_F_XS, NP_C_TEXT_MUTED);
        g_weather_stat_value[i] = np_label(sc, "--", NP_F_MD,
                                           i == 3 ? NP_C_ACCENT : NP_C_TEXT);
        lv_obj_set_style_margin_top(g_weather_stat_value[i], 3, 0);
    }

    lv_obj_t* right = lv_obj_create(scr);
    lv_obj_set_style_flex_grow(right, 1, 0);
    lv_obj_set_height(right, lv_pct(100));
    np_obj_clear_style(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(right, kColumnGap, 0);

    lv_obj_t* ag = np_card(right);
    lv_obj_set_size(ag, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(ag, 1, 0);
    lv_obj_set_style_pad_all(ag, 16, 0);
    lv_obj_set_flex_flow(ag, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ag,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* agh = np_row(ag);
    lv_obj_set_flex_align(agh,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(agh, 9, 0);
    lv_obj_set_style_margin_bottom(agh, 10, 0);
    np_label(agh, NP_I_LIST, NP_F_ICON_SM, NP_C_ACCENT);
    lv_obj_t* ag_title = np_label(agh, "Agenda de hoje", NP_F_SM, NP_C_TEXT);
    lv_obj_set_style_flex_grow(ag_title, 1, 0);
    g_agenda_status = np_label(agh, "Sem fonte ativa", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t* ag_empty = lv_obj_create(ag);
    np_obj_clear_style(ag_empty);
    lv_obj_set_size(ag_empty, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(ag_empty, 1, 0);
    lv_obj_set_flex_flow(ag_empty, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ag_empty,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ag_empty, 8, 0);
    np_label(ag_empty, NP_I_AGENDA, NP_F_ICON_LG, NP_C_TEXT_MUTED);
    np_label(ag_empty, "Sem fonte ativa", NP_F_SM, NP_C_TEXT_MUTED);

    lv_obj_t* mk = np_card(right);
    lv_obj_set_size(mk, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(mk, 1, 0);
    lv_obj_set_style_pad_all(mk, 13, 0);
    lv_obj_set_flex_flow(mk, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mk,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* mh = np_row(mk);
    lv_obj_set_style_pad_column(mh, 6, 0);
    lv_obj_set_style_margin_bottom(mh, 9, 0);
    np_label(mh, NP_I_CHART, NP_F_ICON_SM, NP_C_ACCENT);
    np_label(mh, "Mercado", NP_F_SM, NP_C_TEXT);

    struct MarketRowWidgets {
        lv_obj_t** value;
        lv_obj_t** change;
    };

    const char* labels[] = { "Dolar", "Ibovespa", "Bitcoin" };
    MarketRowWidgets widgets[] = {
        { &g_market_dollar, &g_market_dollar_change },
        { &g_market_ibov, &g_market_ibov_change },
        { &g_market_btc, &g_market_btc_change },
    };

    for (int i = 0; i < 3; ++i) {
        lv_obj_t* row = np_row(mk);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_bottom(row, 6, 0);

        lv_obj_t* name = np_label(row, labels[i], NP_F_SM, lv_color_hex(0x5A6478));
        lv_obj_set_style_flex_grow(name, 1, 0);

        *widgets[i].value = np_label(row, "--", NP_F_SM, NP_C_TEXT);
        lv_obj_set_style_margin_right(*widgets[i].value, 6, 0);
        *widgets[i].change = np_label(row, "--", NP_F_XS, NP_C_TEXT_MUTED);
    }
}

}  // namespace nova
