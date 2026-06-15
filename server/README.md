# NovaPainel - Server (opcional, futuro)

> O server é **opcional e futuro**. O firmware **não depende dele** (ADR-0002).

Esta pasta existe apenas para reservar o lugar do componente no monorepo. Não há
implementação real ainda (sem FastAPI, sem dependências). Quando for o momento,
ele poderá servir para:

- integrações pesadas;
- LLM local futura;
- web search futura;
- sincronização de dados;
- dashboard remoto;
- ponte com o NoiseBot.

Regra inviolável:

```text
O firmware não pode depender do server para funcionar.
```

## Rodar o placeholder

```bash
cd server
python -m novapainel_server.main
# ou, após instalar em modo editável:
# pip install -e .  &&  novapainel-server
```
