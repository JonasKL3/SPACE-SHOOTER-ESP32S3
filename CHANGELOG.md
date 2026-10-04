# Changelog

Todas as mudanças relevantes do projeto serão documentadas neste arquivo.

O projeto seguirá versionamento semântico na forma `MAJOR.MINOR.PATCH`.

## [1.1.0] - 2026-10-04

### Adicionado

- Suporte ao joystick analógico X/Y do Dabble GamePad.
- Movimento proporcional conforme a intensidade/direção do joystick.
- Camada de entrada unificada para controles digitais e analógicos.

### Corrigido

- Prioridade do D-pad sobre o joystick analógico para impedir interferência de drift durante o movimento digital.
- Zona morta central do joystick para evitar movimento involuntário próximo ao centro.
- Código-fonte revisado para remover caracteres de formatação inválidos e um caractere excedente no final do `loop()`.

### Mantido

- CROSS / CIRCLE como botões de tiro.
- START e SELECT com o mesmo comportamento da v1.0.0.
- Mecânicas, progressão, inimigos, power-ups, HUD e renderização da v1.0.0.

### Validação

- Versão testada e aprovada em hardware pelo autor antes da publicação.

## [1.0.0] - 2026-10-04

### Adicionado

- Primeira versão pública versionada do Space Shooter para ESP32-S3.
- Suporte ao display ILI9341 320x240.
- Controle Bluetooth com Dabble GamePad.
- Menu inicial.
- Estados `MENU`, `PLAYING`, `PAUSED` e `GAME_OVER`.
- Movimento em quatro direções.
- Sistema de tiro.
- Três tipos de inimigos.
- Sistema de colisões.
- Pontuação e high score em memória durante a sessão.
- Progressão de nível e dificuldade.
- Power-ups de tiro triplo e escudo.
- Campo de estrelas.
- Efeitos de partículas.
- Buffer gráfico com `TFT_eSprite`.

### Observação

Esta release é a baseline oficial. Correções posteriores devem receber novas versões, começando em `v1.0.1`.
