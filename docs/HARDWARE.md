# NovaPainel - Hardware

## Placa confirmada (Fase 0, 2026-06-23)

A placa física em mãos é a **Waveshare ESP32-P4-WIFI6-Touch-LCD-7B**
([wiki](https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B/Development-Environment-Setup-IDF),
[repo](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B)).

> **Pinout e componentes completos:** ver
> `docs/waveshare_esp32_p4_wifi6_touch_lcd_7b_hardware.md` - mapa técnico
> consolidado (chips exatos, todos os GPIOs por função, headers de expansão,
> drivers/componentes oficiais). Este arquivo aqui foca no que é específico
> de decisão/gate do NovaPainel; aquele é a referência de pinagem.
>
> **Correção importante:** a bateria do RTC precisa ser **recarregável**
> (ML1220 ou equivalente, 3-3.3V) - **não usar CR1220 comum**, que não é
> recarregável e pode ser perigoso/danificar o circuito de carga.

Specs e pinagem confirmadas (mistura de wiki + medição direta no hardware):

```text
- ESP32-P4-Core: ESP32-P4NRW32 + 32MB Nor Flash externa (confirmado via boot
  log: "SPI Flash Size: 32MB"). PSRAM: 32MB empilhada no package (confirmado:
  "esp_psram: Found 32MB PSRAM device").
- Chip do P4 é silício revisão v1.3 (não v3.x) - ver Gate 1 no
  FASE0-CHECKLIST.md. Precisa de CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y +
  CONFIG_ESP32P4_REV_MIN_100=y no sdkconfig, senão o bootloader rejeita o chip.
- ESP32-C6-MINI-1: módulo coprocessador via SDIO (confirma C6, não C5).
  GPIOs medidos no harness: CLK[18] CMD[19] D0[14] D1[15] D2[16] D3[17]
  Slave_Reset[54].
- Duas portas USB-C, AMBAS no P4 (mesmo MAC E8:F6:0A:E0:8F:42, confirmado via
  `esptool chip_id` nas duas):
  - "Type-C USB1.1 FS Port": USB nativo (USB-JTAG/Serial) do P4 - usar para
    flash/monitor do app principal.
  - "Type-C USB TO UART Port": ponte UART (chip CH343) pro mesmo P4 - console
    alternativo, não dá acesso direto ao C6.
  - Existe também um header dedicado "ESP32-C6 UART Terminal" na placa (não
    testado ainda) que parece ser a via direta de flash do C6 via UART/header
    - alternativa ao método validado abaixo.
- Atualização de firmware do C6: como nenhuma das duas portas USB-C chega ao
  C6, o caminho validado é o **ESP-Hosted Slave OTA via SDIO** (host/P4
  flasha o slave/C6 usando o link existente, sem hardware extra) - ver Gate 8
  no FASE0-CHECKLIST.md. Método "Partition OTA" do exemplo
  `host_performs_slave_ota` do componente `espressif/esp_hosted`.
- TF Card Slot (SD): SDMMC 4-bit, CLK=GPIO43, CMD=GPIO44, D0=GPIO39, D1=GPIO40,
  D2=GPIO41, D3=GPIO42. Pull-ups internos (SDMMC_SLOT_FLAG_INTERNAL_PULLUP).
  20MHz padrão / 40MHz high-speed. Ainda não testado (Gate 7).
- Display + touch: existe um **BSP oficial Waveshare no registro de
  componentes do ESP-IDF**: `waveshare/esp32_p4_wifi6_touch_lcd_7b` (v2.0.0,
  fonte: https://github.com/waveshareteam/Waveshare-ESP32-components/tree/main/bsp/esp32_p4_wifi6_touch_lcd_7b).
  Usado em produção por um projeto open-source de terceiros para a mesma
  placa ([TrailCurrentFireside](https://github.com/trailcurrentoss/TrailCurrentFireside)).
  Confirma:
  - Painel **EK79007** (componente `esp_lcd_ek79007`), MIPI-DSI 2-lane,
    **1024x600**, RGB565 por padrão (RGB888 também suportado). PHY via LDO
    canal 3 @ 2500mV.
  - Touch **GT911** (capacitivo, I2C). Pinos oficiais do BSP (fonte de
    verdade, substitui a leitura ambígua do esquemático em PDF):
    `BSP_I2C_SCL=GPIO8`, `BSP_I2C_SDA=GPIO7`, `BSP_LCD_RST=GPIO33`,
    `BSP_LCD_BACKLIGHT=GPIO32`. `BSP_LCD_TOUCH_RST` e `BSP_LCD_TOUCH_INT` são
    `GPIO_NC` (não conectados) - o GT911 funciona só via I2C nesta placa, sem
    pino de reset/interrupt dedicado.
  - LVGL port incluído (v8/v9 dependendo da versão).
  - Usar este componente real no firmware (em vez de reimplementar do zero)
    é a via mais rápida para os Gates 5/6 - já existe um BSP testado em
    produção para esta placa exata.
- RTC: **confirmado, sem chip dedicado.** O esquemático mostra o "RTC Battery
  Holder" ligado direto no pino `ESP_VBAT` do próprio ESP32-P4 - é o domínio
  RTC interno do SoC com backup de bateria (1220, 3-3.3V), não um RTC I2C
  externo (PCF85xx/DS13xx/DS32xx não aparecem no esquemático). Mantém hora
  sem energia principal enquanto a bateria durar; a decisão de Fase 0 está
  fechada: firmware usa o RTC interno quando a época é plausível e degrada para
  NTP-only/horário não sincronizado quando não houver hora confiável.
- Áudio (fora de escopo do MVP, mas presente na placa): ES8311 (codec) +
  ES7210 (cancelamento de eco) + microfones onboard.
- Porta USB Type-A adicional: USB OTG 2.0 High Speed (não usada no MVP).
```

