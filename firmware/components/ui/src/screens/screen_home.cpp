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
static lv_obj_t* g_weather_condition_icon = nullptr;
static lv_obj_t* g_weather_status_icon = nullptr;
static lv_obj_t* g_weather_stat_value[4] = { nullptr };
static lv_obj_t* g_market_btc = nullptr;
static lv_obj_t* g_market_fx = nullptr;
static lv_obj_t* g_market_change_icon = nullptr;
static lv_obj_t* g_market_change = nullptr;
static lv_obj_t* g_market_status_icon = nullptr;

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
        case WeatherCondition::Cloudy: return "Parcialmente nublado";
        case WeatherCondition::Fog: return "Neblina";
        case WeatherCondition::Drizzle: return "Garoa";
        case WeatherCondition::Rain: return "Chuva";
        case WeatherCondition::Snow: return "Neve";
        case WeatherCondition::Thunderstorm: return "Tempestade";
        case WeatherCondition::Unknown: return "Clima indisponível";
    }
    return "Clima indisponível";
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

void set_line_if_changed(lv_obj_t* label, const char* text)
{
    if (!label) {
        return;
    }

    const char* current = lv_label_get_text(label);
    if (!current || std::strcmp(current, text) != 0) {
        lv_label_set_text(label, text);
    }
}

void set_color_if_changed(lv_obj_t* label, lv_color_t color)
{
    if (!label) {
        return;
    }
    const lv_color_t current = lv_obj_get_style_text_color(label, LV_PART_MAIN);
    if (current.red != color.red || current.green != color.green || current.blue != color.blue) {
        lv_obj_set_style_text_color(label, color, 0);
    }
}

void set_status_icon(lv_obj_t* label, bool has_data, bool stale)
{
    if (!label) {
        return;
    }
    if (!has_data) {
        set_line_if_changed(label, NP_I_WARNING);
        set_color_if_changed(label, NP_C_TEXT_MUTED);
        return;
    }
    set_line_if_changed(label, stale ? NP_I_CACHED : NP_I_REFRESH);
    set_color_if_changed(label, stale ? NP_C_ACCENT : NP_C_GREEN);
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

}  // namespace

