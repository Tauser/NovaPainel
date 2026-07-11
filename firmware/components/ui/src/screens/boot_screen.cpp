#include "boot_view_model.hpp"
#include "ui_theme.hpp"

#include "lvgl.h"

namespace nova {
namespace {

struct BootScreenContext {
    lv_obj_t* root{nullptr};
    lv_obj_t* headline{nullptr};
    lv_obj_t* detail{nullptr};
};

BootScreenContext g_boot_screen{};

void set_text_if_changed(lv_obj_t* label, const char* text) {
    if (lv_strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

}  // namespace

lv_obj_t* build_boot_screen(lv_obj_t* parent) {
    auto& ctx = g_boot_screen;
    ctx.root = lv_obj_create(parent);
    lv_obj_set_size(ctx.root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(ctx.root, ui_theme::color_bg(), 0);
    lv_obj_set_style_border_width(ctx.root, 0, 0);
    lv_obj_set_style_pad_all(ctx.root, 24, 0);
    lv_obj_set_flex_flow(ctx.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ctx.root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ctx.headline = lv_label_create(ctx.root);
    lv_obj_set_style_text_font(ctx.headline, ui_theme::font_title(), 0);
    lv_obj_set_style_text_color(ctx.headline, ui_theme::color_text(), 0);

    ctx.detail = lv_label_create(ctx.root);
    lv_obj_set_style_text_font(ctx.detail, ui_theme::font_body(), 0);
    lv_obj_set_style_text_color(ctx.detail, ui_theme::color_muted(), 0);

    return ctx.root;
}

void update_boot_screen(const AppState& state) {
    const BootViewModel vm = make_boot_view_model(state);
    set_text_if_changed(g_boot_screen.headline, vm.headline);
    set_text_if_changed(g_boot_screen.detail, vm.detail);
}

}  // namespace nova
