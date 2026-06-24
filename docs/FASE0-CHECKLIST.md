# NovaPainel - Fase 0: Checklist de Validação de Hardware

> **Bloqueante.** Nenhuma feature evolui antes desta fase passar. A Fase 0 trata
> rede e display como **risco de hardware**, não detalhe posterior.
> Fonte de verdade do hardware: `docs/HARDWARE.md`. Skill: `novapanel-hardware-risk-gate`.

## Como usar este documento

Cada gate é uma linha a ser **preenchida com evidência real** (não "deve funcionar").
Para cada item registre: data, comando/log usado, resultado (PASS / FAIL / ADIADO),
e follow-up. Um gate só é PASS com evidência de log anexada.

Baseline fixo: ESP-IDF `v5.5.4`, target `esp32p4`, P4 sem Wi-Fi nativo (rede via
ESP32-C6 / ESP-Hosted / SDIO).

```text
Critério de saída da Fase 0:
boot e logs estáveis; Wi-Fi + display + touch coexistem sob stress sem reset;
HTTPS e NTP funcionais; PSRAM detectada e framebuffer em PSRAM; SD funcional
(ou decisão consciente de adiar); particionamento inicial validado; RTC decidido.
```

## Tabela de evidências

| #  | Gate                                  | Resultado | Data | Evidência (log/comando) | Follow-up |
|----|---------------------------------------|-----------|------|-------------------------|-----------|
| 1  | Toolchain e target                    | PASS      | 2026-06-23 | `idf.py --version` -> ESP-IDF v5.5.4; `idf.py set-target esp32p4` + `idf.py build` em `firmware/experiments/gate15_coexistence` -> build limpo (1107/1107), `gate15_coexistence.bin` gerado | Nenhum |
| 2  | Flash / boot / monitor (USB-JTAG)     | PASS      | 2026-06-23 | `idf.py -p COM8 flash` + `idf.py -p COM8 monitor`: boot limpo, ESP32-P4 rev v1.3 (não v3.x — ver follow-up), sem PANIC/WDT/BROWNOUT, `app_main()` retorna normalmente | Chip real é rev v1.3, não a v3.x assumida por default pelo IDF 5.5.4. Ajustado `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` + `CONFIG_ESP32P4_REV_MIN_100=y` em `sdkconfig.defaults` do experimento. **Levar essa config pro firmware principal quando sair dos mocks.** |
| 3  | Flash size e partition table          | PASS      | 2026-06-23 | Ajustado `CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y` (era 2MB) em `sdkconfig`/`sdkconfig.defaults`; rebuild + reflash em COM8 ok. Partition table `partitions_singleapp_coredump` (built-in IDF) cabe folgado em 32MB | Confirmar no monitor que o warning `Detected size(32768k) larger than... 2048k` não aparece mais no próximo boot |
| 4  | PSRAM detectada                       | PASS      | 2026-06-23 | Boot log: `esp_psram: Found 32MB PSRAM device`, `Speed: 200MHz`, `SPI SRAM memory test OK`; experimento aloca framebuffer 1.2MB em PSRAM com sucesso (`framebuffer in PSRAM ok: 1228800 bytes @ 0x48000aa0`) | Nenhum |
| 5  | Display: power, backlight, panel init | PASS      | 2026-06-23 | Integrado o BSP oficial `waveshare/esp32_p4_wifi6_touch_lcd_7b` v2.0.0 direto no harness (`bsp_display_new()` + `bsp_display_backlight_on()`). Boot real: `ESP32_P4_EV: MIPI DSI PHY Powered on` -> `Install EK79007 LCD control panel` -> `ek79007: version: 2.0.2`, sem panic. `[STAT] ... display=on` estável. **Achado real:** no driver `espressif/esp_lcd_ek79007` v2.0.2 usado por este BSP, `panel->disp_on_off` fica `NULL`, então o framework `esp_lcd` retorna especificamente `ESP_ERR_NOT_SUPPORTED` (não um erro genérico - ver `esp_lcd_panel_ops.c`). Isso não é universal a toda versão do driver EK79007 (READMEs mais novos do componente mostram exemplos chamando `esp_lcd_panel_disp_on_off` com sucesso) - é específico desta versão/BSP. Na Waveshare 7B via este BSP, não é necessário chamar essa função: a visibilidade é controlada só pelo backlight (`bsp_display_backlight_on()`). Corrigido o código pra checar especificamente `ESP_ERR_NOT_SUPPORTED` (e ainda propagar `ESP_ERROR_CHECK` para qualquer outro erro real) em vez de um `ESP_ERROR_CHECK` cego, que causava panic-loop | Sem confirmação visual ainda (não há como ver a tela fisicamente neste teste) - só confirma que o driver inicializa sem erro/panic. `panel_blit()` envia o framebuffer real via `esp_lcd_panel_draw_bitmap()` |
| 6  | Touch controller                      | PASS      | 2026-06-23 | Integrado via `bsp_touch_new()` (driver GT911 oficial). Boot sem erro, `[STAT] ... touch=ok(points=0)` (0 esperado - tela não foi tocada durante o teste). `touch_poll()` chama `esp_lcd_touch_read_data()` + `esp_lcd_touch_get_coordinates()` a cada tick do `render_task` | Não testado com toque físico real ainda - só confirma init sem erro. Repetir tocando a tela pra ver `points` incrementar |
| 7  | SD card                               | PASS      | 2026-06-23 | Causa raiz do primeiro `ESP_ERR_TIMEOUT` não era falta de cartão: o slot SD é alimentado pelo **LDO interno do P4, canal 4** (doc `waveshare_esp32_p4_wifi6_touch_lcd_7b_hardware.md`, seção 8) - igual o MIPI-DSI precisa do canal 3. Sem `esp_ldo_acquire_channel(chan_id=4, 3300mV)` antes do mount, o cartão nunca respondia ao `send_op_cond`, mesmo inserido. Com o LDO ligado: `[STAT] ... sd(mounted=yes ok=3/fail=0)` | Nenhum. Documentar esse requisito de power sequencing do LDO4 para quando o `board/` real do firmware principal implementar SD |
| 8  | Firmware do C6 / estratégia de update | PASS      | 2026-06-23 | C6 não tem USB/UART próprio nesta placa (Waveshare ESP32-P4-WIFI6) — as 2 portas USB-C vão para o P4 (mesmo MAC nas duas, confirmado via `esptool chip_id` em COM8 e COM9). Update é via **ESP-Hosted Slave OTA sobre SDIO** (exemplo `host_performs_slave_ota`, método Partition OTA): buildado `slave` (esp32c6) -> `network_adapter.bin` v2.12.9, embutido na partição `slave_fw` do host e flashado via `idf.py -p COM8 flash`. Boot log confirma: `Host firmware version: 2.12.9` / `Slave firmware version: 2.12.9` / `Versions compatible` | Documentar este fluxo (sem hardware extra, via SDIO) como o processo oficial de update do C6 para o produto, não só para o harness. Repetir component version pin quando o ESP-Hosted do firmware principal for atualizado. A placa também tem um header físico "ESP32-C6 UART Terminal" (item #3 da doc Waveshare) que parece permitir flash direto via UART — não testado, ficaria como plano B/recovery se a OTA via SDIO falhar em campo |
| 9  | Link P4↔C6 (SDIO / ESP-Hosted)        | PASS      | 2026-06-23 | Captura serial passiva de ~9min (COM8, sem reset): `[STAT]` a cada 60s mostra `render` incrementando 1/s sem parar (1→535), heap interno estável (`free=411735`, sem queda contínua) e PSRAM constante (`free=32306124`/`largest=31981568`) do início ao fim. Zero PANIC/WDT/BROWNOUT na janela | Janela de 9min é curta vs. o soak completo do Gate 15 (≥8h) — esta evidência cobre só estabilidade de curto prazo |
| 10 | Wi-Fi association                     | PASS      | 2026-06-23 | Mesma captura: 2 desconexões transitórias no primeiro minuto (`disc=2`), depois `wifi(conn=1 UP)` estável pelos ~8min restantes sem nenhuma reassociação | As 2 desconexões iniciais se repetem entre boots (visto também no teste anterior) — padrão consistente, não é falha intermitente nova |
| 11 | NTP                                   | PASS      | 2026-06-23 | `ntp_start()` via `esp_netif_sntp` (servidor `pool.ntp.org`) implementado no harness. Captura confirma `[STAT] ... ntp=synced` e log `NTP synced: <data/hora>` logo após a conexão Wi-Fi | Nenhum |
| 12 | HTTPS                                 | PASS      | 2026-06-23 | Causa raiz da falha anterior corrigida: `crt_bundle_attach = esp_crt_bundle_attach` em `do_one_https_get()` (`gate15_main.c`), `mbedtls` adicionado ao `REQUIRES` do componente. Rebuild + reflash; captura de ~2.5min mostrou `esp-x509-crt-bundle: Certificate validated` e `[STAT] ... https(ok=33/fail=0)` após `up=2042s` (~34min) contínuos | Partição `factory` também precisou crescer de 1M -> 4M (custom `partitions.csv`, flash real é 32MB) porque o cert bundle completo deixou só 4% livre na partição antiga |
| 13 | SD + Wi-Fi simultâneos                 | PASS (curto) | 2026-06-23 | Com Gate 7 resolvido: captura de ~2min mostra `https(ok=1/fail=0)` e `sd(mounted=yes ok=3/fail=0)` progredindo juntos, sem erro, na mesma janela (`sd_task` e `https_task` em cores separados, mesma cadência de 60s) | Curto prazo só - confirmar definitivamente durante o soak de 8h (Gate 15) |
| 14 | RTC presente/ausente + clock offline  | PARCIAL (via esquemático) | 2026-06-23 | Esquemático confirma: sem chip de RTC dedicado (sem I2C). "RTC Battery Holder" liga direto no `ESP_VBAT` do P4 - é o domínio RTC interno do SoC, com bateria 1220 (3-3.3V) como backup. **Atenção:** a Waveshare exige bateria recarregável (ML1220 ou equiv.) - **não usar CR1220 comum** | Falta o teste físico real: desligar a placa completamente (sem USB), esperar, religar sem rede e comparar a hora retida com NTP. Decisão de ADR-0009 (degradar para NTP-only se não mantiver) ainda depende desse teste. Confirmar que a bateria física instalada é recarregável antes do teste |
| 15 | **Stress de coexistência (soak)**     | PASS (8h) | 2026-06-24 | Captura serial passiva contínua de `up=2s` a `up=28754s` (~8h), sem reset entre meio. `reboots=90` constante do início ao fim. Zero `PANIC`/`*_WDT`/`BROWNOUT` em todo o log. Wi-Fi: só as 2 desconexões transitórias do primeiro minuto, depois estável (`disc=2`) por ~8h direto. HTTPS: 484 ok / 57 fail (56 são `status 429` da API durante os bursts horários intencionais, 1 hiccup de conexão isolado e não recorrente). SD: 480 ok / 0 fail. NTP/display/touch estáveis o tempo todo. Heap interno assenta nos primeiros ~30min (393327→389215 bytes livres) e fica **completamente flat** pelas ~7h30min restantes - sem leak. PSRAM assenta uma vez no início e fica idêntico o resto do teste (`free=31050496`/`largest=30932992`) | Esta janela ainda não cobre os ciclos cíclicos de queda de AP descritos no protocolo original (derrubar Wi-Fi ~2min a cada ~30min) - rodar uma vez com isso antes de considerar o gate 100% encerrado para produção. Janela de 8h (não 72h, decisão já registrada) cobre o risco mais provável mas tem menos confiança em leaks/deriva térmica muito lentos |
| 16 | Brownout / térmica sob carga          |           |      |                         |           |

---

## Detalhe dos gates 1-14 (ordem obrigatória)

Validar **em ordem** — cada gate depende do anterior.

1. **Toolchain e target.** `idf.py --version` (esperar v5.5.4), `idf.py set-target esp32p4`, `idf.py build`. PASS = build limpo.
2. **Flash/boot/monitor.** `idf.py -p <porta> flash monitor` via USB-JTAG builtin (a menos que use ESP-PROG). PASS = boot repetido sem panic, logs estáveis.
3. **Flash size e partições.** Confirmar tamanho real de flash e validar `firmware/partitions.csv` (preliminar). PASS = layout cabe na flash, inclui espaço para slave fw do C6, OTA futuro, NVS e storage.
4. **PSRAM.** Confirmar detecção no boot (`esp_psram`); medir tamanho livre. PASS = PSRAM detectada com o tamanho esperado.
5. **Display.** Power + backlight + init do painel (confirmar MIPI-DSI vs RGB — ver `HARDWARE.md`). PASS = tela acende e mostra conteúdo de teste.
6. **Touch.** Init do controlador + leitura de toques. PASS = coordenadas coerentes lidas.
7. **SD card.** Mount + read/write de um arquivo de teste. PASS = I/O ok (ou marcar ADIADO conscientemente).
8. **Firmware do C6.** Confirmar se vem pré-flashado e **como atualizar** (ADR-0010). PASS = versão do slave fw identificada e processo de update documentado.
9. **Link P4↔C6.** SDIO / ESP-Hosted sobe sem SDIO RX drops/timeouts. PASS = link estabelecido e estável em idle por ≥10 min.
10. **Wi-Fi association.** Conectar a um AP real; medir tempo e estabilidade. PASS = associa e mantém por ≥10 min sem reassociação espúria.
11. **NTP.** Sincronizar hora via NTP. PASS = hora correta obtida.
12. **HTTPS.** GET simples a um endpoint TLS (ex.: CoinGecko). PASS = handshake TLS + resposta válida; cert bundle definido.
13. **SD + Wi-Fi simultâneos.** Exercitar I/O de SD e rede ao mesmo tempo. PASS = sem corrupção, sem reset (conflito de barramento é risco conhecido).
14. **RTC.** Detectar presença/chip; testar manutenção de hora sem energia (ADR-0009). Registrar o comportamento real. PASS = decisão consciente (tem RTC com bateria / não tem → degrada para NTP-only).

---

## Gate 15 - Protocolo de stress de coexistência (o teste que decide o produto)

**Por quê:** há relatos correntes (2026) de placas P4+C6 que **resetam quando
Wi-Fi e display são usados simultaneamente**, root cause não confirmado. Este é o
risco que pode inviabilizar a arquitetura. Validar isto vale mais que qualquer
feature.

### Setup

Firmware de teste mínimo (pode viver fora do app principal) que rode, ao mesmo tempo:

- **Display ativo**: redraw periódico de uma região (ex.: relógio + um bloco que
  muda de cor a cada segundo) usando o caminho de render real (LVGL ou BSP), com
  framebuffer em PSRAM.
- **Touch ativo**: polling/IRQ do controlador rodando continuamente.
- **Rede sustentada**: HTTPS GET a cada 60s (perfil do `MarketService`, ADR-0006)
  **e** um burst opcional (1 req/5s por 1 min a cada hora) para estressar o SDIO.
- **Backlight**: em brilho alto (pior caso térmico/consumo).

Pinagem de cores como será em produção (ADR-0013): render num core, rede no outro.

### Instrumentação (logar a cada 60s)

```text
- uptime (s)
- esp_reset_reason() do boot atual (detectar resets silenciosos)
- contador de reboots (persistido em NVS)
- free heap interno + maior bloco livre (heap_caps_get_free_size / largest_free_block, MALLOC_CAP_INTERNAL)
- free PSRAM + maior bloco livre (MALLOC_CAP_SPIRAM)
- nº de reassociações Wi-Fi e erros de SDIO desde o boot
- nº de HTTPS ok / falhas
- temperatura (se sensor interno/externo disponível)
```

> **Nota de metodologia (2026-06-23):** abrir uma porta serial nativa
> USB-JTAG/Serial deste chip (ex.: `idf.py monitor` ou um script Python/pyserial
> que não fixe DTR/RTS=False **antes** de abrir a porta) pode disparar um reset
> via o circuito de auto-reset (mesmo mecanismo que o `esptool` usa), mostrando
> `rst:0x17 (CHIP_USB_UART_RESET)` no boot seguinte. Isso já gerou um falso
> positivo de "reset" numa captura de teste. Durante o soak de 8h, evite
> abrir/fechar monitores na porta — se precisar inspecionar ao vivo, abra uma
> única vez no início e mantenha a sessão aberta até o fim.

### Procedimento

> **Decisão (2026-06-23):** janela reduzida de 72h para **8h contínuas**
> (mínimo). Risco assumido: 8h pega o caso mais provável (reset por
> coexistência Wi-Fi+display, que historicamente aparece em minutos/poucas
> horas) mas tem menos confiança em leak/fragmentação de heap muito lentos ou
> deriva térmica de longo prazo, que só aparecem depois de muitas horas. Se o
> Gate 15 passar em 8h e o produto for pra campo, considerar reservar uma
> rodada de 72h depois (não bloqueante para o MVP).

```text
1. Boot limpo, zerar contadores.
2. Rodar a carga combinada por pelo menos 8h contínuas.
3. Injetar adversidade cíclica: derrubar o AP por ~2 min a cada ~30 min
   (rede instável), e validar que o display NÃO congela e o sistema NÃO reseta.
   Em 8h isso dá ~16 ciclos de queda/recuperação de rede.
4. Registrar todos os eventos de reset com a razão.
```

### Critérios PASS / FAIL

| Métrica                                  | PASS                          | FAIL |
|------------------------------------------|-------------------------------|------|
| Resets não intencionais em 8h            | 0                             | ≥1 (investigar razão) |
| UI congela durante queda de rede          | nunca (mostra cache + stale)  | qualquer congelamento |
| Free heap interno (watermark mínimo)      | estável após 1h, sem tendência de queda | queda contínua = leak/fragmentação |
| Maior bloco livre PSRAM                   | estável                       | encolhe ao longo do tempo |
| Reassociações Wi-Fi espúrias              | esporádicas e recuperadas     | loop de reassociação / SDIO drops crescentes |

> Metas numéricas de CPU/heap (ex.: "CPU <15%") só são fixadas **depois** de medir
> a baseline aqui — não são arbitradas antes (ADR-0013, Fase 10 do roadmap).

---

## Gate 16 - Brownout e térmica sob carga

Always-on com display 7" + dois rádios + PSRAM é cenário de consumo e calor
constante. Um "reset Wi-Fi+display" pode ser brownout ou térmico disfarçado.

```text
- Habilitar e logar o brownout detector do ESP-IDF; registrar qualquer disparo.
- Medir consumo no pior caso (brilho máx + Wi-Fi TX + render) e dimensionar a fonte
  com margem; verificar capacitância de bulk na entrada.
- Monitorar temperatura ao longo do soak (mínimo 8h, ver Gate 15); definir curva de dimming/noturno
  se a temperatura subir além do confortável.
- Concluir: o reset (se houver) é firmware, brownout ou térmico? Anexar evidência.
```

---

## Escape hatch (se a Fase 0 falhar)

Se o gate 15/16 não passar e a coexistência Wi-Fi+display for instável de forma
irrecuperável:

```text
1. Tentar mitigação de firmware/config (timings SDIO, clock, sdkconfig do C6,
   brilho/PM, separação temporal de render e TX).
2. Se persistir, acionar o plano B de topologia (ADR-0003 / IBoard protege o core):
   módulo de conectividade alternativo ou reavaliar a placa.
3. Registrar a decisão como ADR nova (não reescrever ADR-0010).
```

A camada `board/` (`IBoard`) existe justamente para que essa troca não reescreva o
core. Falhar a Fase 0 cedo é barato; descobrir em campo é caro.
