# NovaPainel

Smart display **local / offline-first** baseado em **ESP32-P4**, com firmware em
**ESP-IDF + LVGL**, arquitetura modular e expansao futura para automacao,
sensores, midia, voz e integracao opcional com o ecossistema NoiseBot.

## Estado atual

Leitura correta do repositorio em 2026-06-26:

- `Fase 0` de hardware esta concluida
- a trilha tecnica/documental pos-Fase 0 esta consolidada
- o `firmware/` ativo esta em reboot controlado: shell novo, base de core e
  parte da UI ja portadas, enquanto as fases funcionais ainda estao sendo
  reentregues no tree novo

## Estrutura

```text
NovaPainel/
├─ firmware/   # firmware ativo do produto
├─ firmware_legacy_reference/  # referencia congelada para port seletivo
├─ server/     # opcional e futuro
├─ shared/     # contratos e schemas compartilhados
├─ docs/       # documentacao canonica
└─ tools/      # scripts utilitarios
```

## Principios do projeto

```text
1. O firmware funciona sem server.
2. A UI nao faz request direto.
3. Todo request passa pelo RequestOrchestrator.
4. Dados importantes degradam para cache antes de quebrar a UX.
5. O projeto cresce por modulos, nao por acoplamento cruzado.
6. Display, touch e rede precisam coexistir de forma confiavel na plataforma.
```

## Documentos para começar

- [Planejamento](D:\Projetos\NovaPanel\docs\PLANEJAMENTO.md)
- [Arquitetura](D:\Projetos\NovaPanel\docs\ARCHITECTURE.md)
- [Hardware](D:\Projetos\NovaPanel\docs\HARDWARE.md)
- [Roadmap](D:\Projetos\NovaPanel\docs\ROADMAP.md)
- [Decisoes](D:\Projetos\NovaPanel\docs\DECISIONS.md)

## Build do firmware

```bash
cd firmware
idf.py set-target esp32p4
idf.py build
```

## Direcao de engenharia

O objetivo do projeto nao e apenas "funcionar", e sim evoluir com:

- escalabilidade de arquitetura
- robustez operacional
- confiabilidade em campo
- facilidade de manutencao
- documentacao limpa e rastreavel

## Backup documental

Os documentos anteriores a consolidacao atual foram preservados em:

- `docs/_backup/2026-06-26-pre-consolidation/`
