# NovaPainel - Documentacao

Este diretorio contem a documentacao canonica do projeto.

## Ordem de leitura recomendada

1. `PLANEJAMENTO.md`
2. `ARCHITECTURE.md`
3. `HARDWARE.md`
4. `ROADMAP.md`
5. `DECISIONS.md`

## Documentos centrais

| Documento | Papel |
|---|---|
| `PLANEJAMENTO.md` | visao do produto, escopo, principios e estrategia |
| `ARCHITECTURE.md` | arquitetura alvo, limites entre camadas e diretrizes de system design |
| `HARDWARE.md` | base da plataforma e achados da Fase 0 |
| `ROADMAP.md` | fases oficiais do projeto e ordem de entrega |
| `DECISIONS.md` | ADRs e decisoes duraveis |
| `PROTOCOL.md` | ponte para contratos em `shared/` |

## Documentos operacionais

| Documento | Papel |
|---|---|
| `FASE0-CHECKLIST.md` | evidencia detalhada da validacao de hardware |
| `SECURITY-OPERATIONS.md` | seguranca operacional DEV/PROD |
| `FIELD-OPERATIONS.md` | operacao e triagem de campo |
| `RELEASE-ROLLBACK.md` | estrategia de release e rollback |
| `SOAK-VALIDATION.md` | roteiro de soak e estabilidade |

## Regras de verdade

- a documentacao canonica deve refletir o estado real do projeto
- `Fase 0` e `trilha H` continuam valendo como patrimonio tecnico
- o `firmware/` novo e a base ativa de evolucao
- `firmware_legacy_reference/` e somente referencia para port seletivo

## Backup dos documentos anteriores

Os documentos antes desta consolidacao foram preservados em:

- `docs/_backup/2026-06-26-pre-consolidation/`
