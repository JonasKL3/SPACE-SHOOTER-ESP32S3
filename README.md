# SPACE-SHOOTER-ESP32S3

Jogo **SPACE SHOOTER** para **ESP32-S3 N16R8**, tela TFT **ILI9341 2,8″ 320×240** e controle Bluetooth Low Energy pelo aplicativo **Dabble GamePad**. O controlador XPT2046 está fisicamente disponível, mas o toque não é utilizado no jogo.

## Versão atual — v4.0.0

Esta é a versão de lançamento que substitui a v3.0.0 na branch `main` após os testes realizados pelo autor no hardware. O firmware é **idêntico** ao candidato `v4.0.0-ANTI-TRAVAMENTO-TESTE` aprovado na ESP32-S3.

### Novidades

- **Campanha** de 20 níveis (600 pontos por avanço) e modo **Endless**.
- **Cinco tipos de boss**: Sentinel, Hydra, Prime, Devourer e Omega. A campanha enfrenta quatro bosses (níveis 5, 10, 15 e 20); Omega entra no ciclo do Endless no nível 25.
- **Seis armas**: Padrão, Duplo, Triplo, Spread, Laser perfurante e Míssil teleguiado.
- **Cinco cenários**: Espaço, Nebulosa, Asteroides, Tempestade e Caos.
- Power-ups, escudo, vida extra, combos, barra de vida dos bosses e estatísticas persistentes.
- Quatro skins; progresso e recordes salvos na NVS, preservando os dados da versão anterior.

### Estabilidade e desempenho

- Desenho limitado a uma tentativa de quadro a cada **42 ms** (~24 FPS visuais), enquanto a leitura do Dabble continua separada.
- Ajustes nas partículas, desativação de logs de eventos em tempo crítico, intervalo de transição entre bosses e escrita NVS adiada para o resultado da partida.
- **Diagnóstico na tela de pausa**: `DRAW`, `SPI`, `LOGIC` e `SLOW`. Consulte `docs/PERFORMANCE_V4.md`.
- Joystick analógico proporcional, D-pad prioritário, movimento máximo de 6.0 e diagonais normalizadas.
- Proteção contra dano múltiplo em sequência; prevenção de dano repetido pelo mesmo laser; correções de menus, recorde e estado de vitória.
- Identificação `v4.0.0` na tela inicial.

> **Observação sobre as skins:** o menu apresenta os rótulos visuais `VERDE`, `DOURADA`, `CIANO`, `ROXA`, conforme ajuste solicitado e testado na versão anterior. Os índices, cores, requisitos e dados salvos mantêm os mapeamentos internos originais. Consulte `docs/CONTROLS.md`.

## Como instalar

1. Confira `docs/HARDWARE.md` e a pinagem do display.
2. Configure TFT_eSPI com `config/TFT_eSPI/User_Setup.h` e instale a biblioteca `DabbleESP32`.
3. Abra `firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino` na Arduino IDE com o core ESP32.
4. Compile e grave na ESP32-S3 N16R8. Abra o módulo GamePad do Dabble e conecte-se ao dispositivo BLE `ESP32-S3-GAMEPAD`.

Instruções detalhadas: `docs/INSTALLATION.md`.

## Controles

- D-pad ou joystick analógico: mover e navegar.
- CROSS/CIRCLE: atirar durante a partida; CROSS confirma menus.
- START: pausar/continuar e sair da tela final.
- SELECT: voltar/cancelar onde permitido.
- RESET DATA: requer **confirmação explícita**; apaga recordes, estatísticas e progresso.

## Documentação e organização

```text
firmware/SPACE_SHOOTER_ESP32S3/    Firmware Arduino (v4.0.0)
config/TFT_eSPI/                    Setup do ILI9341
 docs/                               Hardware, controles, instalação, performance e testes
references/original/               Fontes recebidas das versões v1 a v4
 tests/                               Testes de lógica em C++ e simuladores locais
licenses/                            Avisos de bibliotecas de terceiros
README.md / CHANGELOG.md / VERSION Documentação e identificação
SHA256SUMS.txt / FILE_LIST.txt      Integridade e inventário
```

## Histórico de releases

- `v1.0.0` — primeira versão.
- `v1.1.0` — analógico e correções de controle.
- `v2.0.0` — menus, estatísticas, inimigos e power-ups.
- `v3.0.0` — campanha curta, bosses e skins.
- **`v4.0.0`** — campanha de 20 níveis, bosses especializados, novas armas/cenários e melhorias antitravamento.

A `main` deve representar a versão estável mais recente; as versões antigas ficam preservadas pelas respectivas **tags e GitHub Releases**. Ao publicar a atualização, envie o **conteúdo extraído** deste pacote para a raiz do repositório, faça commit e somente então crie a tag `v4.0.0` a partir do commit atualizado.
