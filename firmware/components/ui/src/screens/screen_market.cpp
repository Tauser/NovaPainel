#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>

namespace nova {

namespace {

static lv_obj_t* g_market_price = nullptr;
static lv_obj_t* g_market_change = nullptr;
static lv_obj_t* g_market_ohlc_open = nullptr;
static lv_obj_t* g_market_ohlc_high = nullptr;
static lv_obj_t* g_market_ohlc_low = nullptr;
static lv_obj_t* g_market_ohlc_volume = nullptr;
static lv_obj_t* g_market_fx_price = nullptr;
static lv_obj_t* g_market_fx_change = nullptr;
static lv_obj_t* g_market_fx_high = nullptr;
static lv_obj_t* g_market_fx_low = nullptr;
static lv_obj_t* g_market_meta_btc = nullptr;
static lv_obj_t* g_market_meta_fx = nullptr;
static lv_obj_t* g_market_meta_state = nullptr;

static const struct {
    int32_t o;
    int32_t c;
    int32_t h;
    int32_t l;
} kVisualCandles[] = {
    {97200, 97800, 98300, 96900},   {97800, 97300, 98100, 97000},
    {97300, 98600, 99100, 97200},   {98600, 98100, 99000, 97900},
    {98100, 99200, 99500, 98000},   {99200, 98800, 99600, 98500},
    {98800, 100100, 100400, 98700}, {100100, 99600, 100500, 99400},
    {99600, 100900, 101200, 99500}, {100900, 100300, 101300, 100200},
    {100300, 101800, 102000, 100100}, {101800, 103100, 103400, 101700},
    {103100, 102500, 103800, 102300}, {102500, 103842, 104521, 100841},
};

const char* source_name(DataSource source)
{
    switch (source) {
        case DataSource::Live: return "live";
        case DataSource::Cache: return "cache";
        case DataSource::Mock: return "mock";
    }
    return "mock";
}

void set_text_if_changed(lv_obj_t* label, const char* text)
{
    if (label) {
        lv_label_set_text(label, text);
    }
}

void set_color_if_changed(lv_obj_t* label, lv_color_t color)
{
    if (label) {
        lv_obj_set_style_text_color(label, color, 0);
    }
}

void format_usd_price(char* out, size_t out_size, double value)
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

void draw_visual_candles(lv_event_t* e)
{
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));

    lv_area_t obj_area;
    lv_obj_get_coords(obj, &obj_area);

    const int32_t width = lv_area_get_width(&obj_area);
    const int32_t height = lv_area_get_height(&obj_area);
    const int32_t lo = 96000;
    const int32_t hi = 105000;
    const int32_t range = hi - lo;
    const int32_t candle_count = static_cast<int32_t>(sizeof(kVisualCandles) / sizeof(kVisualCandles[0]));
    const int32_t cell_w = width / candle_count;
    const int32_t gap = cell_w / 4;
    int32_t body_w = cell_w - (gap * 2);
    if (body_w < 2) {
        body_w = 2;
    }

    for (int32_t i = 0; i < candle_count; ++i) {
        const auto& candle = kVisualCandles[i];
        const bool bullish = candle.c >= candle.o;
        const lv_color_t color = bullish ? NP_C_GREEN : NP_C_RED;

        const int32_t x0 = obj_area.x1 + (i * cell_w) + gap;
        const int32_t center_x = x0 + (body_w / 2);
        const int32_t y_high = obj_area.y1 + height - static_cast<int32_t>((int64_t)(candle.h - lo) * height / range);
        const int32_t y_low = obj_area.y1 + height - static_cast<int32_t>((int64_t)(candle.l - lo) * height / range);
        const int32_t y_open = obj_area.y1 + height - static_cast<int32_t>((int64_t)(candle.o - lo) * height / range);
        const int32_t y_close = obj_area.y1 + height - static_cast<int32_t>((int64_t)(candle.c - lo) * height / range);

        lv_draw_line_dsc_t wick;
        lv_draw_line_dsc_init(&wick);
        wick.color = color;
        wick.width = 1;
        wick.opa = LV_OPA_COVER;
        wick.p1.x = center_x;
        wick.p1.y = y_high;
        wick.p2.x = center_x;
        wick.p2.y = y_low;
        lv_draw_line(layer, &wick);

        lv_draw_rect_dsc_t body;
        lv_draw_rect_dsc_init(&body);
        body.bg_color = color;
        body.bg_opa = LV_OPA_COVER;
        body.radius = 1;

        lv_area_t area;
        area.x1 = x0;
        area.y1 = LV_MIN(y_open, y_close);
        area.x2 = x0 + body_w;
        area.y2 = LV_MAX(y_open, y_close);
        if (area.y1 == area.y2) {
            area.y2 = area.y1 + 1;
        }
        lv_draw_rect(layer, &body, &area);
    }
}

}  // namespace

