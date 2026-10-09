# Changelog

Todas as mudanças relevantes do projeto são registradas neste arquivo.

## [3.0.0] - 2026-10-08

### Added

- Modos **Campanha** (até o nível 10) e **Endless**.
- Mini-boss nos níveis múltiplos de cinco; chefão final da campanha.
- Boss com barra de vida, duas fases e três padrões de ataque.
- Quatro slots de skins com seleção e progresso salvos na NVS.
- Desbloqueios por pontuação acumulada e por conclusão da campanha.
- Exibição de `v3.0.0` no menu principal.

### Changed

- Restaurado o controle analógico proporcional aprovado em versões anteriores, com prioridade para o D-pad.
- Movimento máximo configurado para 6.0; diagonal normalizada e eixo Y invertido para coordenadas de tela.
- Dabble processado continuamente em vez de usar polling de 30 ms.
- Ajustados os nomes visuais de CIANO/DOURADA no menu, preservando índices internos e dados NVS.

### Fixed

- Corrigida a demora artificial para detectar analógico e a zona morta central.
- Corrigidas transições inesperadas entre telas por botões mantidos pressionados.
- Prevenida perda consecutiva de vidas por projéteis, inimigos ou contato com boss (700 ms de proteção).
- Corrigida a contabilização duplicada do Game Over.
- Corrigida a identificação incorreta de novo recorde quando apenas empatado.
- Corrigido o encerramento e salvamento da campanha após derrotar o último chefe.

### Verification

- Passou em verificação de sintaxe C++ com substitutos das dependências.
- Validação final da correção dos nomes no dispositivo físico pendente de confirmação.

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
