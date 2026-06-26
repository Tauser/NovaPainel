# Waveshare ESP32-P4-WIFI6-Touch-LCD-7B — Mapa Técnico de Hardware

> Documento consolidado com informações de hardware, drivers, GPIOs, barramentos e observações práticas para desenvolvimento com ESP-IDF.

## 1. Identificação da placa

| Item | Informação |
|---|---|
| Modelo | ESP32-P4-WIFI6-Touch-LCD-7B |
| SKU | 32510 |
| Versão com câmera | ESP32-P4-WIFI6-Touch-LCD-7B-C |
| SKU versão com câmera | 32511 |
| Tela | 7 polegadas IPS capacitiva |
| Resolução | 1024 × 600 |
| Touch | Capacitivo, 5 pontos |
| MCU principal | ESP32-P4 / ESP32-P4NRW32 |
| Coprocessador wireless | ESP32-C6-MINI-1 |
| Wi-Fi / Bluetooth | Wi-Fi 6 2.4 GHz + Bluetooth 5 LE |
| Comunicação P4 ↔ C6 | SDIO |
| BSP oficial | `waveshare/esp32_p4_wifi6_touch_lcd_7b` |
| Target ESP-IDF | `esp32p4` |

A Waveshare recomenda o uso de **ESP-IDF** para essa placa. O suporte Arduino para ESP32-P4 ainda é limitado e pode não controlar todos os recursos com estabilidade.

---

## 2. Processamento e memória

| Recurso | Especificação |
|---|---|
| CPU HP | RISC-V 32-bit dual-core |
| Frequência HP | até 360 MHz |
| CPU LP | RISC-V 32-bit single-core |
| Frequência LP | até 40 MHz |
| HP ROM | 128 KB |
| LP ROM | 16 KB |
| HP L2MEM | 768 KB |
| LP SRAM | 32 KB |
| TCM | 8 KB |
| PSRAM | 32 MB no encapsulamento |
| Flash externa | 32 MB NOR Flash |
| Flash IC no esquemático | GD25Q256EYIGR |
| Segurança | Digital Signature Peripheral, Key Management Unit, Secure Boot, Flash Encryption, TRNG |

---

## 3. Chips e blocos principais

| Função | Componente |
|---|---|
| MCU principal | ESP32-P4NRW32 |
| Wi-Fi 6 / Bluetooth 5 LE | ESP32-C6-MINI-1 |
| Codec de áudio | ES8311 |
| Captação / ADC / echo cancel | ES7210 |
| Amplificador de áudio | NS4150B |
| Touch controller | GT911 |
| Driver/controlador LCD no BSP | EK79007 |
| USB-UART | CH343P |
| CAN transceiver | TJA1051T/3/1J |
| RS485 transceiver | THVD1406DR ou MAX13487EESA+ |
| Regulador 3V3 | MP1658GTF-Z |
| Gerência/carga de bateria | ETA6098 |
| Boost 5V bateria | SCT12A0DHKR |
| Flash externa | GD25Q256EYIGR |

---

## 4. Display / LCD

| Item | Valor |
|---|---|
| Tipo | IPS |
| Tamanho | 7 polegadas |
| Resolução | 1024 × 600 |
| Interface | MIPI-DSI |
| Data lanes | 2 |
| Bitrate MIPI DSI | 1000 Mbps |
| Brilho | 350 cd/m² |
| Contraste | 600:1 |
| Ângulo de visão | 160° |
| Formato de cor padrão | RGB565 |
| Formato opcional | RGB888 |
| Driver BSP | `esp_lcd_ek79007` |
| Backlight | PWM via LEDC |
| GPIO backlight | GPIO32 |
| Reset LCD | GPIO33 |
| LDO MIPI DSI PHY | Canal 3 |
| Tensão MIPI DSI PHY | 2500 mV |

### Definições do BSP

```c
#define BSP_LCD_H_RES (1024)
#define BSP_LCD_V_RES (600)
#define BSP_LCD_MIPI_DSI_LANE_NUM (2)
#define BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS (1000)

#define BSP_LCD_BACKLIGHT (GPIO_NUM_32)
#define BSP_LCD_RST       (GPIO_NUM_33)
```

---

## 5. Touch

| Item | Valor |
|---|---|
| Controlador | GT911 |
| Interface | I2C |
| Driver BSP | `esp_lcd_touch_gt911` |
| SDA | GPIO7 |
| SCL | GPIO8 |
| Endereço principal | 0x5D |
| Endereço alternativo | 0x14 |
| Reset touch no BSP | `GPIO_NUM_NC` |
| INT touch no BSP | `GPIO_NUM_NC` |

### Definições do BSP

