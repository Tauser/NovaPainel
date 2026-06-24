# NovaPanel — LVGL v9.5 Export

Código C completo para **LVGL v9.5** gerado a partir do protótipo HTML NovaPanel.  
(Versão anterior para v8.3 está na pasta `lvgl_export/`)

---

## O que mudou de v8 → v9.5

| API v8.3 | API v9.5 | Arquivo(s) |
|---|---|---|
| `lv_scr_act()` | `lv_screen_active()` | `novapanel.c` |
| `lv_btn_create()` | `lv_button_create()` | todos |
| `lv_obj_set_flex_grow(obj, n)` | `lv_obj_set_style_flex_grow(obj, n, 0)` | todos |
| `lv_coord_t cols[]` (grid) | `int32_t cols[]` | `screen_agenda.c` |
| `lv_draw_ctx_t *draw_ctx` | `lv_layer_t *layer` | `screen_mercado.c` |
| `lv_event_get_draw_ctx(e)` | `lv_event_get_layer(e)` | `screen_mercado.c` |
| `lv_draw_line(ctx, &dsc, &p1, &p2)` | `dsc.p1=p1; dsc.p2=p2; lv_draw_line(layer,&dsc)` | `screen_mercado.c` |
| `lv_point_t` (em draw) | `lv_point_precise_t` | `screen_mercado.c` |
| `lv_obj_set_style_arc_rounded()` | `lv_obj_set_style_arc_round_start()` + `lv_obj_set_style_arc_round_end()` | `screen_timer.c` |
| `LV_LABEL_LONG_DOT` | `LV_LABEL_LONG_DOTS` | `screen_home.c` |
| `lv_obj_set_style_border_left_*` | `border_width` + `border_side(LV_BORDER_SIDE_LEFT)` | `screen_agenda.c` |

---

## Estrutura

```
novapanel_theme.h        — cores, dimensões, helpers inline
novapanel.h              — enums, state struct, protótipos
novapanel.c              — init, rail, topbar, navegação
screens/
  screen_home.c
  screen_agenda.c
  screen_mercado.c       — candlestick draw via lv_layer_t (v9)
  screen_casa.c
  screen_alarmes.c
  screen_clima.c
  screen_timer.c         — arc com round_start/end (v9)
  screen_config.c
  screen_notif.c
CMakeLists.txt
```

## Display alvo

| Parâmetro | Valor |
|-----------|-------|
| Resolução | 1024 × 600 px |
| Profundidade | 16-bit RGB565 |
| LVGL | **v9.5.x** |
| Chip | ESP32-P4 |

## lv_conf.h — fontes necessárias

```c
#define LV_FONT_MONTSERRAT_10  1
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_28  1
#define LV_FONT_MONTSERRAT_32  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT        (&lv_font_montserrat_14)
```

## Uso

```c
#include "novapanel.h"

void app_main(void)
{
    bsp_display_start();
    bsp_touch_start();
    lv_init();
    /* ... driver setup ... */

    np_init();   /* constrói toda a UI */

    /* tick do relógio e timer Pomodoro */
    lv_timer_create([](lv_timer_t *){ np_tick(); }, 1000, NULL);

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

## Navegação programática

```c
np_navigate_to(NP_SCR_MERCADO);
np_navigate_to(NP_SCR_TIMER);
np_navigate_to(NP_SCR_CONFIG);
```

## Notas de compatibilidade v9.5

- **Bordas tracejadas** não são suportadas nativamente pelo LVGL — a borda do card "Amanhã" usa linha sólida fina.
- **`lv_point_precise_t`** suporta coordenadas sub-pixel (float); este código usa inteiros, o que é compatível.
- **`lv_obj_set_flex_flow/align()`** ainda existem como wrappers de conveniência em v9; `lv_obj_set_style_flex_grow()` é a forma canônica para o grow.
- **`lv_bar_set_start_value()`** ainda existe em v9 para barras em modo range; não é usado neste código (as barras são simples).
