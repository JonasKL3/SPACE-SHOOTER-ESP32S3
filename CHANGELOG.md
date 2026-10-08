# Changelog

Todas as mudanças relevantes do projeto são registradas neste arquivo.

## [2.0.0] - 2026-10-08

### Added

- Menu navegável com START GAME, SCORE e RESET SCORE.
- Persistência de high score, partidas e pontuação total em NVS.
- Tela de estatísticas.
- Quarto tipo de inimigo: atirador.
- Projéteis inimigos direcionados ao jogador.
- Power-up de vida extra.
- Sistema de combo e multiplicadores de pontuação.
- Nave e inimigos com visuais aprimorados.
- Explosões em múltiplas camadas e partículas ampliadas.
- Rastro de balas e HUD redesenhado.
- Controle analógico proporcional X/Y.
- Navegação analógica no menu.
- Exibição de `v2.0.0` no canto inferior direito da tela inicial.

### Changed

- Velocidade máxima da nave ajustada para 6.0.
- D-pad agora tem prioridade sobre o analógico.
- Movimento diagonal normalizado para evitar ganho de velocidade.
- Leitura do Dabble processada continuamente para reduzir latência.
- Fluxo de input reorganizado para evitar transições duplicadas entre telas.

### Fixed

- Corrigido drift analógico afetando movimento digital.
- Corrigida saída imediata da tela SCORE ao manter o botão de confirmação pressionado.
- Corrigido processamento de dano após Game Over no mesmo frame.
- Adicionada proteção breve após dano para evitar perda múltipla de vidas instantaneamente.
- Corrigida detecção de novo recorde em caso de empate com o high score existente.
- Corrigidas condições de borda em START, SELECT e confirmação de menu.

## [1.1.0] - 2026-10-04

### Added

- Controle analógico X/Y através do Dabble GamePad.
- Movimento proporcional ao deslocamento do joystick.
- Zona morta central para reduzir drift.

### Changed

- D-pad ganhou prioridade sobre o controle analógico.
- Input reorganizado para preservar o comportamento digital da v1.0.0.

### Fixed

- Corrigida interferência do joystick analógico no movimento digital.
- Corrigido caractere acidental no final do loop do código recebido para revisão.

## [1.0.0] - 2026-10-04

### Added

- Primeira versão pública do SPACE SHOOTER para ESP32-S3.
- ILI9341 320x240.
- Dabble BLE GamePad.
- Movimento digital.
- Sistema de tiro.
- Inimigos, partículas e power-ups.
- Score, níveis, vidas e Game Over.
