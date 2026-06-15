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

> Ainda não implementado — apenas a pasta e a intenção. A primeira leva de
> testes deve cobrir o `RequestOrchestrator` (intervalo + rate limit) e o
> fluxo `Service -> StateStore -> EventBus`.
