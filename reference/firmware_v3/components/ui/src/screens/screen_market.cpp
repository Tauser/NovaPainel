#include "novapanel_ui.hpp"

#include <cstdio>
#include <cstring>

namespace nova {

namespace {

static lv_obj_t* g_market_price = nullptr;
static lv_obj_t* g_market_currency = nullptr;
static lv_obj_t* g_market_change_icon = nullptr;
static lv_obj_t* g_market_change = nullptr;
static lv_obj_t* g_market_sync_icon  = nullptr;  // ícone ⟳ reflete estado live/cache/sem dados
static lv_obj_t* g_market_ohlc_open = nullptr;
static lv_obj_t* g_market_ohlc_high = nullptr;
static lv_obj_t* g_market_ohlc_low = nullptr;
static lv_obj_t* g_market_ohlc_volume = nullptr;
static lv_obj_t* g_market_fx_price = nullptr;
static lv_obj_t* g_market_fx_change_icon = nullptr;
static lv_obj_t* g_market_fx_change = nullptr;
static lv_obj_t* g_market_fx_high = nullptr;
static lv_obj_t* g_market_fx_low = nullptr;
static lv_obj_t* g_market_ibov_value = nullptr;
static lv_obj_t* g_market_ibov_change_icon = nullptr;
static lv_obj_t* g_market_ibov_change = nullptr;
static lv_obj_t* g_market_chart = nullptr;   // chart_card — for invalidation on data update
static OhlcSeries g_btc_ohlc{};              // copy updated in np_update_market()

// Period selector state
static lv_obj_t*  g_period_btns[4]{};        // button containers
static lv_obj_t*  g_period_lbls[4]{};        // text labels inside buttons
static int        g_active_period = 2;        // 0=1H 1=4H 2=1D 3=1S (default 1D)
static OhlcPeriodFn g_ohlc_period_fn;        // bound by np_bind_ohlc_period()

// Fallback candles — used while real OHLC data hasn't arrived yet.
static const struct { int32_t o; int32_t c; int32_t h; int32_t l; int32_t v; }
kCandles[] = {
    {97200,97800,98300,96900,18},{97800,97300,98100,97000,20},
    {97300,98600,99100,97200,17},{98600,98100,99000,97900,15},
    {98100,99200,99500,98000,19},{99200,98800,99600,98500,13},
    {98800,100100,100400,98700,24},{100100,99600,100500,99400,16},
    {99600,100900,101200,99500,20},{100900,100300,101300,100200,14},
    {100300,101800,102000,100100,18},{101800,103100,103400,101700,26},
    {103100,102500,103800,102300,21},{102500,103842,104521,100841,17},
};


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

void format_usd_price(char* out, size_t out_size, double value)
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

void format_compact_billion(char* out, size_t out_size, double value)
{
    if (value >= 1000000000.0) {
        std::snprintf(out, out_size, "%.1f bi", value / 1000000000.0);
    } else if (value >= 1000000.0) {
        std::snprintf(out, out_size, "%.1f mi", value / 1000000.0);
    } else {
        std::snprintf(out, out_size, "%.0f", value);
    }
}

// Ícone ⟳: verde=live, âmbar=cache, muted=sem dados — mesmo padrão das outras telas.
void set_sync_icon_state(lv_obj_t* icon, bool has_data, bool stale)
{
    if (!icon) return;
    const lv_color_t col = !has_data ? NP_C_TEXT_MUTED
                         : stale     ? NP_C_ACCENT
                                     : NP_C_GREEN;
    lv_obj_set_style_text_color(icon, col, 0);
}

// Mapeamento índice → OhlcPeriod
static constexpr OhlcPeriod kPeriodMap[4] = {
    OhlcPeriod::H1, OhlcPeriod::H4, OhlcPeriod::D1, OhlcPeriod::W1
};

static void on_period_btn(lv_event_t* e)
{
    const int idx = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    if (idx == g_active_period) return;

    // Desativa botão antigo
    if (g_period_btns[g_active_period]) {
        lv_obj_set_style_bg_color(g_period_btns[g_active_period], lv_color_hex(0x111420), 0);
        lv_obj_set_style_border_color(g_period_btns[g_active_period], NP_C_BORDER, 0);
    }
    if (g_period_lbls[g_active_period]) {
        lv_obj_set_style_text_color(g_period_lbls[g_active_period], NP_C_TEXT_MUTED, 0);
    }
    // Ativa novo
    g_active_period = idx;
    if (g_period_btns[idx]) {
        lv_obj_set_style_bg_color(g_period_btns[idx], lv_color_hex(0x252A3C), 0);
        lv_obj_set_style_border_color(g_period_btns[idx], NP_C_ACCENT_BD, 0);
    }
    if (g_period_lbls[idx]) {
        lv_obj_set_style_text_color(g_period_lbls[idx], NP_C_TEXT, 0);
    }
    if (g_ohlc_period_fn) {
        g_ohlc_period_fn(kPeriodMap[idx]);
    }
}

void draw_candles(lv_event_t* e)
{
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_area_t obj_area;
    lv_obj_get_coords(obj, &obj_area);

    const int32_t w = lv_area_get_width(&obj_area);
    const int32_t h = lv_area_get_height(&obj_area);

    // — Determine data source: real OHLC or static fallback —
    static constexpr int kFallbackN = static_cast<int>(sizeof(kCandles) / sizeof(kCandles[0]));
    const bool use_real  = g_btc_ohlc.valid && !g_btc_ohlc.candles.empty();
    const int  n_candles = use_real ? static_cast<int>(g_btc_ohlc.candles.size()) : kFallbackN;
    if (n_candles == 0 || w <= 0 || h <= 0) return;

    // — Price range: dynamic from data with ~4% headroom —
    int32_t lo, hi;
    if (use_real) {
        lo = static_cast<int32_t>(g_btc_ohlc.candles[0].l);
        hi = static_cast<int32_t>(g_btc_ohlc.candles[0].h);
        for (int i = 1; i < n_candles; ++i) {
            const int32_t cl = static_cast<int32_t>(g_btc_ohlc.candles[i].l);
            const int32_t ch = static_cast<int32_t>(g_btc_ohlc.candles[i].h);
            if (cl < lo) lo = cl;
            if (ch > hi) hi = ch;
        }
        const int32_t pad = LV_MAX((hi - lo) / 15, 1);
        lo = LV_MAX(lo - pad, 0);
        hi = hi + pad;
    } else {
        lo = 96000;
        hi = 105000;
    }
    const int32_t range = hi - lo;
    if (range <= 0) return;

    // — Layout constants —
    // kChartLeft/kChartRight reservam espaço nas bordas: as velas nunca ficam
    // cortadas na esquerda, e os labels do eixo Y têm espaço fixo na direita.
    static constexpr int32_t kChartLeft  = 8;   // margem esquerda antes da 1ª vela
    static constexpr int32_t kChartRight = 46;  // espaço direito para labels do eixo Y
    const int32_t chart_w    = w - kChartLeft - kChartRight;
    const int32_t cw         = chart_w / n_candles;
    const int32_t gap        = LV_MAX(cw / 4, 1);
    const int32_t top_pad    = 8;
    const int32_t bottom_pad = 52;
    const int32_t chart_h    = h - top_pad - bottom_pad;
    int32_t bw = cw - gap * 2;
    if (bw < 1) bw = 1;
    if (cw <= 0) return;

    // — Draw descriptor setup —
    lv_draw_line_dsc_t grid;
    lv_draw_line_dsc_init(&grid);
    grid.color = lv_color_hex(0x1D2232);
    grid.width = 1;
    grid.opa   = LV_OPA_COVER;

    lv_draw_label_dsc_t axis;
    lv_draw_label_dsc_init(&axis);
    axis.color      = lv_color_hex(0x4C5570);
    axis.font       = NP_F_XS;
    axis.text_local = false;

    // — Horizontal grid lines + price axis labels (5 levels) —
    // Formato compacto sem casas decimais para >=10k → evita quebra de linha.
    // Área de label dimensionada para kChartRight-6 px (nunca corta o texto).
    static char glabels[5][12];
    for (int i = 0; i < 5; ++i) {
        const int32_t val = lo + (range * i) / 4;
        const int32_t y   = obj_area.y1 + top_pad + chart_h
                            - static_cast<int32_t>((int64_t)(val - lo) * chart_h / range);

        grid.p1 = lv_point_precise_t{ .x = obj_area.x1 + kChartLeft, .y = y };
        grid.p2 = lv_point_precise_t{ .x = obj_area.x2 - kChartRight, .y = y };
        lv_draw_line(layer, &grid);

        if (val >= 1000000) {
            std::snprintf(glabels[i], sizeof(glabels[i]), "%.1fM", val / 1000000.0f);
        } else if (val >= 10000) {
            std::snprintf(glabels[i], sizeof(glabels[i]), "%.0fk", val / 1000.0f);  // "105k", sem decimal
        } else if (val >= 1000) {
            std::snprintf(glabels[i], sizeof(glabels[i]), "%.1fk", val / 1000.0f);  // "9.5k"
        } else {
            std::snprintf(glabels[i], sizeof(glabels[i]), "%d", static_cast<int>(val));
        }

        lv_area_t txt = {
            .x1 = obj_area.x2 - kChartRight + 4, .y1 = y - 9,
            .x2 = obj_area.x2 - 2,               .y2 = y + 9,
        };
        axis.text = glabels[i];
        lv_draw_label(layer, &axis, &txt);
    }

    // — MA21 (Média Móvel Simples 21 períodos) —
    // Desenhada antes das velas para ficar atrás delas.
    if (use_real && n_candles >= 21) {
        lv_draw_line_dsc_t ma_dsc;
        lv_draw_line_dsc_init(&ma_dsc);
        ma_dsc.color = lv_color_hex(0xF5AC37);  // dourado
        ma_dsc.width = 1;
        ma_dsc.opa   = LV_OPA_70;

        int32_t px = -1, py = -1;
        for (int i = 20; i < n_candles; ++i) {
            float sum = 0.0f;
            for (int j = i - 20; j <= i; ++j) sum += g_btc_ohlc.candles[j].c;
            const int32_t ma_price = static_cast<int32_t>(sum / 21.0f);
            const int32_t x = obj_area.x1 + kChartLeft + i * cw + gap + bw / 2;
            const int32_t y = obj_area.y1 + top_pad + chart_h
                             - static_cast<int32_t>((int64_t)(ma_price - lo) * chart_h / range);
            if (px >= 0) {
                ma_dsc.p1 = lv_point_precise_t{ .x = px, .y = py };
                ma_dsc.p2 = lv_point_precise_t{ .x = x,  .y = y  };
                lv_draw_line(layer, &ma_dsc);
            }
            px = x; py = y;
        }
    }

    // — Volume max (only relevant when v > 0) —
    int32_t max_volume = 1;
    if (use_real) {
        for (int i = 0; i < n_candles; ++i) {
            const int32_t vi = static_cast<int32_t>(g_btc_ohlc.candles[i].v);
            if (vi > max_volume) max_volume = vi;
        }
    } else {
        for (int i = 0; i < kFallbackN; ++i) {
            if (kCandles[i].v > max_volume) max_volume = kCandles[i].v;
        }
    }
    const bool has_volume = (max_volume > 1);

    // — Candles —
    for (int i = 0; i < n_candles; ++i) {
        int32_t co, cc, ch_v, cl_v, cv;
        if (use_real) {
            co   = static_cast<int32_t>(g_btc_ohlc.candles[i].o);
            cc   = static_cast<int32_t>(g_btc_ohlc.candles[i].c);
            ch_v = static_cast<int32_t>(g_btc_ohlc.candles[i].h);
            cl_v = static_cast<int32_t>(g_btc_ohlc.candles[i].l);
            cv   = static_cast<int32_t>(g_btc_ohlc.candles[i].v);
        } else {
            co   = kCandles[i].o;
            cc   = kCandles[i].c;
            ch_v = kCandles[i].h;
            cl_v = kCandles[i].l;
            cv   = kCandles[i].v;
        }

        const bool       bull = cc >= co;
        const lv_color_t col  = bull ? NP_C_GREEN : NP_C_RED;

        const int32_t x0 = obj_area.x1 + kChartLeft + i * cw + gap;
        const int32_t cx = x0 + bw / 2;
        const int32_t yH = obj_area.y1 + top_pad + chart_h - static_cast<int32_t>((int64_t)(ch_v - lo) * chart_h / range);
        const int32_t yL = obj_area.y1 + top_pad + chart_h - static_cast<int32_t>((int64_t)(cl_v - lo) * chart_h / range);
        const int32_t yO = obj_area.y1 + top_pad + chart_h - static_cast<int32_t>((int64_t)(co  - lo) * chart_h / range);
        const int32_t yC = obj_area.y1 + top_pad + chart_h - static_cast<int32_t>((int64_t)(cc  - lo) * chart_h / range);

        // Wick
        lv_draw_line_dsc_t wick;
        lv_draw_line_dsc_init(&wick);
        wick.color = col;
        wick.width = 1;
        wick.opa   = LV_OPA_COVER;
        wick.p1 = lv_point_precise_t{ .x = cx, .y = yH };
        wick.p2 = lv_point_precise_t{ .x = cx, .y = yL };
        lv_draw_line(layer, &wick);

        // Body
        lv_draw_rect_dsc_t body;
        lv_draw_rect_dsc_init(&body);
        body.bg_color = col;
        body.bg_opa   = LV_OPA_COVER;
        body.radius   = 1;
        lv_area_t ba = {
            .x1 = x0,          .y1 = LV_MIN(yO, yC),
            .x2 = x0 + bw,     .y2 = LV_MAX(yO, yC),
        };
        if (ba.y1 == ba.y2) ba.y2 = ba.y1 + 1;
        lv_draw_rect(layer, &body, &ba);

        // Volume bar (only when volume data is available)
        if (has_volume && cv > 0) {
            lv_draw_rect_dsc_t vol;
            lv_draw_rect_dsc_init(&vol);
            vol.bg_color = col;
            vol.bg_opa   = LV_OPA_50;
            vol.radius   = 0;
            const int32_t vol_h = (cv * 46) / max_volume;
            lv_area_t va = {
                .x1 = x0,          .y1 = obj_area.y2 - 30 - vol_h,
                .x2 = x0 + bw,     .y2 = obj_area.y2 - 30,
            };
            lv_draw_rect(layer, &vol, &va);
        }
    }

    // — Price Line: linha tracejada horizontal no preço atual —
    if (use_real && !g_btc_ohlc.candles.empty()) {
        const int32_t cur_price = static_cast<int32_t>(g_btc_ohlc.candles.back().c);
        const int32_t price_y   = obj_area.y1 + top_pad + chart_h
                                - static_cast<int32_t>((int64_t)(cur_price - lo) * chart_h / range);

        lv_draw_line_dsc_t pl;
        lv_draw_line_dsc_init(&pl);
        pl.color      = lv_color_hex(0xFFFFFF);
        pl.width      = 1;
        pl.opa        = LV_OPA_50;
        pl.dash_width = 4;
        pl.dash_gap   = 4;
        pl.p1 = lv_point_precise_t{ .x = obj_area.x1 + kChartLeft,  .y = price_y };
        pl.p2 = lv_point_precise_t{ .x = obj_area.x2 - kChartRight, .y = price_y };
        lv_draw_line(layer, &pl);

        // Label do preço atual no eixo Y — mesmo formato compacto dos labels de grid.
        static char price_label[12];
        if (cur_price >= 1000000) {
            std::snprintf(price_label, sizeof(price_label), "%.1fM", cur_price / 1000000.0f);
        } else if (cur_price >= 10000) {
            std::snprintf(price_label, sizeof(price_label), "%.0fk", cur_price / 1000.0f);
        } else if (cur_price >= 1000) {
            std::snprintf(price_label, sizeof(price_label), "%.1fk", cur_price / 1000.0f);
        } else {
            std::snprintf(price_label, sizeof(price_label), "%d", static_cast<int>(cur_price));
        }
        lv_draw_label_dsc_t pl_lbl;
        lv_draw_label_dsc_init(&pl_lbl);
        pl_lbl.color      = lv_color_hex(0xFFFFFF);
        pl_lbl.font       = NP_F_XS;
        pl_lbl.text_local = false;
        pl_lbl.text       = price_label;
        lv_area_t pl_area = {
            .x1 = obj_area.x2 - kChartRight + 4, .y1 = price_y - 9,
            .x2 = obj_area.x2 - 2,               .y2 = price_y + 9,
        };
        lv_draw_label(layer, &pl_lbl, &pl_area);
    }

    // — X-axis time labels: show ~6 evenly-spaced ticks + "agora" at last —
    static char tlabels[8][8];
    const int tick_every = LV_MAX(n_candles / 6, 1);
    int  tick_drawn = 0;
    for (int i = 0; i < n_candles && tick_drawn < 8; ++i) {
        const bool is_last = (i == n_candles - 1);
        if (!is_last && (i % tick_every != 0)) continue;

        if (use_real) {
            if (is_last) {
                std::snprintf(tlabels[tick_drawn], sizeof(tlabels[0]), "agora");
            } else {
                const int64_t ts_s = g_btc_ohlc.candles[i].ts_ms / 1000LL;
                if (g_btc_ohlc.period == OhlcPeriod::D1 || g_btc_ohlc.period == OhlcPeriod::W1) {
                    // Mostrar DD/MM para períodos diários/semanais
                    const int64_t days_epoch = ts_s / 86400LL;
                    // Algoritmo de Tomohiko Sakamoto simplificado para mês/dia
                    int64_t z = days_epoch + 719468LL;
                    int64_t era = (z >= 0 ? z : z - 146096LL) / 146097LL;
                    int64_t doe = z - era * 146097LL;
                    int64_t yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
                    int64_t doy = doe - (365*yoe + yoe/4 - yoe/100);
                    int64_t mp  = (5*doy + 2) / 153;
                    int mday = static_cast<int>(doy - (153*mp+2)/5 + 1);
                    int mon  = static_cast<int>(mp + (mp < 10 ? 3 : -9));
                    std::snprintf(tlabels[tick_drawn], sizeof(tlabels[0]), "%02d/%02d", mday, mon);
                } else {
                    // H1 / H4: mostrar hora
                    const int hour = static_cast<int>((ts_s / 3600) % 24);
                    std::snprintf(tlabels[tick_drawn], sizeof(tlabels[0]), "%02dh", hour);
                }
            }
        } else {
            static const char* fallback_ticks[] = {"8h","9h","10h","12h","14h","16h","18h","agora"};
            const int fi = (tick_drawn < 8) ? tick_drawn : 7;
            std::snprintf(tlabels[tick_drawn], sizeof(tlabels[0]), "%s", fallback_ticks[fi]);
        }

        const int32_t tx = obj_area.x1 + kChartLeft + i * cw + gap - 4;
        lv_area_t txt = {
            .x1 = tx,      .y1 = obj_area.y2 - 20,
            .x2 = tx + 40, .y2 = obj_area.y2 - 4,
        };
        axis.text = tlabels[tick_drawn];
        lv_draw_label(layer, &axis, &txt);
        tick_drawn++;
    }
}

}  // namespace

void np_update_market(const AppState& state, const OhlcSeries& ohlc)
{
    if (!g_market_price || !g_market_change) {
        return;
    }

    if (state.crypto.valid) {
        char price[32];
        format_usd_price(price, sizeof(price), state.crypto.btc_usd);
        set_line(g_market_price, price);
        set_line(g_market_currency, "USD");

        char change[24];
        std::snprintf(change, sizeof(change), "%+.2f%%", state.crypto.btc_change_24h);
        set_line(g_market_change_icon,
                 state.crypto.btc_change_24h >= 0.0 ? NP_I_VALUE_UP : NP_I_VALUE_DOWN);
        set_line(g_market_change, change);
        set_color(g_market_change_icon, state.crypto.btc_change_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);
        set_color(g_market_change, state.crypto.btc_change_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);

        char open[24];
        char high[24];
        char low[24];
        char volume[24];
        format_usd_price(open, sizeof(open), state.crypto.btc_open_24h);
        format_usd_price(high, sizeof(high), state.crypto.btc_high_24h);
        format_usd_price(low, sizeof(low), state.crypto.btc_low_24h);
        format_compact_billion(volume, sizeof(volume), state.crypto.btc_volume_24h);
        set_line(g_market_ohlc_open, open);
        set_line(g_market_ohlc_high, high);
        set_line(g_market_ohlc_low, low);
        set_line(g_market_ohlc_volume, volume);
    } else {
        set_line(g_market_price, "$ --");
        set_line(g_market_currency, "USD");
        set_line(g_market_change_icon, "");
        set_line(g_market_change, "--");
        set_color(g_market_change_icon, NP_C_TEXT_MUTED);
        set_color(g_market_change, NP_C_TEXT_MUTED);
        set_line(g_market_ohlc_open, "--");
        set_line(g_market_ohlc_high, "--");
        set_line(g_market_ohlc_low, "--");
        set_line(g_market_ohlc_volume, "--");
    }

    if (state.forex.usd_brl_valid) {
        char fx_price[24];
        char fx_change[24];
        char fx_high[24];
        char fx_low[24];
        std::snprintf(fx_price, sizeof(fx_price), "R$ %.2f", state.forex.usd_brl);
        std::snprintf(fx_change, sizeof(fx_change), "%+.2f%% · 24h",
                      state.forex.usd_brl_change_pct_24h);
        std::snprintf(fx_high, sizeof(fx_high), "R$ %.2f", state.forex.usd_brl_high_24h);
        std::snprintf(fx_low, sizeof(fx_low), "R$ %.2f", state.forex.usd_brl_low_24h);
        set_line(g_market_fx_price, fx_price);
        set_line(g_market_fx_change_icon,
                 state.forex.usd_brl_change_pct_24h >= 0.0 ? NP_I_VALUE_UP : NP_I_VALUE_DOWN);
        set_line(g_market_fx_change, fx_change);
        set_color(g_market_fx_change_icon, state.forex.usd_brl_change_pct_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);
        set_color(g_market_fx_change, state.forex.usd_brl_change_pct_24h >= 0.0 ? NP_C_GREEN : NP_C_RED);
        set_line(g_market_fx_high, fx_high);
        set_line(g_market_fx_low, fx_low);
    } else {
        set_line(g_market_fx_price, "R$ --");
        set_line(g_market_fx_change_icon, "");
        set_line(g_market_fx_change, "--");
        set_color(g_market_fx_change_icon, NP_C_TEXT_MUTED);
        set_color(g_market_fx_change, NP_C_TEXT_MUTED);
        set_line(g_market_fx_high, "--");
        set_line(g_market_fx_low, "--");
    }

    set_line(g_market_ibov_value, "138.540");
    set_line(g_market_ibov_change_icon, NP_I_VALUE_DOWN);
    set_line(g_market_ibov_change, "0,60% · 24h");
    set_color(g_market_ibov_change_icon, NP_C_RED);
    set_color(g_market_ibov_change, NP_C_RED);

    const bool any   = state.crypto.valid || state.forex.usd_brl_valid;
    const bool stale = state.crypto.stale || state.forex.usd_brl_stale;
    set_sync_icon_state(g_market_sync_icon, any, stale);

    // Atualiza série OHLC quando dados novos chegam da API
    if (ohlc.valid && (ohlc.last_update_ms != g_btc_ohlc.last_update_ms ||
                       ohlc.period != g_btc_ohlc.period)) {
        g_btc_ohlc = ohlc;
        if (g_market_chart) lv_obj_invalidate(g_market_chart);
    }
    // "Vela em formação": atualiza o close/high/low da última vela com o preço
    // live a cada tick de preço. Isso reflete o movimento intracandle em tempo
    // real sem precisar de uma nova chamada à API de OHLC.
    // O chart é redesenhado somente se o preço realmente mudou.
    else if (g_btc_ohlc.valid && !g_btc_ohlc.candles.empty() && state.crypto.valid) {
        const float live = static_cast<float>(state.crypto.btc_usd);
        OhlcCandle& last = g_btc_ohlc.candles.back();
        if (live != last.c) {
            last.c = live;
            if (live > last.h) last.h = live;
            if (live < last.l) last.l = live;
            if (g_market_chart) lv_obj_invalidate(g_market_chart);
        }
    }
}

void np_screen_market(lv_obj_t *parent)
{
    const auto screen_index = static_cast<std::size_t>(ScreenId::Market);

    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 10, 0);
    np_screens[screen_index] = scr;

