# NovaPainel

Smart display **local / offline-first** baseado em **ESP32-P4** (ESP-IDF + C++),
organizado como monorepo. O firmware é o núcleo do produto; o server é opcional e
futuro.

> Estado atual: **esqueleto inicial**. Arquitetura, contratos e fluxo de dados
> completos, com **mocks** no lugar de LVGL, BSP, rede e APIs reais. Sem
> funcionalidades reais ainda — por decisão (ver `docs/ROADMAP.md`).

## Estrutura

```text
NovaPainel/
├─ firmware/   # núcleo do produto (ESP-IDF/C++) - ver firmware/README.md
├─ server/     # opcional, futuro - o firmware não depende dele
├─ shared/     # contratos firmware <-> server (schemas/protocol)
├─ docs/       # documentação canônica (base v3)
└─ tools/      # scripts utilitários
```

## Por onde começar

- Visão e escopo: `docs/PLANEJAMENTO.md`
- Arquitetura e regras: `docs/ARCHITECTURE.md`
- Hardware e riscos de Fase 0: `docs/HARDWARE.md`
- Decisões (ADRs): `docs/DECISIONS.md`
- Roadmap: `docs/ROADMAP.md`

## Princípios

```text
1. A UI nunca trava e nunca faz request direto.
2. Tudo passa por StateStore/EventBus; a UI só é tocada na lvgl_task.
3. Todo request passa pelo RequestOrchestrator.
4. Dados críticos funcionam offline (cache).
5. Cada integração entra como adapter/provider.
6. O ESP32-P4 depende do ESP32-C6 (ESP-Hosted) para rede - risco de Fase 0.
```

## Build do firmware

```bash
cd firmware
idf.py set-target esp32p4
idf.py build
```

Requer ESP-IDF com suporte a ESP32-P4. Ver `firmware/README.md`.

## Licença

A definir.
