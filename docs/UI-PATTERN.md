# NovaPanel — Padrão de UI (baseline v4)

> Documento-dono do padrão obrigatório de telas, invalidação e view-model.
> Corrige os três defeitos estruturais da UI v3: telas-singleton com
> boilerplate repetido, mapa de invalidação N×M hardcoded no main e
> repintura total da tela a cada evento.

## 1. Conceitos

```text
ScreenRegistry ── registra ──> ScreenSpec (1 por tela)
Shell (rail/topbar/dots)  ──── itera o registry; não conhece telas concretas
ViewModelBuilder (por tela) ── AppState → struct de apresentação pronto
```

## 2. ScreenSpec — contrato de tela

```cpp
struct ScreenSpec {
    ScreenId    id;
    const char* title;                       // título do topbar
    uint32_t    invalidation_mask;           // OR de EventType bits (ver §3)
    lv_obj_t* (*build)(lv_obj_t* parent);    // cria widgets, retorna raiz
    void      (*update)(const AppState& s);  // repinta a partir do estado
    void      (*on_enter)();                 // opcional (nullptr ok)
    void      (*on_leave)();                 // opcional
};
```

Regras:

- Tela nova = 1 arquivo em `ui/src/screens/` + 1 linha de registro. **Zero
  edições em main/ ou no shell.**
- `build/update` rodam SEMPRE sob o lock da UI, chamados pelo shell — a tela
  nunca adquire lock por conta própria.
- Estado interno da tela (handles lv_obj_t) vive em struct de contexto do
  arquivo da tela; proibido `static std::function` global.
- `update()` deve ser idempotente e barato; alvo < 100 ms. Se um evento só
  afeta um widget (ex.: OHLC → gráfico), a tela decide granularidade
  internamente — o shell não micro-gerencia.

## 3. Invalidação por máscara (substitui as flags do main v3)

- Cada `EventType` relevante para UI tem um bit (`ui_event_bit(EventType)`).
- A tela declara em `invalidation_mask` quais eventos a invalidam.
- O shell, ao drenar o `UiDispatcher` (que já coalesceu por tipo — único dono
  de coalescing), acumula a máscara do ciclo e chama `update()` UMA vez por
  tela suja que esteja visível.
- Tela invisível suja não repinta: marca dirty e repinta em `on_enter`.
- Eventos com payload individual (ScreenChanged, NotificationCreated/toast)
  têm tratamento próprio no shell — são os únicos casos especiais permitidos.

Adicionar um evento novo = definir o bit; telas interessadas adicionam o bit
à sua máscara. Nenhum mapa central para editar.

## 4. View-model por tela

- Cada tela tem um builder puro `make_<tela>_vm(const AppState&) -> <Tela>Vm`
  em `ui/src/viewmodels/` — SEM LVGL, host-testável.
- O view-model contém strings prontas (formatação, pt-BR, unidades),
  enums de cor/ícone decididos, e flags de stale/ausência já resolvidas.
- A função `update()` da tela só transfere Vm → widgets. Formatação dentro de
  `update()` é code smell; domínio dentro do Vm builder também (ele formata,
  não calcula negócio).
- Strings de produto em `ui/include/strings_ptbr.hpp` (tabela única).

## 5. Shell

- Dono de: rail de navegação, topbar (relógio, wifi, notificações, engrenagem),
  dots, overlay de notificações, toasts, teclado compartilhado.
- Navegação: shell publica intenção (`navigate(ScreenId)`) → StateStore →
  evento → shell troca a tela ativa e chama `on_leave`/`on_enter`/`update`.
- O shell constrói telas sob demanda (lazy) na primeira navegação, exceto
  Boot/Home — controla pico de RAM de widgets.

## 6. Interação → sistema

- Callback de widget publica intenção via ActionQueue (`enqueue_action`) —
  nunca executa IO, NVS ou lógica no callback de toque.
- Exceção deliberada e documentada: preview contínuo de hardware barato
  (ex.: brilho durante drag) pode chamar `board.set_brightness()` direto;
  persistência só no released (padrão validado no v3).
- Feedback sonoro/visual imediato é permitido; efeito de estado, nunca.

## 7. Checklist de tela nova (review)

1. Arquivo único em `screens/` + Vm builder em `viewmodels/` + registro.
2. `invalidation_mask` mínima (só os eventos que realmente usa).
3. Nenhum include de service/provider/board no arquivo da tela.
4. Vm builder com teste host cobrindo: dado válido, stale, ausente.
5. Placeholder só se documentado no ROADMAP com fase de saída.
6. Medida de tempo de `update()` registrada no PR (log de bancada).
