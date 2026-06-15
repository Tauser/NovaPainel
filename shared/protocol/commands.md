# Protocolo - Comandos

Comandos seguem `shared/schemas/command.schema.json`. No MVP, comandos existem
apenas como contrato; o controle real de dispositivos (Sonoff) é **roadmap
futuro** (ADR-0005).

## Tipos

- `system` — ações do próprio painel (ex.: `setScreen`, `maintenance`).
- `ui` — navegação/estado de UI.
- `scene` — acionar uma cena (futuro).
- `device` — ligar/desligar/toggle de um dispositivo/canal (futuro).

## Exemplo

```json
{
  "id": "8f2b1c10-...",
  "type": "device",
  "target": "sonoff_sala",
  "action": "toggle",
  "params": { "channel": 1 },
  "timestamp": 1781820060000
}
```

## Ações previstas (contrato `DeviceControlService` futuro)

```text
turnOn(deviceId, channel)
turnOff(deviceId, channel)
toggle(deviceId, channel)
getState(deviceId)
```

A UI não sabe se por baixo é Sonoff, MQTT ou outro adapter.
