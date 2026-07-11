# Referências legadas

`firmware_v3/` contém o tree v3 rastreado no repositório quando o baseline v4
foi iniciado. `firmware_v3_field_snapshot/` preserva uma cópia local anterior
que contém a HAL `board/` e o experimento de coexistência validado em bancada.
Ambos são somente leitura e servem para consulta e port seletivo de módulos
que respeitem os contratos do baseline v4.

Não reative esse firmware, não faça builds nele como parte do fluxo normal e
não copie wiring, telas ou dependências sem revisão contra
`docs/ARCHITECTURE.md` e `docs/DECISIONS.md`.
