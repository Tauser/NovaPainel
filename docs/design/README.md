# NovaPainel - Material de design (referência)

Conteúdo desta pasta é **referência visual/de layout, não código de
produto**. Nada aqui é compilado pelo build do firmware.

## `lvgl_export_reference/`

Export gerado (LVGL v8.3) com as 9 telas do protótipo do NovaPainel: Home,
Agenda, Mercado, Casa, Alarmes, Clima, Timer, Config, Notif. Layout, cores
(`novapanel_theme.h`) e composição de widgets são bom material de partida
para a Fase 4 em diante.

**Não colar direto em `firmware/components/ui/`.** Duas adaptações são
necessárias antes de qualquer tela daqui virar código real:

1. **LVGL v8.3 -> v9.5** - o projeto resolveu LVGL 9.5.0 via o BSP oficial
   Waveshare (confirmado no build da Fase 3). A API de widgets
   (`lv_obj_create`, `lv_obj_set_flex_flow`, `lv_obj_set_style_*`,
   `lv_label_create`) é estável entre as duas versões, mas precisa de
   checagem ponto a ponto - não é drop-in.
2. **Arquitetura** - o export usa um `np_state_t` global e as telas
   leem/escrevem esse estado direto. O NovaPainel usa `StateStore` +
   `EventBus` + `UiDispatcher` (ADR-0007/0013): só a `lvgl_task` toca
   objetos LVGL, e a UI nunca guarda estado próprio - ela lê do
   `StateStore`. Cada tela portada precisa trocar o acesso a `np_state`
   por leitura do `AppState`/`SystemStatus` reais.

## Escopo (decisão registrada em 2026-06-24)

Só a tela **Home** (`screens/screen_home.c`) é prioridade - é a única no
escopo do MVP (Fase 4/5/6 do `docs/ROADMAP.md`). As outras 8 telas ficam
como referência até a fase/feature correspondente chegar:

```text
screen_agenda.c   -> Fase 12+ (v1.0, fora do MVP)
screen_mercado.c  -> tela dedicada de mercado, fora do MVP (Home já tem mini-mercado)
screen_casa.c     -> Fase 14 (Sonoff/automação física, futuro)
screen_alarmes.c  -> Fase 12+ (v1.0)
screen_clima.c    -> depende de provider de clima (Fase 5, Open-Meteo) existir
screen_timer.c    -> Fase 12+ (v1.0)
screen_config.c   -> sem fase definida ainda
screen_notif.c    -> NotificationService já existe (mock); tela é Fase 4+
```
