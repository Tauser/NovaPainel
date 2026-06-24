/* ================================================================
 * SCREEN: Configurações
 * Layout — Profile bar + 3-column grid (Rede · Display/Som · Sistema)
 * ================================================================ */
#include "../novapanel.h"
#include <stdio.h>

/* Slider callback helpers */
static lv_obj_t *g_brilho_lbl   = NULL;
static lv_obj_t *g_vsys_lbl     = NULL;
static lv_obj_t *g_vmus_lbl     = NULL;
static lv_obj_t *g_valm_lbl     = NULL;

static void brilho_cb(lv_event_t *e) {
    lv_obj_t *sl = lv_event_get_target(e);
    np_state.brilho = lv_slider_get_value(sl);
    if (g_brilho_lbl) {
        char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", (int)np_state.brilho);
        lv_label_set_text(g_brilho_lbl, buf);
    }
}
static void vsys_cb(lv_event_t *e) {
    lv_obj_t *sl = lv_event_get_target(e);
    np_state.vol_system = lv_slider_get_value(sl);
    if (g_vsys_lbl) {
        char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", (int)np_state.vol_system);
        lv_label_set_text(g_vsys_lbl, buf);
    }
}
static void vmus_cb(lv_event_t *e) {
    lv_obj_t *sl = lv_event_get_target(e);
    np_state.vol_music = lv_slider_get_value(sl);
    if (g_vmus_lbl) {
        char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", (int)np_state.vol_music);
        lv_label_set_text(g_vmus_lbl, buf);
    }
}
static void valm_cb(lv_event_t *e) {
    lv_obj_t *sl = lv_event_get_target(e);
    np_state.vol_alarm = lv_slider_get_value(sl);
    if (g_valm_lbl) {
        char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", (int)np_state.vol_alarm);
        lv_label_set_text(g_valm_lbl, buf);
    }
}

