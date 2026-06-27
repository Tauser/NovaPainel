#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>

namespace nova {

namespace {

static lv_obj_t* g_clock_time = nullptr;
static lv_obj_t* g_clock_seconds = nullptr;
static lv_obj_t* g_clock_date = nullptr;
static lv_obj_t* g_clock_source = nullptr;
static lv_obj_t* g_weather_temp = nullptr;
static lv_obj_t* g_weather_desc = nullptr;
static lv_obj_t* g_weather_meta = nullptr;
static lv_obj_t* g_weather_source = nullptr;
static lv_obj_t* g_market_btc = nullptr;
static lv_obj_t* g_market_fx = nullptr;
static lv_obj_t* g_market_note = nullptr;
static lv_obj_t* g_market_change = nullptr;
static lv_obj_t* g_network_line = nullptr;
static lv_obj_t* g_system_line = nullptr;

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
        case WeatherCondition::Clear: return "Céu limpo";
        case WeatherCondition::Cloudy: return "Nublado";
        case WeatherCondition::Fog: return "Neblina";
        case WeatherCondition::Drizzle: return "Garoa";
        case WeatherCondition::Rain: return "Chuva";
        case WeatherCondition::Snow: return "Neve";
        case WeatherCondition::Thunderstorm: return "Tempestade";
        case WeatherCondition::Unknown: return "Clima indisponível";
    }
    return "Clima indisponível";
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

void format_source_label(char* out, size_t out_size, const char* prefix, DataSource source)
{
    std::snprintf(out, out_size, "%s: %s", prefix, source_name(source));
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

// Evita invalidar o widget quando a cor não mudou.
// lv_obj_set_style_text_color é sempre "dirty" mesmo sem alteração,
// o que força re-render do label a cada np_update_home().
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

void format_usd_whole(char* out, size_t out_size, double value)
{
    const unsigned long whole = static_cast<unsigned long>(value + 0.5);
    const unsigned long millions = whole / 1000000UL;
    const unsigned long thousands = (whole / 1000UL) % 1000UL;
    const unsigned long units = whole % 1000UL;

    if (millions > 0) {
        std::snprintf(out, out_size, "US$ %lu.%03lu.%03lu", millions, thousands, units);
    } else if (thousands > 0) {
        std::snprintf(out, out_size, "US$ %lu.%03lu", thousands, units);
    } else {
        std::snprintf(out, out_size, "US$ %lu", units);
    }
}

}  // namespace

void np_update_home(const AppState& state)
{
    if (!g_clock_time || !g_clock_seconds || !g_clock_date || !g_clock_source) {
        return;
    }

    const ClockState& clock = state.clock;

    char time_text[8];
    std::snprintf(time_text, sizeof(time_text), "%02d:%02d", clock.hour, clock.minute);
    set_line_if_changed(g_clock_time, time_text);

    char seconds_text[4];
    std::snprintf(seconds_text, sizeof(seconds_text), "%02d", clock.second);
    set_line_if_changed(g_clock_seconds, seconds_text);

    char date_text[64];
    std::snprintf(date_text, sizeof(date_text), "%s, %02d de %s",
                  weekday_name(clock.weekday), clock.day, month_name(clock.month));
    set_line_if_changed(g_clock_date, date_text);

    set_line_if_changed(g_clock_source, clock.synced ? "NTP sincronizado" : "Relogio local");
    set_color_if_changed(g_clock_source, clock.synced ? NP_C_GREEN : NP_C_ACCENT);

    if (g_weather_temp && g_weather_desc && g_weather_meta) {
        if (state.weather.valid) {
            char temp_text[16];
            std::snprintf(temp_text, sizeof(temp_text), "%.0f°",
                          state.weather.temperature_c);
            set_line_if_changed(g_weather_temp, temp_text);
            set_line_if_changed(g_weather_desc, weather_name(state.weather.condition));
            char meta_text[64];
            std::snprintf(meta_text, sizeof(meta_text), "Sensação %.0f°  |  %s",
                          state.weather.feels_like_c,
                          source_name(state.weather.source));
            set_line_if_changed(g_weather_meta, meta_text);
            char source_text[32];
            format_source_label(source_text, sizeof(source_text), "clima", state.weather.source);
            set_line_if_changed(g_weather_source, source_text);
        } else {
            set_line_if_changed(g_weather_temp, "--");
            set_line_if_changed(g_weather_desc, "Aguardando clima");
            set_line_if_changed(g_weather_meta, "Sem dados reais ainda | clima: mock");
            set_line_if_changed(g_weather_source, "clima: mock");
        }
    }

    if (g_market_btc && g_market_fx) {
        if (state.crypto.valid) {
            char btc_text[64];
            format_usd_whole(btc_text, sizeof(btc_text), state.crypto.btc_usd);
            set_line_if_changed(g_market_btc, btc_text);
            if (g_market_change) {
                char change_text[32];
                std::snprintf(change_text, sizeof(change_text), "%s %.2f%%",
                              state.crypto.btc_change_24h >= 0.0 ? LV_SYMBOL_UP : LV_SYMBOL_DOWN,
                              state.crypto.btc_change_24h);
                set_line_if_changed(g_market_change, change_text);
                set_color_if_changed(g_market_change,
                                     state.crypto.btc_change_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);
            }
        } else {
            set_line_if_changed(g_market_btc, "US$ --");
            set_line_if_changed(g_market_change, "aguardando");
            set_color_if_changed(g_market_change, NP_C_TEXT_MUTED);
        }

        if (state.forex.usd_brl_valid) {
            char fx_text[64];
            std::snprintf(fx_text, sizeof(fx_text), "R$ %.2f", state.forex.usd_brl);
            set_line_if_changed(g_market_fx, fx_text);
        } else {
            set_line_if_changed(g_market_fx, "R$ --");
        }

        if (g_market_note) {
            const bool has_market_data = state.crypto.valid || state.forex.usd_brl_valid;
            const bool stale = state.crypto.stale || state.forex.usd_brl_stale;
            const char* note = has_market_data
                ? (stale ? "dados do cache" : "dados ao vivo")
                : "aguardando dados";
            set_line_if_changed(g_market_note, note);
        }
    }

    if (g_network_line && g_system_line) {
        const bool connected = state.onboarding.wifi_status == WifiConnectStatus::Connected;
        char net_text[80];
        std::snprintf(net_text, sizeof(net_text), "Rede: %s  |  Wi-Fi: %s",
                      connected ? "online" : "offline",
                      connected ? "conectado" : "sem link");
        set_line_if_changed(g_network_line, net_text);

        char sys_text[96];
        std::snprintf(sys_text, sizeof(sys_text), "Board:%s Display:%s Touch:%s Cache:%s",
                      state.system.board_ready ? "ok" : "--",
                      state.system.display_ready ? "ok" : "--",
                      state.system.touch_ready ? "ok" : "--",
                      state.system.cache_ready ? "ok" : "--");
        set_line_if_changed(g_system_line, sys_text);
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
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[screen_index] = scr;

    lv_obj_t *left = np_card(scr);
    lv_obj_set_size(left, 362, lv_pct(100));
    lv_obj_set_style_pad_all(left, 16, 0);
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

    g_clock_date = np_label(left, "aguardando relogio",
        NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_margin_top(g_clock_date, 6, 0);

    g_clock_source = np_label(left, "Relogio local", NP_F_XS, NP_C_ACCENT);
    lv_obj_set_style_margin_top(g_clock_source, 4, 0);

    g_network_line = np_label(left, "Rede: offline  |  Wi-Fi: sem link", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(g_network_line, 6, 0);
    g_system_line = np_label(left, "Board:-- Display:-- Touch:-- Cache:--", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(g_system_line, 2, 0);

    lv_obj_t *sp = lv_obj_create(left);
    np_obj_clear_style(sp);
    lv_obj_set_size(sp, 1, 1);
    lv_obj_set_style_flex_grow(sp, 1, 0);

    lv_obj_t *hs = np_hsep(left);
    lv_obj_set_style_margin_bottom(hs, 10, 0);

    lv_obj_t *wx = lv_obj_create(left);
    lv_obj_set_size(wx, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wx, NP_C_CARD2, 0);
    lv_obj_set_style_bg_opa(wx, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wx, 0, 0);
    lv_obj_set_style_radius(wx, 10, 0);
    lv_obj_set_style_pad_all(wx, 12, 0);
    lv_obj_set_scrollbar_mode(wx, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wx, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *wr = np_row(wx);
    lv_obj_set_flex_align(wr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    g_weather_temp = np_label(wr, "--", NP_F_4XL, NP_C_TEXT);

    lv_obj_t *city_col = np_col(wr);
    lv_obj_set_size(city_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(city_col,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    np_label(city_col, "Brasilia", NP_F_SM, lv_color_hex(0xC4C8D6));
    g_weather_desc = np_label(city_col, "Aguardando clima", NP_F_XS, NP_C_TEXT_DIM);
    lv_obj_t *weather_meta = np_label(city_col, "Sem dados reais ainda", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(weather_meta, 2, 0);

    lv_obj_t *wsp = lv_obj_create(wx);
    lv_obj_set_size(wsp, lv_pct(100), 1);
    lv_obj_set_style_bg_color(wsp, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_bg_opa(wsp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wsp, 0, 0);
    lv_obj_set_style_radius(wsp, 0, 0);
    lv_obj_set_style_margin_top(wsp, 8, 0);
    lv_obj_set_style_margin_bottom(wsp, 8, 0);

    g_weather_meta = np_label(wx, "Sem dados reais ainda", NP_F_XS, NP_C_TEXT_MUTED);
    g_weather_source = np_label(wx, "mock", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(g_weather_source, 2, 0);

    lv_obj_t *right = lv_obj_create(scr);
    lv_obj_set_style_flex_grow(right, 1, 0);
    lv_obj_set_height(right, lv_pct(100));
    np_obj_clear_style(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(right, 10, 0);

    lv_obj_t *ag = np_card(right);
    lv_obj_set_size(ag, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(ag, 1, 0);
    lv_obj_set_style_pad_all(ag, 14, 0);
    lv_obj_set_flex_flow(ag, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ag,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *agh = np_row(ag);
    lv_obj_set_flex_align(agh,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(agh, 9, 0);
    lv_obj_set_style_margin_bottom(agh, 8, 0);
    np_label(agh, NP_I_LIST, NP_F_ICON, NP_C_ACCENT);
    lv_obj_t *ag_t = np_label(agh, "Agenda de hoje", NP_F_MD, NP_C_TEXT);
    lv_obj_set_style_flex_grow(ag_t, 1, 0);
    np_label(agh, "4 eventos", NP_F_XS, NP_C_TEXT_MUTED);
    np_chip(agh, "Ver tudo");
    np_chip(agh, "+ Adicionar");

    static const struct {
        const char *time;
        const char *title;
        const char *sub;
        uint32_t color;
    } evts[] = {
        { "15:30", "Reuniao - NoiseBot team", "Trabalho - 45 min", 0xE8A83C },
        { "17:00", "Academia", "Pessoal - 1h", 0x4ABB78 },
        { "19:30", "Jantar com Marina", "Pessoal - 2h", 0x4F7ECB },
        { "21:00", "Revisar firmware v1.4", "Projeto - 30 min", 0xB77ABB },
    };

    for (int i = 0; i < 3; ++i) {
        lv_obj_t *row = np_row(ag);
        lv_obj_set_style_pad_column(row, 11, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, NP_C_SEP, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 6, 0);

        lv_obj_t *tm = np_label(row, evts[i].time, NP_F_SM, NP_C_TEXT_MED);
        lv_obj_set_width(tm, 38);

        lv_obj_t *bar = lv_obj_create(row);
        lv_obj_set_size(bar, 3, 28);
        lv_obj_set_style_bg_color(bar, lv_color_hex(evts[i].color), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        lv_obj_t *ttl = np_label(info, evts[i].title, NP_F_SM, NP_C_TEXT);
        lv_label_set_long_mode(ttl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(ttl, 260);
        lv_obj_t *sub = np_label(info, evts[i].sub, NP_F_XS,
            lv_color_hex(0x5A6478));
        lv_obj_set_style_margin_top(sub, 1, 0);
    }

    lv_obj_t *br = lv_obj_create(right);
    np_obj_clear_style(br);
    lv_obj_set_size(br, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(br, 1, 0);
    lv_obj_set_flex_flow(br, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(br,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(br, 10, 0);

    lv_obj_t *pl = np_card(br);
    lv_obj_set_style_flex_grow(pl, 1, 0);
    lv_obj_set_height(pl, lv_pct(100));
    lv_obj_set_style_pad_all(pl, 11, 0);
    lv_obj_set_flex_flow(pl, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pl,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *ptop = np_row(pl);
    lv_obj_set_style_pad_column(ptop, 10, 0);

    lv_obj_t *cov = lv_obj_create(ptop);
    lv_obj_set_size(cov, 34, 34);
    lv_obj_set_style_bg_color(cov, NP_C_ACCENT, 0);
    lv_obj_set_style_radius(cov, 8, 0);
    lv_obj_set_style_border_width(cov, 0, 0);
    lv_obj_set_style_pad_all(cov, 0, 0);
    lv_obj_set_scrollbar_mode(cov, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *cov_ic = np_label(cov, NP_I_MUSIC, NP_F_ICON, NP_C_DARK_FG);
    lv_obj_align(cov_ic, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *si = np_col(ptop);
    lv_obj_set_style_flex_grow(si, 1, 0);
    lv_obj_set_size(si, 0, LV_SIZE_CONTENT);
    np_label(si, "Labirinto", NP_F_SM, NP_C_TEXT);
    np_label(si, "Tuyo - Pra Cima de Mim", NP_F_XS, lv_color_hex(0x5A6478));

    lv_obj_t *pb = lv_bar_create(pl);
    lv_obj_set_size(pb, lv_pct(100), 2);
    lv_bar_set_range(pb, 0, 100);
    lv_bar_set_value(pb, 17, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(pb, NP_C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(pb, NP_C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_radius(pb, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(pb, 1, LV_PART_INDICATOR);

    lv_obj_t *ctrl = np_row(pl);
    lv_obj_set_flex_align(ctrl,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(ctrl, 12, 0);
    np_label(ctrl, NP_I_CHEV_L, NP_F_ICON, NP_C_TEXT_MUTED);

    lv_obj_t *play = lv_button_create(ctrl);
    lv_obj_set_size(play, 30, 30);
    lv_obj_set_style_bg_color(play, NP_C_ACCENT, 0);
    lv_obj_set_style_radius(play, 15, 0);
    lv_obj_set_style_shadow_width(play, 0, 0);
    lv_obj_t *play_ic = np_label(play, NP_I_PAUSE, NP_F_ICON, NP_C_DARK_FG);
    lv_obj_align(play_ic, LV_ALIGN_CENTER, 0, 0);

    np_label(ctrl, NP_I_CHEV_R, NP_F_ICON, NP_C_TEXT_MUTED);

    lv_obj_t *mk = np_card(br);
    lv_obj_set_style_flex_grow(mk, 1, 0);
    lv_obj_set_height(mk, lv_pct(100));
    lv_obj_set_style_pad_all(mk, 11, 0);
    lv_obj_set_flex_flow(mk, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mk,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *mh = np_row(mk);
    lv_obj_set_flex_align(mh,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(mh, 7, 0);
    lv_obj_set_style_margin_bottom(mh, 8, 0);

    lv_obj_t *btc_badge = lv_obj_create(mh);
    lv_obj_set_size(btc_badge, 22, 22);
    lv_obj_set_style_bg_color(btc_badge, lv_color_hex(0xF7931A), 0);
    lv_obj_set_style_bg_opa(btc_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btc_badge, 0, 0);
    lv_obj_set_style_radius(btc_badge, 11, 0);
    lv_obj_set_style_pad_all(btc_badge, 0, 0);
    lv_obj_set_scrollbar_mode(btc_badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *btc_badge_label = np_label(btc_badge, "B", NP_F_XS, NP_C_DARK_FG);
    lv_obj_align(btc_badge_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *title_col = np_col(mh);
    lv_obj_set_style_flex_grow(title_col, 1, 0);
    lv_obj_set_size(title_col, 0, LV_SIZE_CONTENT);
    np_label(title_col, "Bitcoin - BTC/USD", NP_F_SM, NP_C_TEXT);

    lv_obj_t *period = lv_obj_create(mh);
    lv_obj_set_size(period, LV_SIZE_CONTENT, 24);
    lv_obj_set_style_bg_color(period, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_bg_opa(period, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(period, 0, 0);
    lv_obj_set_style_radius(period, 6, 0);
    lv_obj_set_style_pad_hor(period, 9, 0);
    lv_obj_set_style_pad_ver(period, 0, 0);
    lv_obj_set_scrollbar_mode(period, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *period_label = np_label(period, "1D", NP_F_XS, NP_C_TEXT);
    lv_obj_align(period_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *price_row = np_row(mk);
    lv_obj_set_flex_align(price_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(price_row, 8, 0);
    g_market_btc = np_label(price_row, "US$ --", NP_F_3XL, NP_C_TEXT);
    g_market_change = np_label(price_row, "aguardando", NP_F_XS, NP_C_TEXT_MUTED);

    g_market_note = np_label(mk, "aguardando dados", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(g_market_note, 2, 0);

    np_hsep(mk);

    lv_obj_t *fx_row = np_row(mk);
    lv_obj_set_flex_align(fx_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(fx_row, 8, 0);
    lv_obj_t *fx_col = np_col(fx_row);
    lv_obj_set_size(fx_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(fx_col, "DOLAR - USD/BRL", NP_F_XS, NP_C_TEXT_MUTED);
    g_market_fx = np_label(fx_row, "R$ --", NP_F_LG, NP_C_TEXT);
}

}  // namespace nova
