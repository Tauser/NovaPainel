# Gate 15 - Harness de coexistência (Fase 0)

Firmware de teste **independente** que valida o risco que pode inviabilizar o
NovaPainel: **Wi-Fi (via ESP32-C6/ESP-Hosted) + framebuffer em PSRAM + touch
rodando ao mesmo tempo, sem resetar.** Protocolo e critérios PASS/FAIL completos
em `docs/FASE0-CHECKLIST.md` (Gate 15 e Gate 16).

Não é código de produto — é um experimento de bring-up, em C puro, sem depender
de nenhum componente do firmware principal.

## O que ele faz

Sobe cargas concorrentes em tasks separadas (render e rede em cores
diferentes, como em produção — ADR-0013):

- **render_task** — preenche o framebuffer (1024×600 RGB565, ~1.2 MB) em PSRAM a
  cada 1s e chama `panel_blit()` / `touch_poll()`.
- **https_task** — GET HTTPS a cada 60s (perfil do `MarketService`, ADR-0006) +
  um burst de 12 req (1/5s) por hora para estressar o SDIO.
- **sd_task** — escreve + lê de volta um arquivo no cartão SD a cada 60s, na
  mesma cadência do `https_task` (Gate 7/13: SD + Wi-Fi simultâneos).
  Autodesliga se o mount falhar (sem cartão inserido, por exemplo).
- **stats_task** — loga a cada 60s: uptime, contador de reboots (NVS), motivo do
  último reset, conexões/desconexões Wi-Fi, HTTPS ok/fail, NTP sincronizado,
  SD ok/fail, e watermark de heap interno e PSRAM (free + maior bloco).
- **NTP** — sincroniza via `pool.ntp.org` uma vez que o Wi-Fi conecta (Gate 11),
  sem task dedicada (`esp_netif_sntp`, notificação assíncrona).

## Real agora vs. hook

| Parte | Estado |
|-------|--------|
| Framebuffer em PSRAM + churn por segundo | **real** |
| Wi-Fi connect + contagem de desconexão/reconexão | **real** |
| NTP (Gate 11) | **real** |
| SD card write/read (Gate 7/13) | **real** (precisa de cartão físico inserido) |
| HTTPS GET + burst | **real** |
| Instrumentação (reset reason, reboots, heap/PSRAM) | **real** |
| `panel_blit()` — empurrar o frame pro painel | **real** (BSP oficial `waveshare/esp32_p4_wifi6_touch_lcd_7b`, EK79007) |
| `touch_poll()` — ler o controlador de toque | **real** (mesmo BSP, GT911) |

Display e touch agora usam o BSP oficial Waveshare (Gates 5/6 fechados em
2026-06-23) — ver `docs/HARDWARE.md` e `docs/FASE0-CHECKLIST.md`. Não há
confirmação visual (não dá pra ver a tela fisicamente neste teste), só
confirmação de que os drivers inicializam e operam sem erro/panic.

## Antes de rodar

1. Confirmar os Gates 8 e 9 (firmware do C6 / link SDIO) — sem isso o Wi-Fi não
   sobe. Ajustar as versões dos componentes em `main/idf_component.yml`.
2. Definir credenciais via `idf.py menuconfig` (menu "Gate 15 coexistence
   harness" -> `GATE15_WIFI_SSID`/`GATE15_WIFI_PASS`, de `main/Kconfig.projbuild`).
   Isso grava só no `sdkconfig` local, que é gitignored. **Nunca** colocar
   credenciais reais em `gate15_main.c` ou em `sdkconfig.defaults`.
3. Inserir um cartão microSD no slot antes de ligar, senão `sd_task` se
   autodesliga (Gate 7/13 não exercitados).

## Build e flash

```bash
cd firmware/experiments/gate15_coexistence
idf.py set-target esp32p4
idf.py build
idf.py -p <porta> flash monitor
```

## Como ler o resultado (critérios do Gate 15)

Deixe rodar pelo menos 8h (decisão 2026-06-23, reduzido de 72h — ver
`docs/FASE0-CHECKLIST.md` Gate 15). Durante o teste, derrube o AP por ~2 min a
cada ~30 min (rede instável cíclica) e confirme que o sistema **não reseta** e
o render **não congela**.

- **PASS:** zero linhas de reset com `reset_reason=PANIC/INT_WDT/TASK_WDT/BROWNOUT`;
  `reboots` não cresce; `render` sobe de forma contínua mesmo com Wi-Fi `DOWN`;
  watermark de heap interno e maior bloco PSRAM estáveis após a 1ª hora.
- **FAIL:** qualquer reset não intencional, ou render que para de subir durante
  queda de rede, ou heap/PSRAM com tendência de queda contínua (leak/fragmentação).

Um `reset_reason=BROWNOUT` aponta para o **Gate 16** (fonte/consumo/térmica), não
para bug de firmware. Registre a conclusão na tabela de evidências do
`FASE0-CHECKLIST.md`.

## Git

Não commitar `build/` nem `sdkconfig` gerado (só `sdkconfig.defaults`).