void np_update_home(const AppState& state)
{
    if (!g_clock_time || !g_clock_seconds || !g_clock_date) {
        return;
    }

    char time_text[8];
    std::snprintf(time_text, sizeof(time_text), "%02d:%02d",
                  state.clock.hour, state.clock.minute);
    set_line_if_changed(g_clock_time, time_text);

    char seconds_text[4];
    std::snprintf(seconds_text, sizeof(seconds_text), "%02d", state.clock.second);
    set_line_if_changed(g_clock_seconds, seconds_text);

    char date_text[64];
    std::snprintf(date_text, sizeof(date_text), "%s, %02d de %s",
                  weekday_name(state.clock.weekday),
                  state.clock.day,
                  month_name(state.clock.month));
    set_line_if_changed(g_clock_date, date_text);

    if (g_weather_temp && g_weather_city && g_weather_range && g_weather_desc) {
        if (state.weather.valid) {
            char temp_text[16];
            char range_text[32];
            char stat_wind[24];
            char stat_humidity[16];
            char stat_feels[16];
            char stat_uv[16];
            std::snprintf(temp_text, sizeof(temp_text), "%.0f°", state.weather.temperature_c);
            std::snprintf(range_text, sizeof(range_text), "%s %.0f°   %s %.0f°",
                          LV_SYMBOL_UP, state.weather.temp_max_c,
                          LV_SYMBOL_DOWN, state.weather.temp_min_c);
            std::snprintf(stat_wind, sizeof(stat_wind), "%.0f km/h", state.weather.wind_speed_kmh);
            std::snprintf(stat_humidity, sizeof(stat_humidity), "%d%%", state.weather.humidity_pct);
            std::snprintf(stat_feels, sizeof(stat_feels), "%.0f°", state.weather.feels_like_c);
            std::snprintf(stat_uv, sizeof(stat_uv), "%.0f", state.weather.uv_index);

            set_line_if_changed(g_weather_temp, temp_text);
            set_line_if_changed(g_weather_city, "São Paulo");
            set_line_if_changed(g_weather_range, range_text);
            set_line_if_changed(g_weather_desc, weather_name(state.weather.condition));
            set_line_if_changed(g_weather_stat_value[0], stat_wind);
            set_line_if_changed(g_weather_stat_value[1], stat_humidity);
            set_line_if_changed(g_weather_stat_value[2], stat_feels);
            set_line_if_changed(g_weather_stat_value[3], stat_uv);
            set_color_if_changed(g_weather_stat_value[3], NP_C_ACCENT);
            if (g_weather_condition_icon) {
                set_line_if_changed(g_weather_condition_icon, weather_icon(state.weather.condition));
                set_color_if_changed(g_weather_condition_icon, NP_C_TEXT);
            }
            if (g_weather_status_icon) {
                set_status_icon(g_weather_status_icon, state.weather.valid, state.weather.stale);
            }
        } else {
            set_line_if_changed(g_weather_temp, "--");
            set_line_if_changed(g_weather_city, "São Paulo");
            set_line_if_changed(g_weather_range, "--");
            set_line_if_changed(g_weather_desc, "Aguardando clima");
            set_line_if_changed(g_weather_stat_value[0], "--");
            set_line_if_changed(g_weather_stat_value[1], "--");
            set_line_if_changed(g_weather_stat_value[2], "--");
            set_line_if_changed(g_weather_stat_value[3], "--");
            set_color_if_changed(g_weather_stat_value[3], NP_C_TEXT_MUTED);
            if (g_weather_condition_icon) {
                set_line_if_changed(g_weather_condition_icon, NP_I_WEATHER_CLOUDY);
                set_color_if_changed(g_weather_condition_icon, NP_C_TEXT_MUTED);
            }
            if (g_weather_status_icon) {
                set_status_icon(g_weather_status_icon, false, false);
            }
        }
    }

    if (g_market_btc && g_market_fx && g_market_change && g_market_change_icon) {
        if (state.crypto.valid) {
            char btc_text[64];
            char change_text[32];
            format_usd_whole(btc_text, sizeof(btc_text), state.crypto.btc_usd);
            std::snprintf(change_text, sizeof(change_text), "%.2f%%",
                          state.crypto.btc_change_24h >= 0.0 ? state.crypto.btc_change_24h
                                                             : -state.crypto.btc_change_24h);
            set_line_if_changed(g_market_btc, btc_text);
            set_line_if_changed(g_market_change_icon,
                                state.crypto.btc_change_24h >= 0.0 ? NP_I_VALUE_UP : NP_I_VALUE_DOWN);
            set_line_if_changed(g_market_change, change_text);
            set_color_if_changed(g_market_change_icon,
                                 state.crypto.btc_change_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);
            set_color_if_changed(g_market_change,
                                 state.crypto.btc_change_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);
        } else {
            set_line_if_changed(g_market_btc, "$ --");
            set_line_if_changed(g_market_change_icon, "");
            set_line_if_changed(g_market_change, "--");
            set_color_if_changed(g_market_change_icon, NP_C_TEXT_MUTED);
            set_color_if_changed(g_market_change, NP_C_TEXT_MUTED);
        }

        if (state.forex.usd_brl_valid) {
            char fx_text[32];
            std::snprintf(fx_text, sizeof(fx_text), "R$ %.2f", state.forex.usd_brl);
            set_line_if_changed(g_market_fx, fx_text);
        } else {
            set_line_if_changed(g_market_fx, "R$ --");
        }
        set_status_icon(g_market_status_icon,
                        state.crypto.valid || state.forex.usd_brl_valid,
                        state.crypto.stale || state.forex.usd_brl_stale);
    }
}

