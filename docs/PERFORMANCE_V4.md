# Performance e correção de congelamentos — v4.0.0

## Sintoma corrigido nos testes físicos

Pausas temporárias percebidas ao coletar itens, durante surgimento do boss e após sua derrota. O usuário testou o candidato antitravamento na ESP32-S3 e confirmou que o comportamento ficou correto.

## Ajustes feitos

1. **Renderização**: intervalo mínimo de 42 ms entre solicitações usuais de desenho (~24 FPS alvo), reduzindo a sobrecarga de atualização do display.
2. **Dabble**: entrada BLE processada continuamente, independente da periodicidade visual.
3. **Partículas**: geração limitada por varredura de slots livres, com máximo de 80 partículas simultâneas.
4. **Logs**: mensagens de eventos por `Serial` desativadas por padrão (`GAME_EVENT_LOGS=false`).
5. **NVS**: persistência do resultado adiada para a apresentação da tela final ou para o momento de retornar ao menu.
6. **Bosses**: pequeno intervalo de 1.100 ms impede spawns consecutivos instantâneos após saltos de pontuação.

### Por que 42 ms?

Um quadro RGB565 de 320×240 contém 153.600 bytes. Em um barramento SPI a 40 MHz, o tempo de transmissão teórico mínimo é aproximadamente 30,72 ms, antes da renderização e das despesas da biblioteca. Esse cálculo serve como referência; o tempo real depende do display, da biblioteca e da configuração.

## Como interpretar os indicadores da tela de PAUSA

- `DRAW`: maior tempo observado desenhando e enviando um quadro (ms).
- `SPI`: maior tempo de envio do sprite para o display (ms).
- `LOGIC`: maior tempo observado na atualização da lógica (ms).
- `SLOW`: número de desenhos que excederam 42 ms.

> A aprovação do teste na placa confirma a melhora na configuração utilizada, mas não demonstra isoladamente qual dos fatores era a causa única das pausas anteriores.

**Observação sobre gravação do resultado:** como a escrita da NVS é diferida, desligar a ESP32-S3 antes da gravação pode fazer o resultado da última partida não ser salvo. Ao final, aguarde a tela Game Over ou retorne ao menu normalmente.
