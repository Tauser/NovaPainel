# NovaPainel - Field Operations

Playbook operacional da trilha H6.

> Leitura correta no baseline atual:
>
> Este documento registra como a operacao de campo deve funcionar no projeto.
> Parte das informacoes abaixo ja foi validada como patrimonio tecnico, mas a
> instrumentacao correspondente ainda pode depender de reentrega no
> `firmware/` novo antes de existir exatamente desse jeito no codigo ativo.

Define como coletar falhas em campo sem perder contexto de reset, cache, heap e
estado de conectividade.

## Objetivo

Quando uma unidade falhar fora da bancada, precisamos responder quatro
perguntas rapidamente:

- qual foi o motivo do reset
- havia conectividade real ou so o link P4<->C6
- o dado exibido era live ou cache
- existe coredump ou sintoma repetivel para reproduzir

## Status no projeto

- playbook de referencia: **consolidado**
- operacao de campo do baseline novo: **parcial, depende da Fase 6**

## Checklist de triagem

1. Abrir a `SystemScreen`, quando disponivel no firmware ativo, e registrar:
   - `reset_reason`
   - `reboot_count`
   - flags `board/display/touch/network/sd/cache`
   - idade do dado de clima, BTC e USD/BRL
2. Confirmar se o problema ocorreu em build DEV ou PROD.
3. Confirmar se a unidade estava com Wi-Fi conectado ou apenas com o transporte ESP-Hosted ativo.
4. Verificar se o comportamento persiste apos reboot simples.

## Coleta em DEV

Em DEV, `coredump` em flash fica habilitado por design.

Fluxo:

```bash
idf.py coredump-info
```

Registrar junto com o dump:

- hash/versao do firmware
- horario aproximado da falha
- tela/fluxo em uso
- se havia onboarding em andamento
- se o problema ocorreu logo apos boot, apos reconnect ou apos horas de uso

## Coleta em PROD

Em PROD, `coredump` fica desativado para reduzir risco de segredos em memoria.
O diagnostico deve se apoiar em:

- `SystemScreen`
- contagem de reboots
- motivo do reset
- reproduzibilidade
- logs WARN do serial, quando houver acesso controlado

Se uma classe de falha exigir dump persistido em producao, isso deve reabrir a
avaliacao de risco de segredos antes de qualquer mudanca de build.

## Snapshot minimo de health

Registrar estes campos em qualquer incidente:

- versao do firmware
- build DEV ou PROD
- `reset_reason`
- `reboot_count`
- `wifi_status`
- `network_ready`
- `cache_ready`
- idades dos dados por dominio
- se o dado visivel estava marcado como cache

## Heap e performance

Sempre que um bug parecer relacionado a tempo de uso, reconnect ou travamento
de UI, registrar tambem:

- uptime aproximado
- se houve scan Wi-Fi recente
- se a tela estava em `Home`, `Setup`, `Settings` ou `System`
- watermark/minimo observado de heap/PSRAM, quando a instrumentacao existir

## Relacao com a trilha H

- `H6`: fechado por este playbook
- `H7`: depende desta coleta para decidir rollback ou bloqueio de release
- `H8`: complementa com roteiro de soak e metas de estabilidade

## Relacao com o roadmap atual

- este documento e a referencia operacional da **Fase 6 - Observabilidade e
  resiliencia de operacao**
- enquanto `SystemScreen`, diagnostico e instrumentacao nao estiverem
  reentregues no `firmware/` novo, este playbook deve ser lido como alvo
  operacional, nao como retrato literal de tudo que o tree ativo ja fornece