void np_screen_home(lv_obj_t *parent)
{
    const auto screen_index = static_cast<std::size_t>(ScreenId::Home);

    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(scr, kColumnGap, 0);
    np_screens[screen_index] = scr;

    lv_obj_t *left = np_card(scr);
    lv_obj_set_size(left, kLeftWidth, lv_pct(100));
    lv_obj_set_style_pad_all(left, 20, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *clk_row = lv_obj_create(left);
    np_obj_clear_style(clk_row);
    lv_obj_set_size(clk_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(clk_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(clk_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(clk_row, 4, 0);

    g_clock_time = np_label(clk_row, "--:--", NP_F_CLOCK, NP_C_TEXT);
    g_clock_seconds = np_label(clk_row, "--", NP_F_3XL, lv_color_hex(0x3A4252));
    lv_obj_set_style_pad_bottom(g_clock_seconds, 8, 0);
    g_clock_date = np_label(left, "aguardando relógio", NP_F_SM_BOLD, NP_C_TEXT_DIM);
    lv_obj_set_style_margin_top(g_clock_date, 8, 0);

    lv_obj_t *sp = lv_obj_create(left);
    np_obj_clear_style(sp);
    lv_obj_set_size(sp, 1, 1);
    lv_obj_set_style_flex_grow(sp, 1, 0);

    lv_obj_t *hs = np_hsep(left);
    lv_obj_set_style_margin_bottom(hs, 14, 0);

    lv_obj_t *wx = lv_obj_create(left);
    lv_obj_set_size(wx, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wx, lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_bg_opa(wx, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wx, 0, 0);
    lv_obj_set_style_radius(wx, 10, 0);
    lv_obj_set_style_pad_all(wx, 14, 0);
    lv_obj_set_scrollbar_mode(wx, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wx, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *wxh = np_row(wx);
    lv_obj_set_flex_align(wxh,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *wxh_left = np_row(wxh);
    lv_obj_set_size(wxh_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(wxh_left, 8, 0);
    np_label(wxh_left, NP_I_WEATHER_PARTLY_CLOUDY_DAY, NP_F_ICON_SM, NP_C_ACCENT);
    np_label(wxh_left, "Meteorologia", NP_F_SM_BOLD, NP_C_TEXT);
    g_weather_status_icon = np_label(wxh, NP_I_WARNING, NP_F_ICON_SM, NP_C_TEXT_MUTED);

    lv_obj_t *wr = np_row(wx);
    lv_obj_set_flex_align(wr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *wx_left = np_row(wr);
    lv_obj_set_size(wx_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(wx_left, 10, 0);
    g_weather_condition_icon = np_label(wx_left, NP_I_WEATHER_CLOUDY, NP_F_ICON_WEATHER_LG, NP_C_TEXT_MUTED);
    g_weather_temp = np_label(wx_left, "--", NP_F_TEMP, NP_C_TEXT);

    lv_obj_t *city_col = np_col(wr);
    lv_obj_set_size(city_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(city_col,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    g_weather_city = np_label(city_col, "São Paulo", NP_F_SM, lv_color_hex(0xC4C8D6));
    g_weather_range = np_label(city_col, "--", NP_F_XS, NP_C_TEXT_MUTED);
    g_weather_desc = np_label(wx, "Aguardando clima", NP_F_XS, NP_C_TEXT_DIM);

    lv_obj_t *wsp = lv_obj_create(wx);
    lv_obj_set_size(wsp, lv_pct(100), 1);
    lv_obj_set_style_bg_color(wsp, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_bg_opa(wsp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wsp, 0, 0);
    lv_obj_set_style_radius(wsp, 0, 0);
    lv_obj_set_style_margin_top(wsp, 11, 0);
    lv_obj_set_style_margin_bottom(wsp, 11, 0);

    lv_obj_t *wst = np_row(wx);
    lv_obj_set_flex_align(wst,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static const char* stat_labels[] = { "Vento", "Umidade", "Sensação", "UV" };
    for (int i = 0; i < 4; ++i) {
        lv_obj_t *sc = np_col(wst);
        lv_obj_set_size(sc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(sc,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        np_label(sc, stat_labels[i], NP_F_XS, NP_C_TEXT_MUTED);
        g_weather_stat_value[i] = np_label(sc, "--", NP_F_SM_BOLD,
                                           i == 3 ? NP_C_ACCENT : lv_color_hex(0xE8EAF2));
        lv_obj_set_style_margin_top(g_weather_stat_value[i], 3, 0);
    }

    lv_obj_t *right = lv_obj_create(scr);
    lv_obj_set_style_flex_grow(right, 1, 0);
    lv_obj_set_height(right, lv_pct(100));
    np_obj_clear_style(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(right, kColumnGap, 0);

    lv_obj_t *ag = np_card(right);
    lv_obj_set_size(ag, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(ag, 1, 0);
    lv_obj_set_style_pad_all(ag, 16, 0);
    lv_obj_set_flex_flow(ag, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ag,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *agh = np_row(ag);
    lv_obj_set_flex_align(agh,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(agh, 9, 0);
    lv_obj_set_style_margin_bottom(agh, 10, 0);
    np_label(agh, NP_I_AGENDA, NP_F_ICON_SM, NP_C_ACCENT);
    lv_obj_t *ag_t = np_label(agh, "Agenda de hoje", NP_F_SM_BOLD, NP_C_TEXT);
    lv_obj_set_style_flex_grow(ag_t, 1, 0);
    np_label(agh, "4 eventos", NP_F_XS, NP_C_TEXT_MUTED);
    np_chip(agh, "Ver tudo >");

    static const struct {
        const char *t;
        const char *title;
        const char *sub;
        uint32_t c;
    } evts[] = {
        { "15:30", "Reunião - NoiseBot team", "Trabalho · 45 min", 0xE8A83C },
        { "17:00", "Academia", "Pessoal · 1h", 0x4ABB78 },
        { "19:30", "Jantar com Marina", "Pessoal · 2h", 0x4F7ECB },
        { "21:00", "Revisar firmware v1.4", "Projeto · 30 min", 0xB77ABB },
    };
    for (int i = 0; i < 4; ++i) {
        lv_obj_t *row = np_row(ag);
        lv_obj_set_style_pad_column(row, 11, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, NP_C_SEP, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 8, 0);

        lv_obj_t *tm = np_label(row, evts[i].t, NP_F_SM, NP_C_TEXT_MED);
        lv_obj_set_width(tm, 38);

        lv_obj_t *bar = lv_obj_create(row);
        lv_obj_set_size(bar, 3, 28);
        lv_obj_set_style_bg_color(bar, lv_color_hex(evts[i].c), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        lv_obj_t *ttl = np_label(info, evts[i].title, NP_F_SM_BOLD, NP_C_TEXT);
        lv_label_set_long_mode(ttl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(ttl, 260);
        lv_obj_t *sub = np_label(info, evts[i].sub, NP_F_XS, lv_color_hex(0x5A6478));
        lv_obj_set_style_margin_top(sub, 1, 0);
    }

    lv_obj_t *br = lv_obj_create(right);
    np_obj_clear_style(br);
    lv_obj_set_size(br, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(br, 1, 0);
    lv_obj_set_flex_flow(br, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(br,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *mk = np_card(br);
    lv_obj_set_size(mk, lv_pct(100), lv_pct(100));
    lv_obj_set_height(mk, lv_pct(100));
    lv_obj_set_style_pad_all(mk, 13, 0);
    lv_obj_set_flex_flow(mk, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mk,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *mh = np_row(mk);
    lv_obj_set_flex_align(mh,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(mh, 9, 0);
    lv_obj_t *mh_left = np_row(mh);
    lv_obj_set_size(mh_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(mh_left, 6, 0);
    np_label(mh_left, NP_I_CHART, NP_F_ICON_SM, NP_C_ACCENT);
    np_label(mh_left, "Mercado", NP_F_SM_BOLD, NP_C_TEXT);
    g_market_status_icon = np_label(mh, NP_I_WARNING, NP_F_ICON_SM, NP_C_TEXT_MUTED);

    lv_obj_t *ar1 = np_row(mk);
    lv_obj_set_flex_align(ar1,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(ar1, 6, 0);
    lv_obj_t *btc_name = np_label(ar1, "Bitcoin", NP_F_SM_BOLD, lv_color_hex(0x5A6478));
    lv_obj_set_style_flex_grow(btc_name, 1, 0);
    g_market_btc = np_label(ar1, "$ --", NP_F_3XL, NP_C_TEXT);
    lv_obj_set_style_margin_right(g_market_btc, 6, 0);
    g_market_change_icon = np_label(ar1, "", NP_F_ICON_LG, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_right(g_market_change_icon, 4, 0);
    g_market_change = np_label(ar1, "--", NP_F_LG, NP_C_TEXT_MUTED);

    lv_obj_t *ar2 = np_row(mk);
    lv_obj_set_flex_align(ar2,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(ar2, 6, 0);
    lv_obj_t *fx_name = np_label(ar2, "Dólar", NP_F_SM_BOLD, lv_color_hex(0x5A6478));
    lv_obj_set_style_flex_grow(fx_name, 1, 0);
    g_market_fx = np_label(ar2, "R$ --", NP_F_LG, NP_C_TEXT);
}

}  // namespace nova