void np_update_market(const AppState& state)
{
    if (!g_market_price || !g_market_change || !g_market_fx_price) {
        return;
    }

    if (state.crypto.valid) {
        char price_text[64];
        format_usd_price(price_text, sizeof(price_text), state.crypto.btc_usd);
        set_text_if_changed(g_market_price, price_text);

        char change_text[32];
        std::snprintf(change_text, sizeof(change_text), "%s %.2f%%",
                      state.crypto.btc_change_24h >= 0.0 ? "+" : "-",
                      state.crypto.btc_change_24h);
        set_text_if_changed(g_market_change, change_text);
        set_color_if_changed(g_market_change,
                             state.crypto.btc_change_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);
    } else {
        set_text_if_changed(g_market_price, "US$ --");
        set_text_if_changed(g_market_change, "--");
        set_color_if_changed(g_market_change, NP_C_TEXT_MUTED);
    }

    // O layout do export prevê OHLC/volume, mas o estado atual do MVP ainda
    // não expõe esse domínio. Mantemos a área visual sem inventar dados.
    set_text_if_changed(g_market_ohlc_open, "--");
    set_text_if_changed(g_market_ohlc_high, "--");
    set_text_if_changed(g_market_ohlc_low, "--");
    set_text_if_changed(g_market_ohlc_volume, "--");

    if (state.forex.usd_brl_valid) {
        char fx_text[32];
        std::snprintf(fx_text, sizeof(fx_text), "R$ %.2f", state.forex.usd_brl);
        set_text_if_changed(g_market_fx_price, fx_text);
        set_text_if_changed(g_market_fx_change,
                            state.forex.usd_brl_stale ? "cache offline" : "cotacao pronta");
        set_color_if_changed(g_market_fx_change,
                             state.forex.usd_brl_stale ? NP_C_ACCENT : NP_C_GREEN);
    } else {
        set_text_if_changed(g_market_fx_price, "R$ --");
        set_text_if_changed(g_market_fx_change, "--");
        set_color_if_changed(g_market_fx_change, NP_C_TEXT_MUTED);
    }

    set_text_if_changed(g_market_fx_high, "--");
    set_text_if_changed(g_market_fx_low, "--");

    if (g_market_meta_btc) {
        char meta_btc[32];
        std::snprintf(meta_btc, sizeof(meta_btc), "%s", source_name(state.crypto.source));
        set_text_if_changed(g_market_meta_btc, meta_btc);
    }
    if (g_market_meta_fx) {
        char meta_fx[32];
        std::snprintf(meta_fx, sizeof(meta_fx), "%s", source_name(state.forex.usd_brl_source));
        set_text_if_changed(g_market_meta_fx, meta_fx);
    }
    if (g_market_meta_state) {
        const bool has_live = state.crypto.valid || state.forex.usd_brl_valid;
        const bool stale = state.crypto.stale || state.forex.usd_brl_stale;
        const char* label = !has_live ? "aguardando dados" : (stale ? "modo cache" : "online");
        set_text_if_changed(g_market_meta_state, label);
        set_color_if_changed(g_market_meta_state,
                             !has_live ? NP_C_TEXT_MUTED : (stale ? NP_C_ACCENT : NP_C_GREEN));
    }
}

