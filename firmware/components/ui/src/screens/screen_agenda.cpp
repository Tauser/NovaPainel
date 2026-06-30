#include "novapanel_ui.hpp"

namespace nova {

namespace {

struct AgendaItem {
    const char *time;
    const char *title;
    const char *subtitle;
    uint32_t color;
};

static lv_obj_t *make_stat_cell(lv_obj_t *parent,
                                const char *value,
                                const char *label,
                                lv_color_t value_color)
{
    lv_obj_t *cell = lv_obj_create(parent);
    lv_obj_set_style_bg_color(cell, NP_C_ITEM_BG2, 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_style_radius(cell, 9, 0);
    lv_obj_set_style_pad_all(cell, 12, 0);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cell,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cell, LV_SCROLLBAR_MODE_OFF);
    np_label(cell, value, NP_F_4XL, value_color);
    np_label(cell, label, NP_F_XS, lv_color_hex(0x5A6478));
    return cell;
}

}  // namespace

void np_screen_calendar(lv_obj_t *parent)
{
    const auto index = static_cast<std::size_t>(ScreenId::Calendar);

    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[index] = scr;

    lv_obj_t *left = lv_obj_create(scr);
    np_obj_clear_style(left);
    lv_obj_set_height(left, lv_pct(100));
    lv_obj_set_style_flex_grow(left, 135, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left, 12, 0);

    lv_obj_t *nxt = np_card(left);
    lv_obj_set_size(nxt, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_border_color(nxt, NP_C_ACCENT, 0);
    lv_obj_set_style_border_width(nxt, 3, 0);
    lv_obj_set_style_border_side(nxt, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_pad_all(nxt, 14, 0);
    lv_obj_set_style_pad_left(nxt, 17, 0);
    lv_obj_set_flex_flow(nxt, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(nxt,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(nxt, 4, 0);

    lv_obj_t *nxt_head = np_row(nxt);
    lv_obj_set_width(nxt_head, lv_pct(100));
    lv_obj_set_style_pad_column(nxt_head, 8, 0);
    np_label(nxt_head, NP_I_CALENDAR, NP_F_ICON_XS, NP_C_ACCENT);
    np_label(nxt_head, "Próximo evento", NP_F_XS_BOLD, NP_C_TEXT_MUTED);

    lv_obj_t *nx_row = np_row(nxt);
    lv_obj_set_width(nx_row, lv_pct(100));
    lv_obj_set_flex_align(nx_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(nx_row, 10, 0);
    lv_obj_set_style_margin_top(nx_row, 2, 0);
    lv_obj_t *nx_time = np_label(nx_row, "15:30", NP_F_4XL, NP_C_ACCENT);
    lv_obj_set_width(nx_time, 86);
    lv_obj_t *nx_info = np_col(nx_row);
    lv_obj_set_width(nx_info, 0);
    lv_obj_set_style_flex_grow(nx_info, 1, 0);

    lv_obj_t *nx_title = np_label(nx_info, "Reunião - NoiseBot team", NP_F_SM_BOLD, NP_C_TEXT);
    lv_obj_set_width(nx_title, lv_pct(100));
    lv_label_set_long_mode(nx_title, LV_LABEL_LONG_WRAP);

    lv_obj_t *nx_meta = np_label(nxt, "Sala virtual - 45 min - em 1h 12min",
        NP_F_SM, lv_color_hex(0x5A6478));
    lv_obj_set_width(nx_meta, lv_pct(100));
    lv_label_set_long_mode(nx_meta, LV_LABEL_LONG_WRAP);

    lv_obj_t *ec = np_card(left);
    lv_obj_set_size(ec, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(ec, 1, 0);
    lv_obj_set_style_pad_all(ec, 14, 0);
    lv_obj_set_flex_flow(ec, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ec,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *ech = np_row(ec);
    lv_obj_set_flex_align(ech,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(ech, 10, 0);
    np_label(ech, "quarta-feira, 18 de junho", NP_F_XS_BOLD, NP_C_TEXT_MUTED);
    np_chip(ech, "+ Adicionar");

    static const AgendaItem evts[] = {
        { "15:30", "Reunião - NoiseBot team", "Trabalho - 45 min", 0xE8A83C },
        { "17:00", "Academia",                "Pessoal - 1h",      0x4ABB78 },
        { "19:30", "Jantar com Marina",       "Pessoal - 2h",      0x4F7ECB },
        { "21:00", "Revisar firmware v1.4",   "Projeto - 30 min",  0xB77ABB },
    };

    for (int i = 0; i < 4; ++i) {
        lv_obj_t *row = np_row(ec);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, NP_C_SEP, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_column(row, 11, 0);
        lv_obj_set_style_pad_ver(row, 9, 0);

        lv_obj_t *tm = np_label(row, evts[i].time, NP_F_SM_BOLD, NP_C_TEXT_MED);
        lv_obj_set_width(tm, 40);

        lv_obj_t *bar = lv_obj_create(row);
        lv_obj_set_size(bar, 3, 32);
        lv_obj_set_style_bg_color(bar, lv_color_hex(evts[i].color), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        np_label(info, evts[i].title, NP_F_SM_BOLD, NP_C_TEXT);
        lv_obj_t *sub = np_label(info, evts[i].subtitle, NP_F_XS,
                                 lv_color_hex(0x5A6478));
        lv_obj_set_style_margin_top(sub, 0, 0);

        np_label(row, NP_I_BELL, NP_F_ICON_SM, NP_C_TEXT_MUTED);
    }

    lv_obj_t *right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_height(right, lv_pct(100));
    lv_obj_set_style_flex_grow(right, 100, 0);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 12, 0);

    lv_obj_t *sg = np_card(right);
    lv_obj_set_size(sg, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_t *sg_head = np_row(sg);
    lv_obj_set_style_pad_column(sg_head, 8, 0);
    np_label(sg_head, NP_I_LIST, NP_F_ICON_XS, NP_C_ACCENT);
    np_label(sg_head, "Resumo", NP_F_XS_BOLD, NP_C_TEXT_MUTED);

    static int32_t cols[] = { LV_PCT(50), LV_PCT(50), LV_GRID_TEMPLATE_LAST };
    static int32_t rows[] = { LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(sg);
    lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_margin_top(grid, 12, 0);
    lv_obj_set_style_pad_row(grid, 9, 0);
    lv_obj_set_style_pad_column(grid, 9, 0);
    lv_obj_set_grid_dsc_array(grid, cols, rows);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    make_stat_cell(grid, "4", "total", NP_C_TEXT);
    make_stat_cell(grid, "1h", "proximo", NP_C_ACCENT);
    make_stat_cell(grid, "2", "trabalho", NP_C_ACCENT);
    make_stat_cell(grid, "2", "pessoal", NP_C_GREEN);

    lv_obj_t *tom = np_card(right);
    lv_obj_set_size(tom, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(tom, 1, 0);
    lv_obj_set_flex_flow(tom, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *tom_head = np_row(tom);
    lv_obj_set_style_pad_column(tom_head, 8, 0);
    np_label(tom_head, NP_I_CALENDAR, NP_F_ICON_XS, NP_C_ACCENT);
    np_label(tom_head, "Amanhã", NP_F_XS_BOLD, NP_C_TEXT_MUTED);

    lv_obj_t *empty = lv_obj_create(tom);
    lv_obj_set_size(empty, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(empty, 1, 0);
    lv_obj_set_style_bg_opa(empty, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(empty, lv_color_hex(0x252840), 0);
    lv_obj_set_style_border_width(empty, 1, 0);
    lv_obj_set_style_radius(empty, 9, 0);
    lv_obj_set_style_margin_top(empty, 12, 0);
    lv_obj_set_flex_flow(empty, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(empty,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(empty, 8, 0);
    lv_obj_set_scrollbar_mode(empty, LV_SCROLLBAR_MODE_OFF);
    np_label(empty, NP_I_LIST, NP_F_ICON_LG, lv_color_hex(0x363C52));
    np_label(empty, "Sem compromissos", NP_F_SM_BOLD, NP_C_TEXT_MUTED);
}

}  // namespace nova