```c
#define BSP_I2C_SDA        (GPIO_NUM_7)
#define BSP_I2C_SCL        (GPIO_NUM_8)
#define BSP_LCD_TOUCH_RST  (GPIO_NUM_NC)
#define BSP_LCD_TOUCH_INT  (GPIO_NUM_NC)
```

---

## 6. I2C principal

| Função | GPIO |
|---|---|
| I2C SDA | GPIO7 |
| I2C SCL | GPIO8 |

Dispositivos internos no barramento:

| Dispositivo | Uso |
|---|---|
| GT911 | Touch |
| ES8311 | Configuração do codec |
| ES7210 | Configuração do captador/ADC de áudio |
| Header I2C PH2.0 | Expansão externa |

### Header I2C PH2.0 4 pinos

| Pino lógico | Função |
|---|---|
| VCC | Alimentação, default 3V3 no esquemático |
| GND | Terra |
| SDA | GPIO7 |
| SCL | GPIO8 |

---

## 7. Áudio

| Função | GPIO |
|---|---|
| I2S MCLK | GPIO13 |
| I2S BCLK / SCLK | GPIO12 |
| I2S LRCK / WS / LCLK | GPIO10 |
| I2S DOUT | GPIO9 |
| I2S DIN / DSIN | GPIO11 |
| Power amplifier enable | GPIO53 |

Componentes:

| Componente | Função |
|---|---|
| ES8311 | Codec de áudio |
| ES7210 | Captação/ADC de microfone |
| NS4150B | Amplificador para alto-falante |
| Speaker header | PH2.0 2 pinos, sem polaridade |
| MIC1 / MIC2 | Microfones onboard |

### Definições do BSP

```c
#define BSP_I2S_SCLK     (GPIO_NUM_12)
#define BSP_I2S_MCLK     (GPIO_NUM_13)
#define BSP_I2S_LCLK     (GPIO_NUM_10)
#define BSP_I2S_DOUT     (GPIO_NUM_9)
#define BSP_I2S_DSIN     (GPIO_NUM_11)
#define BSP_POWER_AMP_IO (GPIO_NUM_53)
```

Configuração padrão do BSP quando `NULL` é passado para `bsp_audio_init()`:

| Parâmetro | Valor |
|---|---|
| Modo | I2S standard |
| Slot | Philips |
| Canais | Mono duplex |
| Bits | 16 bits |
| Sample rate | 22050 Hz |

---

## 8. MicroSD / TF Card

| Função | GPIO |
|---|---|
| SD D0 | GPIO39 |
| SD D1 | GPIO40 |
| SD D2 | GPIO41 |
| SD D3 | GPIO42 |
| SD CLK | GPIO43 |
| SD CMD | GPIO44 |
| Interface | SDMMC |
| Slot | SDMMC Slot 0 |
| Largura | 4-bit |
| Frequência | High speed |
| Alimentação | LDO interno VO4, canal 4 |

### Definições do BSP

```c
#define BSP_SD_D0  (GPIO_NUM_39)
#define BSP_SD_D1  (GPIO_NUM_40)
#define BSP_SD_D2  (GPIO_NUM_41)
#define BSP_SD_D3  (GPIO_NUM_42)
#define BSP_SD_CMD (GPIO_NUM_44)
#define BSP_SD_CLK (GPIO_NUM_43)
```

---

## 9. USB

| Porta | Uso |
|---|---|
| USB Type-A | USB OTG 2.0 High Speed / Host |
| USB Type-C USB1.1 FS | Alimentação, gravação e debug |
| USB Type-C USB-UART | Alimentação, gravação e debug via CH343P |

Sinais identificados no esquemático:

| Função | Sinal / GPIO |
|---|---|
| USB-UART CH343P | GPIO37 / GPIO38 |
| USB nativo FS | `USBD_N`, `USBD_P` |
| USB1.1 Type-C | `USB1P1_N`, `USB1P1_P` |

O BSP possui funções:

```c
esp_err_t bsp_usb_host_start(bsp_usb_host_power_mode_t mode, bool limit_500mA);
esp_err_t bsp_usb_host_stop(void);
```

---

## 10. Wi-Fi 6 / Bluetooth

| Item | Valor |
|---|---|
| Coprocessador | ESP32-C6-MINI-1 |
| Wi-Fi | Wi-Fi 6 2.4 GHz |
| Bluetooth | Bluetooth 5 LE |
| Comunicação com ESP32-P4 | SDIO |
| UART dedicada | Header/terminal para flashing do ESP32-C6 |

Observação importante: o ESP32-P4 **não possui rádio Wi-Fi/Bluetooth interno**. A conectividade wireless depende do ESP32-C6 onboard.

---

## 11. Câmera / MIPI-CSI

