# NovaPainel - Soak and Performance Validation

Playbook da trilha H8. Define o roteiro minimo para validar estabilidade longa
sem depender da memoria da conversa ou de testes ad hoc.

## Objetivo

Validar que o painel continua util, responsivo e previsivel sob uso prolongado,
com foco em:

- Wi-Fi real
- display/touch ativos
- cache local
- renderizacao recorrente
- reconnect e fetch periodico

## Roteiro 24h

Executar com unidade em ambiente estavel:

1. Boot frio com credenciais ja salvas.
2. Confirmar conexao Wi-Fi, NTP e dados reais.
3. Deixar a unidade 24h ligada alternando entre `Home`, `Settings` e `System`.
4. Durante a janela, provocar ao menos:
   - um reboot manual
   - um desligamento/retorno do roteador ou AP
   - um scan Wi-Fi
   - uma fase sem internet suficiente para cair em cache

Registrar:

- reboot_count
- reset_reason
- se o reconnect voltou sozinho
- se o dado stale foi sinalizado corretamente

## Roteiro 72h

Executar quando uma release estiver perto de campo real:

- repetir o roteiro de 24h
- incluir ao menos duas quedas de conectividade
- incluir permanencia prolongada na tela principal
- observar qualquer reset espontaneo, freeze de touch ou falha de render

## Metas

- zero panic/reset espontaneo
- reconnect automatico apos queda de Wi-Fi dentro de tempo aceitavel
- nenhuma corrupcao de cache/NVS detectada no boot seguinte
- UI continua responsiva apos scan, reconnect e horas de uptime
- sem crescimento evidente de falhas operacionais ao longo do tempo

## Sinais de bloqueio

Bloqueiam release:

- `reset_reason` diferente de `POWERON`/`SW` sem explicacao controlada
- reconnect falhando de forma repetivel com credenciais validas
- onboarding reaparecendo sem motivo apos reboot
- cache invalido reaparecendo em todo boot
- degradacao visual persistente ou travamento de touch

## Relacao com a trilha H

- `H8`: fechado como roteiro de validacao
- resultados concretos devem ser anexados ao processo de release correspondente