### Descrição completa de hardware (25 componentes, doc oficial Waveshare)

Lista integral dos componentes numerados na wiki, pra referência rápida sem
precisar reabrir a página (itens já cobertos com detalhe acima não repetem
pinos aqui):

```text
1.  ESP32-P4-Core: ESP32-P4NRW32 + 32MB Nor Flash
2.  ESP32-C6-MINI-1: módulo Wi-Fi 6 / BT 5 via SDIO
3.  ESP32-C6 UART Terminal: header dedicado p/ flash direto do C6 (plano B,
    não testado - ver Gate 8 no FASE0-CHECKLIST.md)
4.  ES8311: codec de áudio (fora de escopo MVP)
5.  ES7210: cancelamento de eco (fora de escopo MVP)
6.  Interface MIPI-DSI LCD: conecta o painel de 7" (EK79007, ver acima)
7.  Interface de Touch: GT911 via I2C (ver acima)
8.  MIPI-CSI (2-lane): conector 15 pin/1.0mm p/ câmera (fora de escopo MVP,
    NoiseBot/câmera é roadmap futuro - ver ROADMAP.md)
9.  Conector de bateria MX1.25: alimentação externa (polaridade direta)
10. Header PH2.0 alto-falante: saída de áudio (não polarizado)
11. RTC Battery Holder: bateria 1220 (3-3.3V) - ver seção RTC acima
12. Switch ON/OFF de alimentação
13. LED indicador de status
14. Porta USB Type-A: USB OTG 2.0 High Speed (não usada no MVP)
15. Porta USB-C "USB1.1 FS": USB nativo do P4 (flash/monitor - usar esta)
16. Porta USB-C "USB TO UART": ponte UART (CH343) pro mesmo P4
17. Botão RESET
18. Botão BOOT: modo de download ao ligar
19. Header PH2.0 4PIN I2C: candidato natural pros sensores futuros
    BH1750/BME280 (ver seção "Sensores futuros" abaixo)
20. Header PH2.0 4PIN UART: candidato natural pro LD2410C (presença mmWave)
21. Header PH2.0 4PIN CAN: não usado no MVP
22. Header PH2.0 4PIN RS485: não usado no MVP
23. Microfones onboard (com cancelamento de eco via ES7210)
24. Headers PH2.0 12Pin GPIO: expõe 17 GPIOs programáveis + alimentação
25. TF Card Slot: SDMMC, ver pinos na seção acima
```

### Links de referência