    lv_obj_t *top_row = lv_obj_create(scr);
    np_obj_clear_style(top_row);
    lv_obj_set_size(top_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(top_row, 12, 0);

    // — Coluna central: card de dados
    lv_obj_t *center = lv_obj_create(top_row);
    np_obj_clear_style(center);
    lv_obj_set_height(center, LV_SIZE_CONTENT);
    lv_obj_set_style_flex_grow(center, 1, 0);
    lv_obj_set_flex_flow(center, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(center,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(center, 10, 0);

    // ——— CARD DE DADOS ———
    // Construído sem flex-column para evitar gaps invisíveis do tema LVGL.
    // Altura calculada explicitamente: pad_top + header + sep_h + price_h + pad_bot.
    static constexpr int16_t kDcPadH   = 9;    // pad topo/base do card
    static constexpr int16_t kHdrH     = 32;   // altura do header row (aumentado para botões de período mais clicáveis)
    static constexpr int16_t kSepH     = 8;    // espaço entre header e preço
    static constexpr int16_t kPriceH   = 70;   // preço maior + OHLC confortável
    static constexpr int16_t kCardH    = kDcPadH*2 + kHdrH + kSepH + kPriceH;

    lv_obj_t *data_card = np_card(center);
    lv_obj_set_size(data_card, lv_pct(100), kCardH);
    lv_obj_clear_flag(data_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_hor(data_card, 16, 0);
    lv_obj_set_style_pad_top(data_card, kDcPadH, 0);
    lv_obj_set_style_pad_bottom(data_card, kDcPadH, 0);

    // — Header row (posicionado manualmente, y=0 relativo ao content area)
    lv_obj_t *lh = lv_obj_create(data_card);
    np_obj_clear_style(lh);
    lv_obj_set_size(lh, lv_pct(100), kHdrH);
    lv_obj_set_pos(lh, 0, 0);
    lv_obj_set_flex_flow(lh, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lh,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(lh, 8, 0);

    // badge BTC laranja
    lv_obj_t *btc_badge = lv_obj_create(lh);
    lv_obj_set_size(btc_badge, 22, 22);
    lv_obj_set_style_bg_color(btc_badge, lv_color_hex(0xF7931A), 0);
    lv_obj_set_style_radius(btc_badge, 11, 0);
    lv_obj_set_style_border_width(btc_badge, 0, 0);
    lv_obj_set_style_pad_all(btc_badge, 0, 0);
    lv_obj_set_scrollbar_mode(btc_badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *bl = np_label(btc_badge, "B", NP_F_SM_BOLD, NP_C_DARK_FG);
    lv_obj_align(bl, LV_ALIGN_CENTER, 0, 0);

    np_label(lh, "Bitcoin \xC2\xB7 BTC/USD", NP_F_SM_BOLD, NP_C_TEXT);

    // spacer flexível empurra tabs para a direita
    lv_obj_t *lh_sp = lv_obj_create(lh);
    np_obj_clear_style(lh_sp);
    lv_obj_set_style_flex_grow(lh_sp, 1, 0);

    // Pills de período — cada botão é separado, com borda própria e font bold.
    // Container transparente só para agrupar com gap.
    lv_obj_t *per = lv_obj_create(lh);
    np_obj_clear_style(per);
    lv_obj_set_size(per, LV_SIZE_CONTENT, kHdrH);
    lv_obj_set_flex_flow(per, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(per, 5, 0);  // gap entre pills

    static const char *kPeriods[] = { "1H", "4H", "1D", "1S" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *pb = lv_obj_create(per);
        np_obj_clear_style(pb);
        lv_obj_set_size(pb, LV_SIZE_CONTENT, kHdrH);
        const bool active = (i == g_active_period);
        lv_obj_set_style_bg_color(pb, active ? lv_color_hex(0x252A3C) : lv_color_hex(0x111420), 0);
        lv_obj_set_style_bg_opa(pb, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(pb, active ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
        lv_obj_set_style_border_width(pb, 1, 0);
        lv_obj_set_style_radius(pb, 7, 0);
        lv_obj_set_style_pad_hor(pb, 12, 0);
        lv_obj_set_style_pad_ver(pb, 0, 0);
        lv_obj_add_flag(pb, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(pb, on_period_btn, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_t *pl = np_label(pb, kPeriods[i], NP_F_XS_BOLD,
                                active ? NP_C_TEXT : NP_C_TEXT_MUTED);
        lv_obj_align(pl, LV_ALIGN_CENTER, 0, 0);
        g_period_btns[i] = pb;
        g_period_lbls[i] = pl;
    }

    // Ícone de sync: verde=live, âmbar=cache, muted=sem dados. Começa muted
    // até os dados chegarem; np_update_market aplica a cor correta.
    g_market_sync_icon = np_label(lh, NP_I_REFRESH, NP_F_ICON_SM, NP_C_TEXT_MUTED);
    lv_obj_set_style_pad_left(g_market_sync_icon, 4, 0);

    // — Linha de preço (posicionada logo abaixo do header)
    lv_obj_t *pr = lv_obj_create(data_card);
    np_obj_clear_style(pr);
    lv_obj_set_size(pr, lv_pct(100), kPriceH);
    lv_obj_set_pos(pr, 0, kHdrH + kSepH);
    lv_obj_set_flex_flow(pr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // coluna preço + moeda
    lv_obj_t *pinfo = lv_obj_create(pr);
    np_obj_clear_style(pinfo);
    lv_obj_set_size(pinfo, 210, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(pinfo, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pinfo,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(pinfo, 5, 0);
    g_market_price    = np_label(pinfo, "$ --", NP_F_HERO, NP_C_TEXT);

    // variação %
    lv_obj_t *change_row = lv_obj_create(pr);
    np_obj_clear_style(change_row);
    lv_obj_set_size(change_row, 110, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(change_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(change_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(change_row, 4, 0);
    g_market_change_icon = np_label(change_row, "", NP_F_ICON_LG, NP_C_TEXT_MUTED);
    g_market_change = np_label(change_row, "--", NP_F_LG, NP_C_TEXT_MUTED);

    // — Card do gráfico (cresce para preencher o espaço restante)
    lv_obj_t *chart_card = np_card(scr);
    g_market_chart = chart_card;  // stored for lv_obj_invalidate on OHLC update
    lv_obj_set_size(chart_card, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(chart_card, 1, 0);
    lv_obj_set_style_pad_all(chart_card, 0, 0);
    lv_obj_add_event_cb(chart_card, draw_candles, LV_EVENT_DRAW_MAIN_END, NULL);

    lv_obj_t *right = lv_obj_create(top_row);
    np_obj_clear_style(right);
    lv_obj_set_size(right, 256, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 8, 0);

    lv_obj_t *usd = np_card(right);
    lv_obj_set_size(usd, lv_pct(100), kCardH);
    lv_obj_set_style_pad_all(usd, 6, 0);
    lv_obj_set_flex_flow(usd, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(usd,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    //np_label(usd, "RESUMO DO MERCADO", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_pad_top(usd, 4, 0);
    lv_obj_set_style_pad_row(usd, 1, 0);  // espaçamento vertical entre as 4 linhas
    static const char *kOhlcLabels[] = { "Abertura", "M\xC3\xA1x", "M\xC3\xADn", "Volume 24h" };
    lv_obj_t **ohlc_values[] = {
        &g_market_ohlc_open, &g_market_ohlc_high, &g_market_ohlc_low, &g_market_ohlc_volume,
    };
    const lv_color_t ohlc_colors[] = { NP_C_TEXT, NP_C_GREEN, NP_C_RED, NP_C_TEXT };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = np_row(usd);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_top(row, 0, 0);
        np_label(row, kOhlcLabels[i], NP_F_XS, lv_color_hex(0x5A6478));
        *ohlc_values[i] = np_label(row, "--", NP_F_SM_BOLD, ohlc_colors[i]);
        lv_obj_set_width(*ohlc_values[i], LV_SIZE_CONTENT);
        lv_label_set_long_mode(*ohlc_values[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(*ohlc_values[i], LV_TEXT_ALIGN_RIGHT, 0);
    }
}

// Chamado por np_bind_ohlc_period() em novapanel_ui.cpp.
void np_screen_market_bind_period(nova::OhlcPeriodFn fn)
{
    nova::g_ohlc_period_fn = std::move(fn);
}

}  // namespace nova
