# Roteiro de regressão — v4.0.0

## Antes de publicar (executado pelo autor na ESP32-S3)

- [ ] Tela inicial mostra `v4.0.0`.
- [ ] Analógico e D-pad: direção correta, diagonal sem ganho de velocidade, sem drift.
- [ ] Menus SCORE/SKIN/CAMPANHA/ENDLESS sem confirmação dupla.
- [ ] Rótulos visuais DOURADA e CIANO conforme a versão anterior.
- [ ] Coletar armas e power-ups sem pausas perceptíveis.
- [ ] Boss surge e é derrotado sem congelamentos perceptíveis.
- [ ] Pausar e verificar `DRAW`, `SPI`, `LOGIC`, `SLOW`.
- [ ] Testar seis armas, especialmente laser perfurante e míssil guiado.
- [ ] Testar os bosses dos níveis 5, 10, 15 e 20 na CAMPANHA.
- [ ] Testar o boss Omega no ciclo de Endless a partir do nível 25.
- [ ] Conferir vitória apenas após derrotar o boss final do nível 20.
- [ ] Conferir NVS, skins e recorde após reiniciar.
- [ ] Confirmar que RESET DATA pede confirmação (cancelar com SELECT caso não queira apagar).

## Testes locais reproduzíveis (Linux)

```bash
g++ -std=c++17 -O1 -Wall -Wextra -Itests/stubs tests/test_v4_logic.cpp -o /tmp/space_shooter_test_v4
/tmp/space_shooter_test_v4
```

Os testes locais usam stubs para APIs Arduino, Dabble, Preferences e TFT_eSPI; não substituem o hardware.