| Item | Valor |
|---|---|
| Interface | MIPI-CSI |
| Lanes | 2 |
| Conector | 15 pinos |
| Pitch | 1.0 mm |
| Câmeras citadas | RPi FPC Camera (B), RPi Camera (B) e outras MIPI 2-lane |
| Sinais | CSI_D0_N/P, CSI_D1_N/P, CSI_CLK_N/P |
| I2C da câmera | ESP_I2C_SCL / ESP_I2C_SDA |

---

## 12. CAN

| Item | Valor |
|---|---|
| Header | PH2.0 4 pinos |
| Transceiver | TJA1051T/3/1J |
| Sinais externos | CANH, CANL, GND, 5V |
| Sinais internos | CANTX, CANRX |
| Proteção | TVS/ESD |
| Terminador | 120 Ω indicado no esquemático |
| GPIOs relacionados no esquemático | GPIO26 / GPIO33 aparecem próximos ao bloco CAN |

Para firmware, usar o driver **TWAI** do ESP-IDF.

---

## 13. RS485

| Item | Valor |
|---|---|
| Header | PH2.0 4 pinos |
| Transceiver | THVD1406DR ou MAX13487EESA+ |
| Sinais externos | A, B, GND, 5V |
| Sinais internos | 485_TXD, 485_RXD |
| Proteção | TVS/ESD |
| Terminador | 120 Ω indicado no esquemático |
| GPIOs relacionados no esquemático | GPIO27 / GPIO32 aparecem próximos ao bloco RS485 |

Para firmware, usar UART do ESP-IDF e controlar o modo de transmissão conforme a topologia do transceiver.

---

## 14. Headers externos

### I2C PH2.0 4 pinos

| Pino | Função |
|---|---|
| 1 | VCC |
| 2 | GND |
| 3 | SDA |
| 4 | SCL |

### UART PH2.0 4 pinos

| Pino lógico | Função |
|---|---|
| TXD | UART TX |
| RXD | UART RX |
| GND | Terra |
| VCC | Alimentação |

> Observação: o esquemático mostra o header UART com TXD/RXD/GND/VCC, mas a extração textual não deixou totalmente claro o par de GPIO usado. Conferir no PDF ampliado/silk antes de usar em firmware.

### CAN PH2.0 4 pinos

| Pino lógico | Função |
|---|---|
| CANL | Linha CAN L |
| CANH | Linha CAN H |
| GND | Terra |
| 5V | Alimentação |

### RS485 PH2.0 4 pinos

| Pino lógico | Função |
|---|---|
| B | RS485 B |
| A | RS485 A |
| GND | Terra |
| 5V | Alimentação |

---

## 15. GPIOs definidos diretamente no BSP

| Função | GPIO |
|---|---|
| I2C SDA | GPIO7 |
| I2C SCL | GPIO8 |
| I2S DOUT | GPIO9 |
| I2S LCLK / LRCK / WS | GPIO10 |
| I2S DIN / DSIN | GPIO11 |
| I2S SCLK / BCLK | GPIO12 |
| I2S MCLK | GPIO13 |
| LCD backlight | GPIO32 |
| LCD reset | GPIO33 |
| SD D0 | GPIO39 |
| SD D1 | GPIO40 |
| SD D2 | GPIO41 |
| SD D3 | GPIO42 |
| SD CLK | GPIO43 |
| SD CMD | GPIO44 |
| Power amplifier enable | GPIO53 |

---

## 16. GPIOs de expansão / headers de 12 pinos

A Waveshare informa que a placa possui **2 headers PH2.0 de 12 pinos**, com **17 GPIOs programáveis restantes**.

GPIOs identificados na região dos headers/expansão pelo esquemático:

| GPIO | Observação |
|---|---|
| GPIO2 | Expansão |
| GPIO3 | Expansão |
| GPIO4 | Expansão |
| GPIO5 | Expansão |
| GPIO28 | Expansão |
| GPIO29 | Expansão |
| GPIO30 | Expansão |
| GPIO31 | Expansão |
| GPIO34 | Expansão |
| GPIO36 | Região de expansão |
| GPIO46 | Região de expansão |
| GPIO47 | Região de expansão |
| GPIO48 | Região de expansão |
| GPIO49 | Expansão |
| GPIO50 | Expansão |
| GPIO51 | Expansão |
| GPIO52 | Expansão |

> Atenção: antes de usar GPIO46/GPIO47/GPIO48/GPIO49/GPIO50/GPIO51/GPIO52 em algo crítico, validar no silk da placa e no esquemático ampliado, porque alguns aparecem próximos a blocos internos e conectores.

---

## 17. GPIOs ocupados ou reservados por funções internas

