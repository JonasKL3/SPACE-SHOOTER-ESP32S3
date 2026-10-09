# Controles — SPACE SHOOTER v3.0.0

## GamePad Dabble

| Ação | Comando |
|---|---|
| Movimentar a nave | D-pad ou joystick analógico X/Y |
| Atirar | CROSS ou CIRCLE |
| Confirmar uma opção | CROSS ou START (na maioria dos menus) |
| Pausar/continuar | START durante a partida |
| Voltar | SELECT nas telas que permitem retorno |
| Encerrar a tela Game Over | START ou SELECT |

O D-pad tem prioridade sobre o joystick analógico; a zona morta elimina leitura nos valores `-1, 0, +1`. A diagonal é normalizada e a velocidade máxima é `6.0` por frame.

## Menus

- **MENU**: START GAME, SKIN, SCORE, RESET DATA. Atenção: RESET DATA remove high score, estatísticas e skins desbloqueadas.
- **MODE SELECT**: CAMPANHA ou ENDLESS.
- **SKIN SELECT**: navegar entre quatro slots; confirmar com CROSS/START somente se desbloqueado.
- **SCORES**: CROSS/SELECT/START para voltar ao menu.

Para navegar, use UP/DOWN do D-pad ou o eixo Y analógico. Não há controle por touchscreen nesta versão.