```text
- Wiki do produto:    https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B/Development-Environment-Setup-IDF
- Repo de exemplos:   https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B
  - 07_color_panel (EK79007, MIPI-DSI, sem touch)
  - 08_lvgl_display_panel / 09_lvgl_demo_v8 / 10_lvgl_demo_v9 (LVGL + touch)
  - 04_sdmmc (SD card)
  - 05_wifistation (Wi-Fi via ESP-Hosted)
- BSP oficial (componente IDF): waveshare/esp32_p4_wifi6_touch_lcd_7b
  - Fonte: https://github.com/waveshareteam/Waveshare-ESP32-components/tree/main/bsp/esp32_p4_wifi6_touch_lcd_7b
- Esquemático (Altium PDF): https://files.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-7B/ESP32-P4-WIFI6-Touch-LCD-7B.pdf
  (texto extraível via `pdftotext`, mas com sobreposição de rótulos em
  algumas regiões - preferir o BSP oficial como fonte de pinos quando houver
  conflito)
- Referência de implementação real (terceiro, mesma placa, em produção):
  https://github.com/trailcurrentoss/TrailCurrentFireside (LVGL + MQTT,
  ESP-IDF v5.5.2, usa o BSP oficial acima + esp_hosted)
- ESP-Hosted (link P4<->C6): componente `espressif/esp_hosted` v2.12.9
  - Exemplo de OTA do slave sem hardware extra:
    managed_components/espressif__esp_hosted/examples/host_performs_slave_ota
```

## Premissa crítica: ESP32-P4 não tem Wi-Fi nativo

- O **ESP32-P4 não possui Wi-Fi/Bluetooth nativo**.
- Nesta placa, a conectividade vem do coprocessador **ESP32-C6** via
  **ESP-Hosted/SDIO** (confirmado, ver seção acima - não é C5).
- Consequência: **toda** comunicação de rede passa pelo link **P4 ↔ C6** —
  HTTPS, WebSocket, NTP, APIs externas e quaisquer dispositivos/serviços LAN
  futuros.

## Fase 0 - Risk Gates (fechada)

A Fase 0 tratou a rede como **risco de hardware**, não como detalhe posterior, e
foi fechada com sucesso em 2026-06-26. Gates 1-16 estão PASS em
`docs/FASE0-CHECKLIST.md`.

```text
- firmware do C6 / ESP-Hosted (vem pré-flashado? como atualizar?)
- link P4 ↔ C6 via SDIO
- Wi-Fi real
- HTTPS básico
- NTP
- WebSocket simples (prova de futuro; não bloqueante p/ MVP)
- SD card
- SD + Wi-Fi simultâneo
- PSRAM detectada
- framebuffer em PSRAM
- display
- touch
- backlight
- RTC (existe? qual chip? mantém hora sem energia?)
- particionamento correto (inclui partição do slave fw do C6 e OTA futuro)
```

### Critério de saída da Fase 0

Atendido: boot e logs estáveis; Wi-Fi, HTTPS, NTP, display e touch funcionais;
PSRAM detectada; SD funcional; particionamento inicial validado; RTC decidido;
brownout/térmica validados.

## Relógio offline (ADR-0009)

A promessa de relógio offline depende de haver um RTC com bateria. Sem RTC:
mantém a hora enquanto ligado; após reboot sem internet, exibe horário como
**não sincronizado** e ressincroniza via NTP quando a rede voltar.

## Particionamento

`firmware/partitions.csv` está validado para o MVP na placa real (flash 32MB,
NVS, `nvs_keys`, `phy_init`, app factory, storage/cache e coredump). OTA/app
rollback e partição operacional do firmware do C6 seguem como evolução futura de
release, não pendência da Fase 0.

## Sensores futuros (prováveis)

```text
- LD2410C (presença mmWave) via UART
- BH1750  (luz ambiente)     via I2C
- BME280  (temp/umid/press)  via I2C
```

Ligações previstas (headers físicos confirmados na placa, itens #19/#20 da
descrição de hardware acima):

```text
PH2.0 4PIN I2C header  : BH1750, BME280 (compartilha o barramento
                         BSP_I2C_SCL=GPIO8 / BSP_I2C_SDA=GPIO7 com o touch
                         GT911 - confirmar endereços I2C sem conflito)
PH2.0 4PIN UART header : LD2410C
```

> Sensores são roadmap futuro (não entram no MVP), mas a arquitetura já reserva
> espaço para um `PresenceService`/sensores via mock até o hardware chegar.
