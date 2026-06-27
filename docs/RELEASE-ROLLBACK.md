# NovaPainel - Release and Rollback

Playbook da trilha H7.

> Leitura correta no baseline atual:
>
> Este documento define a estrategia de release segura do projeto. A politica
> continua valida, mas sua aplicacao operacional no `firmware/` novo depende da
> reentrega das fases funcionais e da **Fase 7** do roadmap consolidado.

Define a estrategia de release segura para o firmware do P4 e o que continua
explicitamente futuro para OTA/firmware do C6.

## Estado atual

Hoje o projeto tem base de seguranca para release:

- `sdkconfig.prod`
- Secure Boot v2
- Flash Encryption em RELEASE
- NVS Encryption
- particao `nvs_keys`

Mas ainda nao tem pipeline OTA operacional do app nem politica final de
rollback automatizado em campo. Este documento fixa o plano para essa etapa.

No repositorio atual, isso deve ser lido como:

- politica de release: **definida**
- patrimonio tecnico da trilha H7: **consolidado**
- fluxo de release do `firmware/` novo: **ainda nao encerrado**

## Estrategia recomendada

Fase atual de release:

- manter `factory` como imagem de producao principal
- usar release cabeada/controlada enquanto OTA nao existir
- tratar rollback como procedimento operacional de reflash controlado

Fase futura:

- migrar `partitions.csv` para esquema com `otadata`, `ota_0` e `ota_1`
- manter imagem anterior assinada para rollback
- versionar `security_version` junto do processo de release

Planejamento tecnico recomendado para essa migracao:

- manter `nvs`, `nvs_keys`, `phy_init` e `coredump`
- introduzir `otadata`
- substituir `factory` unico por `ota_0` e `ota_1`
- validar espaco de app, cache e coredump antes de qualquer corte final
- integrar `esp_ota_ops` somente junto do fluxo completo de validacao,
  confirmacao e rollback

Layout alvo de referencia:

```text
nvs | nvs_keys | otadata | phy_init | ota_0 | ota_1 | littlefs | coredump
```

## Regra de rollback

Rollback so e permitido para imagem:

- assinada
- compativel com o `security_version` vigente
- compativel com o schema de NVS/cache suportado pela release alvo

Rollback nunca deve rebaixar uma unidade abaixo da politica de anti-rollback
definida para producao.

## Firmware do ESP32-C6

Estado atual:

- o caminho validado e `ESP-Hosted Slave OTA via SDIO`
- o projeto ainda nao reservou particao operacional dedicada para o firmware do C6
- isso continua como trabalho de release futuro, nao bloqueio do MVP validado

Quando essa fase entrar:

- definir artefato/versionamento do firmware do C6
- definir compatibilidade P4<->C6 por release
- documentar janela de update, fallback e criterio de bloqueio

## Gate de release

Antes de marcar uma release como pronta:

- build PROD assinado gerado
- checklist de `docs/SECURITY-OPERATIONS.md` concluido
- `host_check --app --tests` verde
- `idf.py build` verde
- roteiro de soak relevante executado
- nenhum bug aberto de reboot, perda de Wi-Fi ou corrupcao de persistencia sem dono

## Relacao com a trilha H

- `H7`: fechado como plano operacional
- a implementacao tecnica de OTA fica para uma fase futura de firmware/release

## Panic, coredump e diagnostico

No baseline atual, o caminho robusto para falhas criticas deve continuar sendo:

- `reset_reason` persistido no boot seguinte
- `reboot_count` persistido em NVS
- particao `coredump` preservada para coleta em campo

Nao foi adotado um hook customizado de panic no `firmware/` novo nesta rodada,
porque gravar backtrace manualmente no caminho de panic aumenta risco
operacional se nao vier junto de validacao especifica do ESP-IDF e do layout de
flash final. A estrategia recomendada e consolidar primeiro OTA/particoes e
usar o fluxo oficial de coredump + diagnostico de boot como base de operacao.

## Relacao com o roadmap atual

- este documento serve de base para a **Fase 7 - Hardening de release e
  seguranca PROD**
- nao deve ser interpretado como sinal de que OTA, rollback de campo ou release
  final ja estao implementados no baseline novo
