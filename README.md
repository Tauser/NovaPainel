# NovaPanel

Smart display pessoal, local e offline-first para ESP32-P4 + ESP32-C6.
O baseline vigente é o **v4**, uma reconstrução arquitetural em ESP-IDF +
LVGL/C++17.

O estado do projeto está exclusivamente em [docs/STATUS.md](docs/STATUS.md).
No momento, a Fase 0 está em andamento e **não existe firmware ativo** em
`firmware/`; a Fase 1 criará esse tree.

## Estrutura

```text
reference/firmware_v3/  referência v3, somente leitura e port seletivo
firmware/               firmware v4 (criado na Fase 1)
docs/                   documentação canônica v4
shared/                 contratos congelados até existir a segunda ponta
tools/                  tooling e validação portátil
```

Comece por [docs/STATUS.md](docs/STATUS.md), depois consulte
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) e
[docs/ROADMAP.md](docs/ROADMAP.md).
