# SPACE SHOOTER v5.0.0 — Naves, Loja e Bosses

## Novidades
- Campanha de 50 níveis, dez bosses com 3 fases, 9 inimigos e Endless.
- Cinco naves e seis armas; moedas, loja, seis upgrades persistentes e 15 conquistas.
- Dash, cenários e efeitos visuais expandidos.

## Correções
- Inimigo blindado agora sofre dano de tiros vindos de baixo.
- Recompensas do boss Void são creditadas na tela final.
- Correção **local** da DabbleESP32 evita o reset `heap_caps_free` observado: siga [DABBLE_BLE_RESET.md](DABBLE_BLE_RESET.md) antes de compilar.

## Validação
- 252.788 verificações simuladas de lógica/gameplay (incluindo 10 bosses e 50 níveis).
- O autor aplicou o patch BLE na placa e confirmou que o erro de reset deixou de ocorrer.
- O patch da biblioteca não está dentro do `.ino` e pode ser removido por uma atualização da DabbleESP32.