/* Helper: slider row with label + value */
static void slider_row(lv_obj_t *parent, const char *title,
                       int32_t val, lv_event_cb_t cb, lv_obj_t **out_lbl)
{
    lv_obj_t *row = np_row(parent);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(row, 6, 0);
    np_label(row, title, NP_F_SM, NP_C_TEXT_DIM);

    char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", (int)val);
    lv_obj_t *vl = np_label(row, buf, NP_F_LG, NP_C_ACCENT);
    if (out_lbl) *out_lbl = vl;

    lv_obj_t *sl = lv_slider_create(parent);
    lv_obj_set_size(sl, lv_pct(100), 6);
    lv_obj_set_style_margin_bottom(sl, 14, 0);
    lv_slider_set_range(sl, (title[0]=='B') ? 10 : 0, 100);
    lv_slider_set_value(sl, val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, NP_C_BORDER,  LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, NP_C_ACCENT,  LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, NP_C_ACCENT,  LV_PART_KNOB);
    lv_obj_set_style_radius(sl, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(sl, 3, LV_PART_INDICATOR);
    lv_obj_set_style_radius(sl, 6, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sl, 4, LV_PART_KNOB);
    if (cb) lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void np_screen_config(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 11, 0);
    np_screens[NP_SCR_CONFIG] = scr;

    /* ── Profile bar ── */
    lv_obj_t *prof = np_card(scr);
    lv_obj_set_size(prof, lv_pct(100), 68);
    lv_obj_set_style_pad_hor(prof, 18, 0);
    lv_obj_set_style_pad_ver(prof, 0, 0);
    lv_obj_set_flex_flow(prof, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(prof,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(prof, 14, 0);

    lv_obj_t *av = lv_obj_create(prof);
    lv_obj_set_size(av, 42, 42);
    lv_obj_set_style_bg_color(av,     NP_C_ACCENT, 0);
    lv_obj_set_style_radius(av,       21, 0);
    lv_obj_set_style_border_width(av, 0, 0);
    lv_obj_set_style_pad_all(av,      0, 0);
    lv_obj_t *av_l = np_label(av, "RL", NP_F_LG, NP_C_DARK_FG);
    lv_obj_center(av_l);

    lv_obj_t *pi = np_col(prof);
    lv_obj_set_flex_grow(pi, 1);
    lv_obj_set_size(pi, 0, LV_SIZE_CONTENT);
    np_label(pi, "Rafael Lopes", NP_F_MD, NP_C_TEXT);
    np_label(pi, "Perfil padrão · NovaPanel ESP32-P4",
             NP_F_SM, lv_color_hex(0x5A6478));

    np_chip(prof, "Editar");

    /* ── 3-column grid ── */
    lv_obj_t *grid = lv_obj_create(scr);
    np_obj_clear_style(grid);
    lv_obj_set_size(grid, lv_pct(100), 0);
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(grid, 11, 0);

    /* ════ COL 1: Rede ════ */
    lv_obj_t *net = np_card(grid);
    lv_obj_set_flex_grow(net, 1);
    lv_obj_set_height(net, lv_pct(100));
    lv_obj_set_style_pad_all(net, 16, 0);
    lv_obj_set_flex_flow(net, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(net, LV_SCROLLBAR_MODE_ON);

    lv_obj_t *net_h = np_row(net);
    lv_obj_set_style_margin_bottom(net_h, 14, 0);
    lv_obj_set_style_pad_column(net_h, 7, 0);
    np_label(net_h, LV_SYMBOL_WIFI, NP_F_MD, NP_C_ACCENT);
    np_label(net_h, "Rede", NP_F_SM, NP_C_ACCENT);

    np_label(net, "WI-FI", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t *wifi_card = lv_obj_create(net);
    lv_obj_set_size(wifi_card, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wifi_card,   NP_C_CARD2, 0);
    lv_obj_set_style_bg_opa(wifi_card,     LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(wifi_card, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_border_width(wifi_card, 1, 0);
    lv_obj_set_style_radius(wifi_card, 9, 0);
    lv_obj_set_style_pad_all(wifi_card, 11, 0);
    lv_obj_set_style_margin_top(wifi_card, 7, 0);
    lv_obj_set_scrollbar_mode(wifi_card, LV_SCROLLBAR_MODE_OFF);
    np_label(wifi_card, "NovaNet 5G", NP_F_SM, NP_C_TEXT);
    np_label(wifi_card, "192.168.0.114 · -52 dBm · ●●●●",
             NP_F_XS, lv_color_hex(0x5A6478));

    lv_obj_t *wm = np_label(net, "Gerenciar redes Wi-Fi >",
                             NP_F_SM, NP_C_ACCENT);
    lv_obj_set_style_margin_top(wm, 7, 0);
    lv_obj_set_style_margin_bottom(wm, 14, 0);

    np_hsep(net);

    lv_obj_t *bt_hdr = np_row(net);
    lv_obj_set_style_margin_top(bt_hdr, 14, 0);
    lv_obj_set_style_margin_bottom(bt_hdr, 4, 0);
    lv_obj_set_flex_align(bt_hdr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    np_label(bt_hdr, "BLUETOOTH", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t *bt_row = np_row(net);
    lv_obj_set_flex_align(bt_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(bt_row, 14, 0);
    lv_obj_t *bt_info = np_col(bt_row);
    lv_obj_set_size(bt_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(bt_info, "Bluetooth", NP_F_SM, NP_C_TEXT);
    np_label(bt_info, "Desativado", NP_F_XS, lv_color_hex(0x5A6478));
    np_toggle(bt_row, np_state.bt_enabled);

    np_hsep(net);

    lv_obj_t *tz_h = np_row(net);
    lv_obj_set_style_margin_top(tz_h, 14, 0);
    lv_obj_set_style_margin_bottom(tz_h, 8, 0);
    np_label(tz_h, "FUSO HORÁRIO", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t *tz = lv_obj_create(net);
    lv_obj_set_size(tz, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tz, NP_C_CARD2, 0);
    lv_obj_set_style_bg_opa(tz, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tz, lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_border_width(tz, 1, 0);
    lv_obj_set_style_radius(tz, 9, 0);
    lv_obj_set_style_pad_all(tz, 10, 0);
    lv_obj_set_scrollbar_mode(tz, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(tz, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tz,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *tz_info = np_col(tz);
    lv_obj_set_size(tz_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(tz_info, "America/São Paulo", NP_F_SM, NP_C_TEXT);
    np_label(tz_info, "UTC-3 · NTP automático",
             NP_F_XS, lv_color_hex(0x5A6478));
    np_label(tz, LV_SYMBOL_RIGHT, NP_F_SM, NP_C_TEXT_MUTED);

    /* ════ COL 2: Display & Som ════ */
    lv_obj_t *disp = np_card(grid);
    lv_obj_set_flex_grow(disp, 1);
    lv_obj_set_height(disp, lv_pct(100));
    lv_obj_set_style_pad_all(disp, 16, 0);
    lv_obj_set_flex_flow(disp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(disp, LV_SCROLLBAR_MODE_ON);

    lv_obj_t *disp_h = np_row(disp);
    lv_obj_set_style_margin_bottom(disp_h, 14, 0);
    lv_obj_set_style_pad_column(disp_h, 7, 0);
    np_label(disp_h, LV_SYMBOL_TINT, NP_F_MD, NP_C_ACCENT);
    np_label(disp_h, "Display & Som", NP_F_SM, NP_C_ACCENT);

    np_label(disp, "BRILHO", NP_F_XS, NP_C_TEXT_DARK);
    slider_row(disp, "Brilho da tela", np_state.brilho, brilho_cb, &g_brilho_lbl);

    np_label(disp, "VOLUME", NP_F_XS, NP_C_TEXT_DARK);
    slider_row(disp, "Sistema", np_state.vol_system, vsys_cb, &g_vsys_lbl);
    slider_row(disp, "Música",  np_state.vol_music,  vmus_cb, &g_vmus_lbl);
    slider_row(disp, "Alarme",  np_state.vol_alarm,  valm_cb, &g_valm_lbl);

    np_hsep(disp);

    lv_obj_t *th_h = np_row(disp);
    lv_obj_set_style_margin_top(th_h, 14, 0);
    lv_obj_set_style_margin_bottom(th_h, 9, 0);
    np_label(th_h, "TEMA", NP_F_XS, NP_C_TEXT_DARK);

    /* theme segmented */
    lv_obj_t *thm = lv_obj_create(disp);
    lv_obj_set_size(thm, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(thm, lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(thm, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(thm, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(thm, 1, 0);
    lv_obj_set_style_radius(thm, 8, 0);
    lv_obj_set_style_pad_all(thm, 3, 0);
    lv_obj_set_style_margin_bottom(thm, 14, 0);
    lv_obj_set_flex_flow(thm, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollbar_mode(thm, LV_SCROLLBAR_MODE_OFF);

    static const char *themes[] = { "Grafite", "Carvão", "Âmbar" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *tb = lv_obj_create(thm);
        lv_obj_set_height(tb, 30);
        lv_obj_set_flex_grow(tb, 1);
        lv_obj_set_style_bg_color(tb, i == 0 ? lv_color_hex(0x252A3C) : lv_color_hex(0x1A1D2C), 0);
        lv_obj_set_style_bg_opa(tb, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tb, 0, 0);
        lv_obj_set_style_radius(tb, 6, 0);
        lv_obj_set_style_pad_all(tb, 0, 0);
        lv_obj_set_scrollbar_mode(tb, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *tl = np_label(tb, themes[i], NP_F_SM,
                                i == 0 ? NP_C_TEXT : NP_C_TEXT_MUTED);
        lv_obj_center(tl);
    }

    /* night mode toggle */
    lv_obj_t *nm = np_row(disp);
    lv_obj_set_flex_align(nm,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(nm, 12, 0);
    lv_obj_t *nm_info = np_col(nm);
    lv_obj_set_size(nm_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(nm_info, "Modo noturno", NP_F_SM, NP_C_TEXT);
    np_label(nm_info, "Reduz brilho automaticamente",
             NP_F_XS, lv_color_hex(0x5A6478));
    np_toggle(nm, np_state.night_mode);

    np_hsep(disp);

    /* hour format */
    lv_obj_t *hf = np_row(disp);
    lv_obj_set_flex_align(hf,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(hf, 12, 0);
    np_label(hf, "Formato de hora", NP_F_SM, NP_C_TEXT);

    lv_obj_t *fmt = lv_obj_create(hf);
    lv_obj_set_size(fmt, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_bg_color(fmt, lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(fmt, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(fmt, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(fmt, 1, 0);
    lv_obj_set_style_radius(fmt, 7, 0);
    lv_obj_set_style_pad_all(fmt, 2, 0);
    lv_obj_set_scrollbar_mode(fmt, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(fmt, LV_FLEX_FLOW_ROW);

    static const char *fmts[] = { "24h", "12h" };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *fb = lv_obj_create(fmt);
        lv_obj_set_size(fb, LV_SIZE_CONTENT, lv_pct(100));
        lv_obj_set_style_bg_color(fb, i == 0 ? lv_color_hex(0x252A3C) : lv_color_hex(0x1A1D2C), 0);
        lv_obj_set_style_bg_opa(fb, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(fb, 0, 0);
        lv_obj_set_style_radius(fb, 5, 0);
        lv_obj_set_style_pad_hor(fb, 10, 0);
        lv_obj_set_style_pad_ver(fb, 0, 0);
        lv_obj_set_scrollbar_mode(fb, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *fl = np_label(fb, fmts[i], NP_F_SM,
                                i == 0 ? NP_C_TEXT : NP_C_TEXT_MUTED);
        lv_obj_center(fl);
    }

    /* ════ COL 3: Sistema ════ */
    lv_obj_t *sys = np_card(grid);
    lv_obj_set_flex_grow(sys, 1);
    lv_obj_set_height(sys, lv_pct(100));
    lv_obj_set_style_pad_all(sys, 16, 0);
    lv_obj_set_flex_flow(sys, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(sys, LV_SCROLLBAR_MODE_ON);

    lv_obj_t *sys_h = np_row(sys);
    lv_obj_set_style_margin_bottom(sys_h, 14, 0);
    lv_obj_set_style_pad_column(sys_h, 7, 0);
    np_label(sys_h, LV_SYMBOL_SETTINGS, NP_F_MD, NP_C_ACCENT);
    np_label(sys_h, "Sistema", NP_F_SM, NP_C_ACCENT);

    static const struct { const char *k; const char *v; uint32_t c; }
    sysinfo[] = {
        { "Dispositivo",   "NovaPanel v2",  0xE8EAF2 },
        { "Firmware",      "v1.3.2",        0xE8EAF2 },
        { "Chip",          "ESP32-P4",      0xE8EAF2 },
        { "RAM livre",     "312 KB",        0x4ABB78 },
        { "Flash livre",   "1.4 MB",        0x4ABB78 },
        { "Temperatura",   "42°C",          0xE8A83C },
        { "Uptime",        "14d 06h",       0xE8EAF2 },
    };
    for (int i = 0; i < 7; i++) {
        lv_obj_t *sr = np_row(sys);
        lv_obj_set_flex_align(sr,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_side(sr, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(sr, lv_color_hex(0x1A1D2C), 0);
        lv_obj_set_style_border_width(sr, 1, 0);
        lv_obj_set_style_pad_ver(sr, 9, 0);
        np_label(sr, sysinfo[i].k, NP_F_SM, lv_color_hex(0x7A8298));
        np_label(sr, sysinfo[i].v, NP_F_SM, lv_color_hex(sysinfo[i].c));
    }

    lv_obj_set_style_margin_top(sys, 12, 0);

    /* update button */
    lv_obj_t *upd = lv_btn_create(sys);
    lv_obj_set_size(upd, lv_pct(100), 40);
    lv_obj_set_style_bg_color(upd, NP_C_GREEN_BG, 0);
    lv_obj_set_style_border_color(upd, NP_C_GREEN_BD, 0);
    lv_obj_set_style_border_width(upd, 1, 0);
    lv_obj_set_style_radius(upd, NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(upd, 0, 0);
    lv_obj_set_style_margin_bottom(upd, 7, 0);
    lv_obj_set_flex_flow(upd, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(upd,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(upd, 8, 0);
    np_label(upd, LV_SYMBOL_DOWNLOAD, NP_F_XS, NP_C_GREEN);
    np_label(upd, "Atualizar firmware v1.4", NP_F_SM, NP_C_GREEN);

    /* restart button */
    lv_obj_t *rst = lv_btn_create(sys);
    lv_obj_set_size(rst, lv_pct(100), 40);
    lv_obj_set_style_bg_color(rst, NP_C_CARD2, 0);
    lv_obj_set_style_border_color(rst, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(rst, 1, 0);
    lv_obj_set_style_radius(rst, NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(rst, 0, 0);
    lv_obj_set_flex_flow(rst, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rst,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(rst, 8, 0);
    np_label(rst, LV_SYMBOL_REFRESH, NP_F_XS, NP_C_TEXT_DIM);
    np_label(rst, "Reiniciar dispositivo", NP_F_SM, NP_C_TEXT_DIM);
}
