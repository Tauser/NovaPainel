# NovaPanel — Documentação canônica (baseline v4)

Este diretório é a fonte de verdade do projeto. Regras do conjunto:

1. **`STATUS.md` é o único documento de estado.** Todos os demais descrevem
   alvo, política ou decisão. Nenhum outro doc pode conter "feito",
   "em produção" ou percentuais de progresso.
2. Todo documento tem um dono de propósito (abaixo). Conteúdo duplicado entre
   docs é bug — linkar, não copiar.
3. Mudança estrutural sem ADR em `DECISIONS.md` não é aceita em review.
4. Documento histórico do baseline v3 vive no histórico git e em
   `reference/`; nunca é citado como estado atual.

## Mapa e ordem de leitura

| Doc | Propósito (dono de) |
|---|---|
| `STATUS.md` | Estado atual, fase corrente, dívidas abertas |
| `PLANEJAMENTO.md` | Produto: visão, escopo, requisitos de qualidade, não-objetivos |
| `ARCHITECTURE.md` | Arquitetura alvo: camadas, contratos, concorrência, dados |
| `UI-PATTERN.md` | Padrão obrigatório de telas/tiles e view-model |
| `RESOURCE-BUDGET.md` | Contrato físico: SRAM/PSRAM/MSPI/flash — limites e regras |
| `HARDWARE.md` | Fatos da placa, pinos, gotchas validados |
| `ROADMAP.md` | Fases, entregas e critérios de saída |
| `DECISIONS.md` | ADRs do baseline v4 (histórico de raciocínio) |
| `TESTING.md` | Pirâmide de testes, fixtures, CI, soak |
| `OPERATIONS.md` | Observabilidade, release, OTA, rollback, segurança PROD |

Referência técnica bruta da placa (datasheet-level, herdada e ainda válida):
`waveshare_esp32_p4_wifi6_touch_lcd_7b_hardware.md`.

## Ordem de leitura para quem chega agora

`STATUS.md` → `PLANEJAMENTO.md` → `ARCHITECTURE.md` → `ROADMAP.md` → resto
conforme a tarefa.