| GPIO | Uso |
|---|---|
| GPIO7 | I2C SDA |
| GPIO8 | I2C SCL |
| GPIO9 | I2S DOUT |
| GPIO10 | I2S LRCK |
| GPIO11 | I2S DIN |
| GPIO12 | I2S BCLK |
| GPIO13 | I2S MCLK |
| GPIO23 | Associação com touch/INT no esquemático |
| GPIO32 | LCD backlight / também aparece próximo ao bloco RS485 |
| GPIO33 | LCD reset / também aparece próximo ao bloco CAN |
| GPIO35 | BOOT / lógica de gravação |
| GPIO37 | USB-UART |
| GPIO38 | USB-UART |
| GPIO39 | SD D0 |
| GPIO40 | SD D1 |
| GPIO41 | SD D2 |
| GPIO42 | SD D3 |
| GPIO43 | SD CLK |
| GPIO44 | SD CMD |
| GPIO53 | Enable do amplificador |
| GPIO54 | Controle relacionado ao ESP32-C6 |

---

## 18. Alimentação e baterias

| Item | Informação |
|---|---|
| Entrada principal | USB Type-C / USB |
| Bateria principal | Conector MX1.25, polaridade direta |
| Área reservada | Espaço para bateria recarregável |
| Gerência de bateria | ETA6098 |
| Boost 5V | SCT12A0DHKR |
| RTC battery holder | Tamanho 1220 |
| Bateria RTC recomendada | ML1220 ou equivalente recarregável |
| Tensão RTC | 3 V a 3,3 V |
| Dimensão RTC | 12 mm × 2 mm |

> Importante: não usar CR1220 comum no RTC, porque a Waveshare informa suporte somente para bateria recarregável 3 V–3,3 V.

---

## 19. Drivers e componentes oficiais

| Recurso | Componente |
|---|---|
| Display | `espressif/esp_lcd_ek79007` |
| Touch | `espressif/esp_lcd_touch_gt911` |
| LVGL port | `espressif/esp_lvgl_port` |
| Áudio | `esp_codec_dev` |
| SD Card | SDMMC/FATFS do ESP-IDF |
| USB | `usb` |
| LVGL | `lvgl/lvgl >=8,<10` |
| IDF mínimo no componente | `idf >=5.3` |
| Target | `esp32p4` |

Adicionar BSP ao projeto:

```bash
idf.py add-dependency "waveshare/esp32_p4_wifi6_touch_lcd_7b^2.0.0"
```

---

## 20. Recursos suportados pelo BSP

| Recurso | Suporte |
|---|---|
| Display | Sim |
| Touch | Sim |
| LVGL Port | Sim |
| Áudio | Sim |
| Speaker | Sim |
| Microfone | Sim |
| SD Card | Sim |
| USB Host | Sim |
| Botões | Não |
| IMU | Não |

---

## 21. Recomendações práticas para o NovaPanel

Para o projeto NovaPanel, o caminho mais seguro é:

| Área | Recomendação |
|---|---|
| Framework | ESP-IDF |
| UI | LVGL |
| Display | BSP oficial + `esp_lcd_ek79007` |
| Touch | BSP oficial + `esp_lcd_touch_gt911` |
| Áudio | BSP oficial + `esp_codec_dev` |
| SD Card | `bsp_sdcard_mount()` |
| I2C sensores | GPIO7/GPIO8, compartilhado com touch/codec |
| CAN | Driver TWAI do ESP-IDF |
| RS485 | UART do ESP-IDF |
| USB Host | `bsp_usb_host_start()` |
| RTC | Bateria recarregável ML1220/1220 3V–3,3V |

## 22. Pontos de atenção

1. **Evitar Arduino no início**  
   A placa é complexa: MIPI-DSI, MIPI-CSI, áudio, SDMMC, USB Host e ESP32-C6 via SDIO. ESP-IDF é o caminho mais estável.

2. **Não reutilizar GPIOs internos sem conferir**  
   GPIOs usados por SD, áudio, LCD, backlight, touch e C6 devem ser considerados reservados.

3. **I2C é compartilhado**  
   Sensores externos podem usar GPIO7/GPIO8, mas precisam conviver com GT911, ES8311 e ES7210.

4. **CAN e RS485 já têm transceiver**  
   Não é necessário adicionar módulo externo para CAN/RS485, apenas usar os headers adequados e configurar o driver.

5. **A bateria RTC deve ser recarregável**  
   Usar ML1220 ou equivalente. Não usar CR1220 comum.

---

## 23. Fontes consultadas

- Documentação oficial Waveshare — ESP32-P4-WIFI6-Touch-LCD-7B  
- Página de recursos da Waveshare com esquemático PDF  
- Esquemático oficial `ESP32-P4-WIFI6-Touch-LCD-7B.pdf`  
- ESP Component Registry — `waveshare/esp32_p4_wifi6_touch_lcd_7b`  
- BSP oficial Waveshare em `waveshareteam/Waveshare-ESP32-components`

