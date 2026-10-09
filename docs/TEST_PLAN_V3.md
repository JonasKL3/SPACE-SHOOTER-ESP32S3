# Plano de teste final — SPACE SHOOTER v3.0.0

Executar **no ESP32-S3 físico**, com o `User_Setup.h` e Dabble utilizados pelo autor. Marcar cada caso como aprovado ou reprovado antes de publicar.

- [ ] A Arduino IDE compila o `.ino` e a gravação termina sem erros.
- [ ] Inicialização sem travamento; imagem íntegra e `v3.0.0` visível no canto inferior direito.
- [ ] D-pad: UP, DOWN, LEFT, RIGHT e diagonais. Direção LEFT com a mesma velocidade que RIGHT.
- [ ] Analógico: quatro direções, velocidade proporcional, eixo Y correto e imobilidade ao soltar.
- [ ] D-pad mantém prioridade com analógico levemente deslocado; diagonal sem aumento de velocidade.
- [ ] MENU: navegar e entrar em START GAME → MODE SELECT sem atravessar telas involuntariamente.
- [ ] MODE SELECT: iniciar CAMPANHA e ENDLESS; START durante o jogo pausa/continua.
- [ ] SKIN SELECT: navegação, nomes visuais VERDE / DOURADA / CIANO / ROXA, cores exibidas e seleção de item desbloqueado.
- [ ] SKIN SELECT: requisitos de desbloqueio correspondem ao slot correto e dados permanecem após reiniciar.
- [ ] SCORE: entrar, permanecer na tela mesmo segurando CROSS, e sair com CROSS/SELECT/START.
- [ ] Tiro simples e triplo, escudo e vida extra; mudança de fase, inimigos e combo.
- [ ] Tiros inimigos: dano único, proteção temporária após dano, escudo bloqueando dano.
- [ ] Boss nível 5: barra de vida, movimentos, três padrões, mudança de fase, derrota.
- [ ] CAMPANHA: boss nível 10, tela VITORIA, desbloqueio de skin e conclusão salva em NVS.
- [ ] ENDLESS: continua após nível 10 e gera outros bosses a cada cinco níveis.
- [ ] Game Over: partidas incrementam uma única vez, pontuação e recorde persistem após reiniciar.
- [ ] RESET DATA apaga os dados esperados e volta para skin padrão sem erro.

A simples aprovação do ZIP e da sintaxe C++ não comprova funcionamento no hardware.