void np_screen_market(lv_obj_t* parent)
{
    const auto screen_index = static_cast<std::size_t>(ScreenId::Market);

    lv_obj_t* scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[screen_index] = scr;

    lv_obj_t* left = np_card(scr);
    lv_obj_set_style_flex_grow(left, 1, 0);
    lv_obj_set_height(left, lv_pct(100));
    lv_obj_set_style_pad_all(left, 16, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* header = np_row(left);
    lv_obj_set_flex_align(header,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(header, 10, 0);
    lv_obj_set_style_margin_bottom(header, 12, 0);

    lv_obj_t* btc_badge = lv_obj_create(header);
    lv_obj_set_size(btc_badge, 24, 24);
    lv_obj_set_style_bg_color(btc_badge, lv_color_hex(0xF7931A), 0);
    lv_obj_set_style_radius(btc_badge, 12, 0);
    lv_obj_set_style_border_width(btc_badge, 0, 0);
    lv_obj_set_style_pad_all(btc_badge, 0, 0);
    lv_obj_set_scrollbar_mode(btc_badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* badge_label = np_label(btc_badge, "B", NP_F_XS, NP_C_DARK_FG);
    lv_obj_align(badge_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* market_title = np_label(header, "Bitcoin / BTC-USD", NP_F_SM, NP_C_TEXT);
    lv_label_set_long_mode(market_title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(market_title, 180);

    lv_obj_t* header_spacer = lv_obj_create(header);
    np_obj_clear_style(header_spacer);
    lv_obj_set_style_flex_grow(header_spacer, 1, 0);

    lv_obj_t* period = lv_obj_create(header);
    lv_obj_set_size(period, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(period, lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_bg_opa(period, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(period, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(period, 1, 0);
    lv_obj_set_style_radius(period, 8, 0);
    lv_obj_set_style_pad_all(period, 3, 0);
    lv_obj_set_flex_flow(period, LV_FLEX_FLOW_ROW);

    static const char* periods[] = {"1h", "1D", "1S", "1M"};
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* button = lv_obj_create(period);
        lv_obj_set_size(button, LV_SIZE_CONTENT, lv_pct(100));
        lv_obj_set_style_bg_color(button,
                                  i == 1 ? lv_color_hex(0x252A3C) : lv_color_hex(0x1B1E2D), 0);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(button, 0, 0);
        lv_obj_set_style_radius(button, 6, 0);
        lv_obj_set_style_pad_hor(button, 10, 0);
        lv_obj_set_style_pad_ver(button, 0, 0);
        lv_obj_set_scrollbar_mode(button, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t* label = np_label(button, periods[i], NP_F_XS,
                                   i == 1 ? NP_C_TEXT : NP_C_TEXT_MUTED);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_t* price_row = np_row(left);
    lv_obj_set_flex_align(price_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(price_row, 20, 0);
    lv_obj_set_style_border_side(price_row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(price_row, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(price_row, 1, 0);
    lv_obj_set_style_pad_bottom(price_row, 12, 0);
    lv_obj_set_style_margin_bottom(price_row, 12, 0);

    lv_obj_t* price_info = np_col(price_row);
    lv_obj_set_size(price_info, 220, LV_SIZE_CONTENT);
    g_market_price = np_label(price_info, "US$ --", NP_F_HERO, NP_C_TEXT);
    np_label(price_info, "spot", NP_F_XS, lv_color_hex(0x5A6478));
    g_market_change = np_label(price_row, "--", NP_F_LG, NP_C_TEXT_MUTED);

    lv_obj_t* price_spacer = lv_obj_create(price_row);
    np_obj_clear_style(price_spacer);
    lv_obj_set_style_flex_grow(price_spacer, 1, 0);

    lv_obj_t* ohlc = np_row(price_row);
    lv_obj_set_size(ohlc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(ohlc, 18, 0);

    const char* labels[] = {"Abertura", "Max", "Min", "Volume 24h"};
    lv_obj_t** values[] = {
        &g_market_ohlc_open,
        &g_market_ohlc_high,
        &g_market_ohlc_low,
        &g_market_ohlc_volume,
    };
    const lv_color_t colors[] = {NP_C_TEXT, NP_C_GREEN, NP_C_RED, NP_C_TEXT};

    for (int i = 0; i < 4; ++i) {
        lv_obj_t* col = np_col(ohlc);
        lv_obj_set_size(col, 72, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(col,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(col, 3, 0);
        lv_obj_t* key = np_label(col, labels[i], NP_F_XS, NP_C_TEXT_MUTED);
        lv_label_set_long_mode(key, LV_LABEL_LONG_DOT);
        lv_obj_set_width(key, 72);
        *values[i] = np_label(col, "--", NP_F_SM, colors[i]);
        lv_obj_set_width(*values[i], 72);
        lv_obj_set_style_text_align(*values[i], LV_TEXT_ALIGN_CENTER, 0);
    }

    lv_obj_t* chart_area = lv_obj_create(left);
    lv_obj_set_style_flex_grow(chart_area, 1, 0);
    lv_obj_set_size(chart_area, lv_pct(100), 0);
    np_obj_clear_style(chart_area);
    lv_obj_add_event_cb(chart_area, draw_visual_candles, LV_EVENT_DRAW_MAIN_END, nullptr);

    lv_obj_t* right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_size(right, 256, lv_pct(100));
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 12, 0);

    lv_obj_t* usd = np_card(right);
    lv_obj_set_size(usd, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(usd, 14, 0);
    np_label(usd, "DOLAR / USD-BRL", NP_F_XS, NP_C_TEXT_MUTED);
    g_market_fx_price = np_label(usd, "R$ --", NP_F_4XL, NP_C_TEXT);
    lv_obj_set_style_margin_top(g_market_fx_price, 8, 0);
    g_market_fx_change = np_label(usd, "--", NP_F_SM, NP_C_TEXT_MUTED);
    np_hsep(usd);

    lv_obj_t* fx_row_1 = np_row(usd);
    lv_obj_set_flex_align(fx_row_1,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(fx_row_1, 10, 0);
    np_label(fx_row_1, "Max 24h", NP_F_SM, lv_color_hex(0x5A6478));
    g_market_fx_high = np_label(fx_row_1, "--", NP_F_SM, NP_C_TEXT);

    lv_obj_t* fx_row_2 = np_row(usd);
    lv_obj_set_flex_align(fx_row_2,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(fx_row_2, 4, 0);
    np_label(fx_row_2, "Min 24h", NP_F_SM, lv_color_hex(0x5A6478));
    g_market_fx_low = np_label(fx_row_2, "--", NP_F_SM, NP_C_TEXT);

    lv_obj_t* ibov = np_card(right);
    lv_obj_set_size(ibov, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(ibov, 14, 0);
    np_label(ibov, "IBOVESPA", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_t* ibov_value = np_label(ibov, "--", NP_F_3XL, NP_C_TEXT);
    lv_obj_set_style_margin_top(ibov_value, 8, 0);
    np_label(ibov, "fora do escopo do MVP atual", NP_F_SM, NP_C_TEXT_MUTED);

    lv_obj_t* meta = np_card(right);
    lv_obj_set_size(meta, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(meta, 1, 0);
    lv_obj_set_style_pad_all(meta, 14, 0);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_COLUMN);

    np_label(meta, "FONTE DOS DADOS", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t* meta_row_1 = np_row(meta);
    lv_obj_set_flex_align(meta_row_1,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(meta_row_1, 7, 0);
    np_label(meta_row_1, "BTC", NP_F_SM, lv_color_hex(0x5A6478));
    g_market_meta_btc = np_label(meta_row_1, "BTC mock", NP_F_SM, NP_C_TEXT);
    lv_label_set_long_mode(g_market_meta_btc, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_market_meta_btc, 92);
    lv_obj_set_style_text_align(g_market_meta_btc, LV_TEXT_ALIGN_RIGHT, 0);

    lv_obj_t* meta_row_2 = np_row(meta);
    lv_obj_set_flex_align(meta_row_2,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(meta_row_2, 7, 0);
    np_label(meta_row_2, "USD/BRL", NP_F_SM, lv_color_hex(0x5A6478));
    g_market_meta_fx = np_label(meta_row_2, "USD/BRL mock", NP_F_SM, NP_C_TEXT);
    lv_label_set_long_mode(g_market_meta_fx, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_market_meta_fx, 92);
    lv_obj_set_style_text_align(g_market_meta_fx, LV_TEXT_ALIGN_RIGHT, 0);

    lv_obj_t* meta_row_3 = np_row(meta);
    lv_obj_set_flex_align(meta_row_3,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(meta_row_3, 7, 0);
    np_label(meta_row_3, "Estado", NP_F_SM, lv_color_hex(0x5A6478));
    g_market_meta_state = np_label(meta_row_3, "aguardando dados", NP_F_SM, NP_C_TEXT_MUTED);
    lv_label_set_long_mode(g_market_meta_state, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_market_meta_state, 92);
    lv_obj_set_style_text_align(g_market_meta_state, LV_TEXT_ALIGN_RIGHT, 0);

    np_hsep(meta);
    np_label(meta, "FAVORITOS", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t* favorites = lv_obj_create(meta);
    lv_obj_set_size(favorites, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(favorites, 1, 0);
    lv_obj_set_style_bg_opa(favorites, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(favorites, lv_color_hex(0x2C2700), 0);
    lv_obj_set_style_border_width(favorites, 1, 0);
    lv_obj_set_style_radius(favorites, 9, 0);
    lv_obj_set_style_margin_top(favorites, 9, 0);
    lv_obj_set_flex_flow(favorites, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(favorites,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(favorites, 7, 0);
    lv_obj_set_scrollbar_mode(favorites, LV_SCROLLBAR_MODE_OFF);
    np_label(favorites, "+", NP_F_LG, NP_C_ACCENT);
    np_label(favorites, "Adicionar favorito", NP_F_SM, NP_C_ACCENT);
}

}  // namespace nova
