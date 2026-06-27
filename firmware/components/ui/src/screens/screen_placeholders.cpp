#include "novapanel_ui.hpp"

namespace nova {

namespace {

static void build_placeholder(ScreenId screen,
                              const char *title,
                              const char *subtitle,
                              const char *symbol,
                              lv_color_t accent)
{
    const auto index = static_cast<std::size_t>(screen);

    lv_obj_t *scr = lv_obj_create(np_content);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(scr, 24, 0);
    lv_obj_set_style_pad_row(scr, 12, 0);
    np_screens[index] = scr;

    lv_obj_t *badge = lv_obj_create(scr);
    lv_obj_set_size(badge, 74, 74);
    lv_obj_set_style_bg_color(badge, NP_C_CARD2, 0);
    lv_obj_set_style_border_color(badge, accent, 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_radius(badge, 37, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_set_scrollbar_mode(badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *ic = np_label(badge, symbol, NP_F_ICON, accent);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *ttl = np_label(scr, title, NP_F_3XL, NP_C_TEXT);
    lv_obj_set_style_text_align(ttl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *sub = np_label(scr, subtitle, NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
}

}  // namespace

void np_screen_market(lv_obj_t *)
{
    build_placeholder(ScreenId::Market, "Mercado", "Tela em construcao",
        NP_I_CHART, NP_C_GREEN);
}

void np_screen_devices(lv_obj_t *)
{
    build_placeholder(ScreenId::Devices, "Casa", "Tela em construcao",
        NP_I_DEVICE, NP_C_PURPLE);
}

void np_screen_focus(lv_obj_t *)
{
    build_placeholder(ScreenId::Focus, "Focus", "Tela em construcao",
        NP_I_FOCUS, NP_C_ACCENT);
}

void np_screen_photoframe(lv_obj_t *)
{
    build_placeholder(ScreenId::PhotoFrame, "Photo", "Tela em construcao",
        NP_I_IMAGE, NP_C_BLUE);
}

void np_screen_routines(lv_obj_t *)
{
    build_placeholder(ScreenId::Routines, "Rotinas", "Tela em construcao",
        NP_I_REFRESH, NP_C_RED);
}

void np_screen_settings(lv_obj_t *)
{
    build_placeholder(ScreenId::Settings, "Configuracoes", "Tela em construcao",
        NP_I_SETTINGS, NP_C_ACCENT);
}

void np_screen_system(lv_obj_t *)
{
    build_placeholder(ScreenId::System, "Sistema", "Tela em construcao",
        NP_I_INFO, NP_C_GREEN);
}

}  // namespace nova
