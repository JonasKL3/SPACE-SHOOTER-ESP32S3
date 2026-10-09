# Changelog

## [4.0.0] - 2026-10-08

### Added

- Campanha de **20 níveis**, com quatro chefes (5/10/15/20), além do modo Endless.
- Cinco modelos de chefe: Sentinel, Hydra, Prime, Devourer e Omega (Omega no Endless a partir do nível 25).
- Seis tipos de arma: Padrão, Duplo, Triplo, Spread, Laser e Míssil.
- Mísseis guiados e laser perfurante com registro de alvos já atingidos.
- Cinco cenários dinâmicos, power-ups específicos e visuais distintos.
- Indicadores `DRAW`, `SPI`, `LOGIC`, `SLOW` na tela de pausa para análise de desempenho.
- Confirmação de exclusão antes de executar `RESET DATA`.
- Testes hospedados reproduzíveis em `tests/`.

### Changed

- Renderização limitada a aproximadamente 24 quadros visuais por segundo, com relógio de lógica separado.
- Processamento do Dabble preservado independentemente do desenho do display.
- Mensagens de eventos durante gameplay desabilitadas por padrão.
- Geração de partículas otimizada dentro do limite de 80 elementos.
- Escrita NVS de fim de partida diferida para depois da apresentação do resultado ou antes de sair dessa tela.
- Intervalo de aproximadamente 1.100 ms entre chefes consecutivos, evitando sobreposição após saltos de nível.
- Namespace de NVS `shooter` preservado; vitória da campanha de 20 níveis mantida em campo separado de conclusão da campanha de 10 níveis.
- Identificação visual `v4.0.0` no menu inicial.

### Fixed

- Correções de const-correctness na configuração dos cenários.
- Joystick sem atraso artificial, eixo Y adequado, zona morta ±1, D-pad prioritário e velocidade 6.0.
- Normalização das diagonais e proteção de 700 ms após dano.
- Contabilização única de Game Over; ausência de falso novo recorde em empates.
- Correções de menu para impedir confirmações duplicadas entre telas.
- Boss final da campanha concluído apenas após a derrota do chefe correto.
- Desbloqueios e estatísticas anteriores validados durante leitura da NVS.
- Nomes visuais de skins conforme correção aprovada na v3, sem alterar índices salvos.
- Correções de disparos, limites dos mísseis e colisões de laser para impedir dano repetido no mesmo alvo.

### Verification

- Testes de sintaxe e comportamento executados previamente com substitutos locais das APIs C++/Arduino.
- Melhorias antitravamento **testadas e aprovadas pelo autor na ESP32-S3 física** antes da preparação deste pacote.
- Nenhuma alteração de lógica realizada ao promover o firmware de teste aprovado para esta versão de lançamento.

---

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
