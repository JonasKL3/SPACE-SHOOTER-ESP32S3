# Changelog

Todas as mudanças relevantes do projeto serão documentadas neste arquivo.

O projeto seguirá versionamento semântico na forma `MAJOR.MINOR.PATCH`.

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
