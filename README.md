# SPACE-SHOOTER-ESP32S3

Mini game **Space Shooter** desenvolvido para **ESP32-S3 N16R8**, display **ILI9341 2.8\" 320x240**, touch **XPT2046** e controle **Dabble BLE GamePad**.

## Versão atual

**v2.0.0**

Esta versão representa uma evolução ampla do projeto, com novo menu, persistência em NVS, estatísticas, novos inimigos, tiros inimigos, combo, power-ups adicionais, melhorias visuais e controle analógico.

## Principais recursos da v2.0.0

- Menu navegável com START GAME, SCORE e RESET SCORE
- High score persistente em NVS
- Estatísticas de partidas e pontuação acumulada
- 4 tipos de inimigos
- Inimigo atirador com projéteis direcionados
- Power-ups de tiro triplo, escudo e vida extra
- Sistema de combo com multiplicadores
- Explosões e partículas aprimoradas
- Rastro visual dos tiros
- HUD redesenhado
- Nave com visual mais detalhado
- Controle digital via D-pad
- Controle analógico proporcional X/Y
- Prioridade do D-pad sobre o analógico para evitar interferência de drift
- Movimento diagonal normalizado
- Proteção breve após dano para evitar perda múltipla de vidas no mesmo instante
- Identificação da versão na tela inicial

## Controles

- **D-Pad** — movimentação digital
- **Analógico X/Y** — movimentação proporcional
- **CROSS / CIRCLE** — atirar / confirmar
- **START** — iniciar / pausar / continuar / reiniciar
- **SELECT** — voltar ao menu em telas compatíveis

Consulte `docs/CONTROLS.md` para detalhes.

## Hardware

- ESP32-S3 N16R8
- TFT 2.8\" ILI9341
- XPT2046
- Dabble BLE GamePad

A configuração testada da biblioteca TFT_eSPI está em:

`config/TFT_eSPI/User_Setup.h`

Consulte `docs/HARDWARE.md` e `docs/INSTALLATION.md` antes de compilar.

## Estrutura

```text
firmware/                     Código-fonte principal
config/TFT_eSPI/              Configuração testada da TFT_eSPI
docs/                         Documentação
licenses/                     Avisos de terceiros
references/original/          Fontes históricas das versões
CHANGELOG.md                  Histórico de versões
VERSION                       Versão atual
SHA256SUMS.txt                Checksums dos arquivos
```

## Histórico

- **v1.0.0** — primeira versão pública
- **v1.1.0** — controle analógico e melhorias de input
- **v2.0.0** — grande atualização de gameplay, interface, persistência e controles

## Compilação

O projeto utiliza Arduino/ESP32 e as bibliotecas:

- TFT_eSPI
- DabbleESP32
- Preferences (ESP32)

Antes de compilar, instale a configuração fornecida em `config/TFT_eSPI/User_Setup.h` conforme explicado em `docs/INSTALLATION.md`.

## Status

**v2.0.0 testada no hardware e aprovada para publicação.**
