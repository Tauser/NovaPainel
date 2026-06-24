# NovaPanel — LVGL Export

Código C completo para LVGL v8.3 gerado a partir do protótipo HTML NovaPanel.

## Estrutura

```
novapanel_theme.h        — cores, dimensões, helpers inline
novapanel.h              — enums, state struct, protótipos
novapanel.c              — init, rail de navegação, topbar, roteador de telas
screens/
  screen_home.c          — Início (relógio + clima + agenda + player + mercado mini)
  screen_agenda.c        — Agenda (lista de eventos + resumo)
  screen_mercado.c       — Mercado (candlestick BTC + USD/BRL + Ibovespa)
  screen_casa.c          — Casa (cenas + dispositivos IoT + sensores)
  screen_alarmes.c       — Alarmes (lista com toggles + adicionar)
  screen_clima.c         — Clima (atual + horária + 5 dias + detalhes)
  screen_timer.c         — Pomodoro (arc animado + presets + play/reset)
  screen_config.c        — Configurações (rede + display/som + sistema)
  screen_notif.c         — Notificações (lista scrollável)
CMakeLists.txt           — ESP-IDF component
```

## Display alvo

| Parâmetro | Valor |
|-----------|-------|
| Resolução | 1024 × 600 px |
| Profundidade de cor | 16-bit (RGB565) |
| LVGL | v8.3.x |
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

### 1. Copiar como componente ESP-IDF

```
<seu_projeto>/
  components/
    novapanel/          ← cole esta pasta aqui
      novapanel.h
      novapanel.c
      novapanel_theme.h
      screens/
      CMakeLists.txt
  main/
    main.c
```

### 2. Chamar no `app_main`

```c
#include "novapanel.h"

void app_main(void)
{
    /* inicializar display, touch e LVGL aqui */
    bsp_display_start();
    bsp_touch_start();

    lv_init();
    /* ... driver setup ... */

    /* inicializar NovaPanel */
    np_init();

    /* loop LVGL */
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

### 3. Relógio — atualizar a cada 1 segundo

```c
/* em uma task FreeRTOS ou timer LVGL */
static void clock_timer_cb(lv_timer_t *t)
{
    np_tick();
}

/* registrar após np_init() */
lv_timer_create(clock_timer_cb, 1000, NULL);
```

### 4. Navegar entre telas programaticamente

```c
np_navigate_to(NP_SCR_AGENDA);
np_navigate_to(NP_SCR_TIMER);
np_navigate_to(NP_SCR_CONFIG);
```

## Paleta de cores

| Token | Hex | Uso |
|-------|-----|-----|
| `NP_C_BG` | `#0D0F18` | Fundo principal |
| `NP_C_CARD` | `#141721` | Surface dos cards |
| `NP_C_BORDER` | `#1E2235` | Bordas sutis |
| `NP_C_ACCENT` | `#E8A83C` | Âmbar (destaque) |
| `NP_C_GREEN` | `#4ABB78` | Sucesso / alta |
| `NP_C_RED` | `#D05252` | Erro / queda |
| `NP_C_BLUE` | `#4F7ECB` | Info / precipitação |
| `NP_C_TEXT` | `#E8EAF2` | Texto primário |
| `NP_C_TEXT_DIM` | `#7A8298` | Texto secundário |

## Ícones personalizados (opcional)

Os ícones da rail usam `LV_SYMBOL_*` embutidos. Para ícones idênticos
ao protótipo (Feather Icons), gere uma fonte com o
[LVGL Font Converter](https://lvgl.io/tools/fontconverter) e substitua
os símbolos nas entradas `nav_items[]` em `novapanel.c`.

## Tela Mercado — candlestick

O gráfico usa `LV_EVENT_DRAW_MAIN_END` com chamadas diretas a
`lv_draw_line` / `lv_draw_rect` — sem dependências externas.
Alimente dados reais substituindo o array `candles[]` em
`screens/screen_mercado.c` com valores vindos de uma API REST ou MQTT.
