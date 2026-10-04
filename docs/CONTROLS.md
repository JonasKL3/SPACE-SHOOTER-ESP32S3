# Controles — v1.1.0

O jogo utiliza o módulo **GamePad** do aplicativo Dabble e agora aceita **D-pad digital** ou **joystick analógico** para movimentação.

| Controle | Ação |
|---|---|
| D-pad UP | Mover nave para cima |
| D-pad DOWN | Mover nave para baixo |
| D-pad LEFT | Mover nave para a esquerda |
| D-pad RIGHT | Mover nave para a direita |
| Joystick analógico X/Y | Mover nave proporcionalmente |
| CROSS | Atirar |
| CIRCLE | Atirar |
| START no menu | Iniciar partida |
| START jogando | Pausar |
| START pausado | Continuar |
| START no Game Over | Reiniciar |
| SELECT no Game Over | Voltar ao menu |

## Prioridade de entrada

Quando qualquer direção do D-pad estiver pressionada, o jogo usa somente o controle digital e ignora o joystick analógico naquele frame. Isso evita interferência de drift do analógico durante o uso do D-pad.

Quando o D-pad não estiver ativo, o joystick analógico controla a nave proporcionalmente, com zona morta central.

## Bluetooth

Nome configurado:

```text
ESP32-S3-GAMEPAD
```
