# NovaPainel - Security Operations

Guia operacional da trilha H3. Consolida o uso de `firmware/sdkconfig.prod`,
`firmware/partitions.csv` e as ADRs `0011`/`0030` para evitar erro humano em
build, provisionamento e manutencao de unidades de producao.

## Objetivo

Em DEV, o projeto deve continuar facil de compilar, depurar e reflashear.
Em PROD, o projeto deve proteger credenciais, rejeitar firmware nao assinado e
reduzir exposicao de segredos por log, dump ou acesso fisico ao flash.

## DEV x PROD

```text
DEV
- Comando normal: idf.py build
- Usa so sdkconfig.defaults
- Secure Boot: off
- Flash Encryption: off
- NVS Encryption: off
- Coredump em flash: on
- Log default: INFO
- Reflash plaintext: permitido

PROD
- Comando: idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.prod" build
- Secure Boot v2: on
- Flash Encryption (RELEASE): on
- NVS Encryption: on
- Coredump em flash: off
- Log default: WARN
- Reflash plaintext apos 1o boot: proibido
```

## Artefatos sensiveis

- `firmware/secure_boot_signing_key.pem`: nunca versionar, nunca copiar para o repo, nunca deixar em compartilhamento solto.
- eFuses gravadas em producao: operacao irreversivel; tratar a placa como unidade de producao apos essa etapa.
- `nvs_keys`: particao usada pelo ESP-IDF quando `CONFIG_NVS_ENCRYPTION=y`; nao precisa de transporte manual de chave.

## Build de producao

Build de release:

```bash
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.prod" build
```

Override explicito da chave de assinatura:

```bash
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.prod" \
       -DCONFIG_SECURE_BOOT_SIGNING_KEY="C:/segredos/novapanel/secure_boot_signing_key.pem" \
       build
```

Verificacoes antes de publicar o binario:

- confirmar que o build usou `sdkconfig.prod`
- confirmar que a chave de assinatura veio de fora do repo
- confirmar que nenhum segredo foi adicionado a log, script ou README
- confirmar que `CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=n` ficou ativo no build PROD

## Provisionamento de unidade PROD

Sequencia recomendada por unidade:

1. Buildar o firmware PROD assinado.
2. Gravar a digest key de Secure Boot v2 na eFuse da unidade.
3. Flashear a imagem de producao.
4. Fazer o primeiro boot da unidade para que o dispositivo gere a chave de Flash Encryption.
5. Registrar numero da unidade, versao do firmware e status do provisionamento.
6. Para reflashes posteriores, usar apenas fluxo de imagem criptografada.

Comando de eFuse citado pelo projeto:

```bash
idf.py efuse-burn-key BLOCK_KEY0 secure_boot_signing_key.pem SECURE_BOOT_DIGEST0
```

Notas operacionais:

- essa etapa e irreversivel
- nao executar em placa de bancada/dev
- apos o primeiro boot com Flash Encryption RELEASE, o fluxo muda para `idf.py encrypted-flash`

## Politica de logs e segredos

- nunca logar senha Wi-Fi, token, chave de API ou payload que os contenha
- evitar dumps textuais de structs que possam carregar credenciais
- em DEV, `coredump` e log INFO existem para diagnostico; em PROD, ambos devem minimizar vazamento
- qualquer novo codigo que toque credenciais deve ser auditado antes de release

`SetupService` e o principal ponto atual de cuidado, pois persiste SSID/senha em NVS.

## Checklist de release

- `sdkconfig.prod` revisado
- `partitions.csv` compatível com `nvs_keys`
- chave de signing fora do repo
- build PROD assinado gerado com sucesso
- validacao de que coredump esta desativado em PROD
- validacao de que log default esta em WARN em PROD
- auditoria de logs de credenciais concluida
- procedimento de `encrypted-flash` registrado para o lote/unidade

## O que nao fazer

- nao usar a mesma placa para DEV e para provisionamento irreversivel
- nao commitar `.pem`, credenciais Wi-Fi ou capturas com segredos
- nao usar `idf.py build` simples como se fosse imagem de producao
- nao reativar coredump em PROD sem reavaliar risco de segredo em memoria
- nao tratar `encrypted-flash` como opcional apos o primeiro boot PROD

## Relacao com a trilha H

- `H3`: fechado por este documento e pela revisao do fluxo DEV/PROD existente
- `H4`: proximo passo tecnico - migracao/versionamento robusto de NVS e cache
- `H6`: usa este documento como base para o procedimento de coleta de falhas em campo
