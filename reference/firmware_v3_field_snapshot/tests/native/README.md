# Testes nativos (host)

Os módulos de `components/{core,models,services,utils}` são escritos para serem
**independentes de plataforma** sempre que possível, de modo que a lógica possa
ser testada no host (PC), sem hardware nem ESP-IDF.

Estratégia pretendida:

- compilar `EventBus`, `StateStore`, `RequestOrchestrator`, `Result<T>` e os
  serviços (com providers/board mockados) com um `g++`/CMake nativo;
- usar um framework leve (ex.: doctest/Catch2) ou asserts simples;
- isolar dependências de ESP-IDF (`esp_log`, FreeRTOS) atrás de pequenas
  abstrações ou shims de teste.

Cobertura atual:

- `core_tests.cpp` cobre `RequestOrchestrator`, `StateStore` (merge parcial de
  mercado/forex), `EventBus` (unsubscribe durante publish), `UiDispatcher`
  (filtro/coalescencia de eventos) e `NotificationService`.

Como rodar:

```bash
bash tools/scripts/host_check.sh --app --tests
```

Isso compila o core host-checkable e executa o binario nativo de testes usando
os mesmos shims de host do `host_check`.
