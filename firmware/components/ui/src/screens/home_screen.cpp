#include "home_view_model.hpp"
#include "strings_ptbr.hpp"
#include "ui_theme.hpp"

#include "lvgl.h"

#include <cstdio>

namespace nova {
namespace {

struct HomeScreenContext {
    lv_obj_t* root{nullptr};
    lv_obj_t* title{nullptr};
    lv_obj_t* status{nullptr};
    lv_obj_t* detail{nullptr};
    lv_obj_t* clock{nullptr};
};

HomeScreenContext g_home_screen{};

void set_text_if_changed(lv_obj_t* label, const char* text) {
    if (lv_strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

}  // namespace

lv_obj_t* build_home_screen(lv_obj_t* parent) {
    auto& ctx = g_home_screen;
    ctx.root = lv_obj_create(parent);
    lv_obj_set_size(ctx.root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(ctx.root, ui_theme::color_bg(), 0);
    lv_obj_set_style_border_width(ctx.root, 0, 0);
    lv_obj_set_style_pad_all(ctx.root, 24, 0);
    lv_obj_set_flex_flow(ctx.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ctx.root, 16, 0);

    lv_obj_t* hero = lv_obj_create(ctx.root);
    lv_obj_set_width(hero, LV_PCT(100));
    lv_obj_set_style_bg_color(hero, ui_theme::color_panel(), 0);
    lv_obj_set_style_border_color(hero, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(hero, 1, 0);
    lv_obj_set_style_radius(hero, 12, 0);
    lv_obj_set_style_pad_all(hero, 20, 0);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(hero, 8, 0);

    ctx.title = lv_label_create(hero);
    lv_obj_set_style_text_font(ctx.title, ui_theme::font_title(), 0);
    lv_obj_set_style_text_color(ctx.title, ui_theme::color_text(), 0);

    ctx.status = lv_label_create(hero);
    lv_obj_set_style_text_font(ctx.status, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.status, ui_theme::color_accent(), 0);

    ctx.detail = lv_label_create(hero);
    lv_obj_set_style_text_font(ctx.detail, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.detail, ui_theme::color_muted(), 0);

    lv_obj_t* diagnostics = lv_obj_create(ctx.root);
    lv_obj_set_width(diagnostics, LV_PCT(100));
    lv_obj_set_style_bg_color(diagnostics, ui_theme::color_panel_alt(), 0);
    lv_obj_set_style_border_color(diagnostics, ui_theme::color_border(), 0);
    lv_obj_set_style_border_width(diagnostics, 1, 0);
    lv_obj_set_style_radius(diagnostics, 12, 0);
    lv_obj_set_style_pad_all(diagnostics, 20, 0);
    lv_obj_set_flex_flow(diagnostics, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(diagnostics, 8, 0);

    lv_obj_t* label = lv_label_create(diagnostics);
    lv_obj_set_style_text_font(label, ui_theme::font_small(), 0);
    lv_obj_set_style_text_color(label, ui_theme::color_muted(), 0);
    lv_label_set_text(label, "Estado base da Fase 2");

    ctx.clock = lv_label_create(diagnostics);
    lv_obj_set_style_text_font(ctx.clock, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.clock, ui_theme::color_text(), 0);

    return ctx.root;
}

void update_home_screen(const AppState& state) {
    const HomeViewModel vm = make_home_view_model(state);
    char clock_text[96];
    std::snprintf(clock_text, sizeof(clock_text),
                  "screen=%s source=%s display=%d net=%d",
                  to_string(state.ui.active_screen),
                  to_string(state.system.source),
                  state.system.display_ready,
                  state.system.network_ready);

    set_text_if_changed(g_home_screen.title, vm.title);
    set_text_if_changed(g_home_screen.status, vm.status);
    set_text_if_changed(g_home_screen.detail, vm.detail);
    set_text_if_changed(g_home_screen.clock, clock_text);
}

lv_obj_t* build_placeholder_screen(lv_obj_t* parent) {
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, ui_theme::color_bg(), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 24, 0);

    lv_obj_t* label = lv_label_create(root);
    lv_obj_set_style_text_font(label, ui_theme::font_title(), 0);
    lv_obj_set_style_text_color(label, ui_theme::color_text(), 0);
    lv_label_set_text(label, strings_ptbr::kPlaceholderMessage);
    lv_obj_center(label);
    return root;
}

void update_placeholder_screen(const AppState& /*state*/) {}

}  // namespace nova
