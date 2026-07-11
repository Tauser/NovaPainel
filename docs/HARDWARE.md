# NovaPanel — Hardware (baseline v4)

> Fatos de hardware validados em bancada (herdados da Fase 0 do baseline v3,
> fechada em 2026-06-26 com Gates 1–16 PASS — histórico em
> `FASE0-CHECKLIST.md`). **Este patrimônio não se refaz no v4; se reusa.**
> Regras de alocação/limites derivados: `RESOURCE-BUDGET.md`.

## Placa confirmada

**Waveshare ESP32-P4-WIFI6-Touch-LCD-7B**
([wiki](https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B/Development-Environment-Setup-IDF),
[repo](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B)).

> **Pinout completo:** `waveshare_esp32_p4_wifi6_touch_lcd_7b_hardware.md`
> (mapa técnico consolidado). Este arquivo foca no que é decisão/gotcha do
> NovaPanel.
>
> **Bateria do RTC: recarregável (ML1220, 3–3.3 V).** NÃO usar CR1220 comum
> (não recarregável; risco no circuito de carga).

## Fatos validados (medição direta + BSP oficial)

```text
- ESP32-P4NRW32: flash NOR 32 MB externa + PSRAM 32 MB no package
  (confirmados via boot log).
- Silício P4 revisão v1.3: exige CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y +
  CONFIG_ESP32P4_REV_MIN_100=y, senão o bootloader rejeita o chip.
- ESP32-C6-MINI-1 via SDIO: CLK[18] CMD[19] D0[14] D1[15] D2[16] D3[17]
  Slave_Reset[54] (medidos em harness).
- Duas portas USB-C, AMBAS no P4 (mesmo MAC):
  · "USB1.1 FS" = USB nativo (JTAG/Serial) → usar para flash/monitor;
  · "USB TO UART" = ponte CH343 para o mesmo P4 (console alternativo).
  Nenhuma chega ao C6.
- Firmware do C6: atualizar via ESP-Hosted Slave OTA pelo SDIO (método
  "Partition OTA" do exemplo host_performs_slave_ota do componente
  espressif/esp_hosted). Header "ESP32-C6 UART Terminal" é plano B não
  testado.
- Display: painel EK79007, MIPI-DSI 2-lane, 1024×600, RGB565 default,
  PHY via LDO canal 3 @ 2500 mV. BSP oficial no registry:
  waveshare/esp32_p4_wifi6_touch_lcd_7b (usar; não reimplementar).
- Touch GT911 (I2C): BSP_I2C_SCL=GPIO8, BSP_I2C_SDA=GPIO7,
  BSP_LCD_RST=GPIO33, BSP_LCD_BACKLIGHT=GPIO32; TOUCH_RST/INT = GPIO_NC
  (só I2C, sem reset/interrupt dedicados).
- RTC: SEM chip dedicado — domínio RTC interno do P4 com backup de bateria
  no pino ESP_VBAT. Mantém hora sem energia enquanto a bateria durar.
  Política de relógio: ADR-0014 (RTC plausível → hora imediata; NTP refina).
- SD (TF slot): SDMMC 4-bit, CLK=43 CMD=44 D0=39 D1=40 D2=41 D3=42,
  pull-ups internos, 20/40 MHz.
- Áudio: ES8311 (codec) + ES7210 (AEC) + microfones onboard. ES8311 divide
  o I2C com o GT911 → lock semântico obrigatório (ADR-0005,
  RESOURCE-BUDGET §6).
- USB Type-A OTG 2.0 HS: não usado no MVP.
```

## Gotchas de display validados

- O EK79007 **não gira por MADCTL** (mirror de hardware sem efeito visível):
  rotação 180° é por software/PPA (`sw_rotate=true`).
- `avoid_tearing` é **incompatível** com `sw_rotate` neste BSP.
- Flicker observado historicamente era do backlight (GPIO32), não tearing.
- Render LVGL em modo parcial (nunca FULL) — ver RESOURCE-BUDGET §2.

## Premissa crítica: P4 não tem rádio

Toda rede (HTTPS, NTP, LAN futura) passa pelo link **P4 ↔ C6 via
ESP-Hosted/SDIO**. Falha ou saturação desse link é falha de rede do produto —
o firmware trata o transporte como recurso compartilhado e degradável.

## Particionamento (alvo v4 — ADR-0010)

Flash de 32 MB permite A/B desde o início:

```text
nvs, nvs_keys, phy_init, otadata,
ota_0 (app), ota_1 (app),
storage (littlefs/cache), coredump,
[reserva: partição de firmware do C6 p/ slave-OTA coordenada]
```

Dimensões exatas são fixadas na Fase 1 (ROADMAP) e mantidas estáveis dali em
diante — reparticionar depois de unidades em campo é proibitivo.

## Headers de expansão (sensores futuros, fora do MVP)

```text
PH2.0 I2C  : BH1750 (luz), BME280 (temp/umid/press) — compartilham o
             barramento do GT911: confirmar endereços sem conflito e passar
             pelo lock semântico do board/.
PH2.0 UART : LD2410C (presença mmWave).
```

Sensores entram por módulo (provider + service + tela registrada) nas fases
9+, com custo declarado no RESOURCE-BUDGET.

## Links de referência

```text
Wiki:        https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B/Development-Environment-Setup-IDF
Exemplos:    https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B
BSP oficial: waveshare/esp32_p4_wifi6_touch_lcd_7b
             https://github.com/waveshareteam/Waveshare-ESP32-components/tree/main/bsp/esp32_p4_wifi6_touch_lcd_7b
Esquemático: https://files.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-7B/ESP32-P4-WIFI6-Touch-LCD-7B.pdf
             (em conflito de pinos, o BSP oficial vence o PDF)
Referência de terceiro na mesma placa: https://github.com/trailcurrentoss/TrailCurrentFireside
ESP-Hosted:  componente espressif/esp_hosted (exemplo host_performs_slave_ota)
```
