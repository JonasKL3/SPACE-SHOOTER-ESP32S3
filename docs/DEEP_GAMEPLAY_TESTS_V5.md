# Testes de gameplay aprofundados — SPACE SHOOTER v5.0.0

Metodologia: simulação C++17 do código do jogo com mocks de Arduino, TFT_eSPI, DabbleESP32 e NVS; sanitizadores ASan/UBSan. **Não** equivale a teste físico do display SPI, BLE ou alimentação.

| Caso | Resultado |
| --- | --- |
| Campanha | 50 níveis e dez bosses; progressão via pontuação acelerada |
| Bosses | 10 × 3 fases exercitadas |
| Endless | 25 min simulados em duas rodadas |
| Inimigos | 9 tipos exercitados |
| Achievements | 15 conquistas verificadas |
| Regressão: inimigo blindado | danos por disparos normais corrigidos |
| Regressão: Void | recompensas em moedas creditadas na vitória |

**Total reportado:** 252.788 verificações automatizadas; o bot ajustou a pontuação e a mira para percorrer a campanha. O reset físico foi localizado com ELF no parser da biblioteca DabbleESP32; o autor confirmou o desaparecimento da falha após aplicar o patch local. Consulte [DABBLE_BLE_RESET.md](DABBLE_BLE_RESET.md).
