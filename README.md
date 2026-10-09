# SPACE-SHOOTER-ESP32S3

Mini game **Space Shooter** para **ESP32-S3 N16R8**, display **ILI9341 2.8" 320×240**, módulo touch XPT2046 (sem uso no jogo) e **Dabble BLE GamePad**.

## Versão do pacote

**v3.0.0** — campanha, modo endless, bosses e skins.

> **Validação:** o código passou em verificação de sintaxe C++ com simuladores das APIs. A versão com a última correção visual dos nomes das skins ainda requer confirmação de teste no dispositivo real antes de ser divulgada como estável.

## Principais recursos

- Dois modos de jogo: **Campanha** (até o nível 10 com chefe final) e **Endless**.
- Chefes nos níveis 5, 10, 15... do modo Endless; campanha encerra após o boss final.
- Três padrões de ataque do boss: leque, radial e rajada vertical (identificada no código como laser).
- Skins de nave desbloqueáveis; seleção, progresso e desbloqueios guardados na NVS.
- High score, partidas e pontuação acumulada persistentes na NVS.
- D-pad e joystick analógico X/Y com prioridade ao D-pad, zona morta e diagonais normalizadas.
- Movimento máximo da nave configurado para **6.0 unidades por frame**.
- Quatro tipos de inimigos, disparos inimigos, combos, tiro triplo, escudo e vida extra.
- Proteção de 700 ms após dano, prevenção de Game Over duplicado e transições de menu revisadas.
- Identificação `v3.0.0` no canto inferior direito do menu inicial.

## Skins

Menu visual: **VERDE**, **DOURADA**, **CIANO** e **ROXA** (nomes visuais ajustados conforme os testes no display). Os índices internos e os requisitos de desbloqueio do firmware foram preservados da base recebida. Em especial, o slot 1 depende de 5.000 pontos acumulados, o slot 2 depende de vencer a campanha e o slot 3 depende de 20.000 pontos acumulados. **Não apagar os dados da NVS ao atualizar o firmware se quiser preservar progresso.**

## Controles

- D-Pad: movimentação digital e navegação em menus.
- Analógico X/Y: movimentação proporcional e navegação vertical em menus.
- CROSS / CIRCLE: atirar durante a partida.
- CROSS / START: confirmar menus compatíveis.
- START: pausar/continuar, sair da tela de Game Over.
- SELECT: voltar ao menu nas telas compatíveis.

Confira os detalhes em `docs/CONTROLS.md`.

## Requisitos de hardware

- Placa ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM).
- TFT ILI9341 SPI de 2.8", ligado com os GPIOs indicados em `docs/HARDWARE.md`.
- Dabble BLE GamePad no smartphone.

O projeto inclui a configuração da TFT_eSPI em `config/TFT_eSPI/User_Setup.h`. **É necessário selecionar esse setup na biblioteca antes de compilar.**

## Compilação

Abra `firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino` na Arduino IDE com o core ESP32 e as bibliotecas TFT_eSPI e DabbleESP32 instaladas. `Preferences` acompanha o core ESP32.

Consulte `docs/INSTALLATION.md` e execute o roteiro de `docs/TEST_PLAN_V3.md` antes de publicar uma release.

## Estrutura

```text
firmware/              Código principal Arduino
config/TFT_eSPI/       Configuração ILI9341 / XPT2046
docs/                  Instalação, hardware, controles e testes
references/original/   Fontes recebidas por versão
licenses/              Avisos sobre componentes de terceiros
CHANGELOG.md           Histórico da evolução
VERSION                Versão do projeto
FILE_LIST.txt          Inventário do pacote
SHA256SUMS.txt         Integridade dos arquivos
```

## Histórico

- `v1.0.0` — jogo inicial.
- `v1.1.0` — controle analógico e correções de input.
- `v2.0.0` — menus, NVS, novos inimigos, partículas, combo e power-ups.
- `v3.0.0` — campanha, endless, chefes, skins e correções de estabilidade.

Este repositório usa `main` para a versão estável atual e GitHub Releases (com tags automáticas) para os marcos anteriores.
