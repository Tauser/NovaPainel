# NovaPanel — Operação, Release e Segurança (baseline v4)

> Unifica os antigos SECURITY-OPERATIONS / FIELD-OPERATIONS /
> RELEASE-ROLLBACK do v3 em um documento coerente e sem contradições
> (a contradição coredump-vs-PROD do v3 foi resolvida no ADR-0012).
> Estado de implementação: `STATUS.md`. Fase alvo: 6 e 7 do ROADMAP.

## 1. Observabilidade

### Em memória / tela

- Tela de Sistema mostra: reset reason, reboot count, uptime, heap interno e
  PSRAM (atual + watermark), estado de cada circuit breaker, RSSI, versão de
  firmware e partição ativa.
- `ResourceWarning` tem handler real: log WARN + contador persistido +
  desligamento do modo focado (candles) até recuperar.

### Persistido (NVS, contadores pequenos)

reboots totais · reboots por breadcrumb de falha de boot · falhas por domínio
de rede · aberturas de breaker · overflows de ActionQueue · ResourceWarnings.
Dedup de escrita; commit agrupado; nunca em callback de toque.

### Coredump (ADR-0012)

- Partição `coredump` dedicada; **habilitado inclusive em PROD**.
- Provisionamento apaga a partição; triagem via `espcoredump.py` documentada
  em runbook (§5).
- Gotcha herdado: crash em init de áudio já corrompeu coredump no v3 —
  validar coredump são no roteiro de fault injection.

## 2. Perfis de build

| | DEV (`sdkconfig.defaults`) | PROD (`+ sdkconfig.prod`) |
|---|---|---|
| Log | INFO | WARN |
| Coredump | flash | flash (mantido) |
| Secure Boot v2 | off | on |
| Flash Encryption | off | **RELEASE (irreversível)** |
| NVS Encryption | off | on |
| JTAG/USB-Serial | on | desabilitado |
| Anti-rollback | off | on, só junto do fluxo OTA (ADR-0010) |

Chaves (assinatura Secure Boot / OTA) vivem FORA do repositório, com backup
offline documentado. Perder a chave de assinatura = perder a frota.

## 3. OTA (Fase 7 — pré-requisito de qualquer unidade PROD)

- Partições `ota_0/ota_1 + otadata` desde a Fase 1 (ADR-0010).
- Canal MVP: upload local na LAN (sem nuvem), imagem assinada; verificação de
  assinatura antes de gravar.
- Escrita de OTA em modo de UI reduzida (tela de update) — pior caso do
  barramento flash/PSRAM (RESOURCE-BUDGET §4).
- **Rollback automático**: imagem nova marca-se válida só após health-check
  pós-boot (display ok + services init + heap acima do limiar + relógio
  plausível dentro de N s). Sem confirmação → bootloader volta para a
  partição anterior.
- Firmware do C6: slave-OTA via SDIO (HARDWARE.md), coordenada com a versão
  do app (matriz de compatibilidade P4↔C6 registrada por release).

## 4. Release

1. CI verde (TESTING §5) + build IDF local + checklist de placa da fase.
2. Versionar (semver + `PROJECT_VER`), changelog curto, tag git.
3. Gerar imagem DEV → validar em bancada → gerar imagem PROD assinada.
4. Ensaiar OTA na unidade de bancada (aplicar + rollback forçado).
5. Só então distribuir para unidade(s) de campo.

Enquanto OTA não existir (Fases < 7): release é cabeada, e **nenhuma unidade
recebe Flash Encryption RELEASE** — sem exceções.

## 5. Runbooks de campo

- **Painel travado/reiniciando**: ler reset reason + breadcrumb na tela de
  sistema (ou via serial DEV); coletar coredump; abrir issue com os três.
- **Sem rede**: tela de sistema mostra estado do link C6, do Wi-Fi e dos
  breakers separadamente — distinguir "AP fora", "link SDIO caiu" (raro,
  reboot resolve e conta breadcrumb) e "APIs fora" (breaker aberto, resolve
  sozinho).
- **Dados velhos na tela**: por design (stale sinalizado); verificar breaker
  do domínio na tela de sistema antes de assumir bug.
- **Boot loop**: breadcrumb NVS indica a etapa que falhou; cadência de retry
  degrada automaticamente (ARCHITECTURE §9); recuperação via reflash cabeado
  DEV.
- **Provisionamento PROD**: procedimento passo-a-passo mantido junto ao
  script em `tools/` (Fase 7): efuses → chaves → imagem assinada → wipe de
  coredump/NVS → smoke test → registro de unidade.

## 6. Segurança — modelo de ameaça resumido

Ativos: credencial Wi-Fi (NVS cifrada em PROD), integridade do firmware
(Secure Boot + assinatura OTA), disponibilidade do painel.
Superfície: rede local (cliente HTTPS apenas — sem servidor escutando no
MVP), acesso físico (mitigado por Flash Encryption/efuses em PROD; DEV é
aberto por escolha).
Não-objetivos do MVP: secure element externo, attestation remota, multiusuário.

Regras permanentes: nunca logar segredo · TLS com bundle de CA em toda
conexão · NTP antes de validar certificado (dependência real: relógio errado
quebra TLS) · nenhum segredo em `shared/`, docs ou fixtures.
