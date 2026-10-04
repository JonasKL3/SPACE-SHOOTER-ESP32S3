# SPACE SHOOTER — ESP32-S3

Mini game arcade para **ESP32-S3** com display **ILI9341 2.8\" (320x240)** e controle Bluetooth pelo **Dabble GamePad**.

> Versão atual: **v1.1.0**. Baseline inicial: **v1.0.0**.

## Recursos da v1.1.0

- Nave controlada pelo **D-pad** ou pelo **joystick analógico** do Dabble.
- Movimento analógico proporcional com zona morta central.
- Prioridade automática do D-pad para evitar interferência de drift analógico.
- Tiro com **CROSS** ou **CIRCLE**.
- **START** inicia, pausa, continua ou reinicia a partida.
- **SELECT** retorna ao menu quando estiver em Game Over.
- 3 tipos de inimigos.
- Sistema de vidas, pontuação e high score durante a sessão.
- Progressão de nível e aumento de dificuldade.
- Power-up de tiro triplo.
- Power-up de escudo.
- Campo de estrelas animado.
- Efeitos de partículas e explosões.
- Renderização em `TFT_eSprite` para reduzir flicker.
- Loop-alvo de aproximadamente 60 FPS (`FRAME_MS = 16`).

## Hardware de referência

O projeto foi organizado para a mesma família de hardware utilizada no projeto HALO2600-RETRO:

- ESP32-S3 DEV MODULE
- Display TFT ILI9341 2.8\"
- Bluetooth BLE
- Aplicativo Dabble no modo GamePad

O código da v1.1.0 não utiliza Wi-Fi, LittleFS ou ROMs.

## Estrutura do repositório

```text
SPACE-SHOOTER-ESP32S3/
├── firmware/
│   └── SPACE_SHOOTER_ESP32S3/
│       └── SPACE_SHOOTER_ESP32S3.ino
├── docs/
│   ├── CONTROLS.md
│   ├── HARDWARE.md
│   ├── INSTALLATION.md
│   └── VERSIONING.md
├── licenses/
│   └── THIRD_PARTY_NOTICE.md
├── references/
│   └── original/
│       ├── space-shooter-v1.0.0.txt
│       └── space-shooter-v1.1.0.txt
├── .gitignore
├── CHANGELOG.md
├── CREDITS.md
├── FILE_LIST.txt
├── LICENSE-NOTICE.md
├── README.md
├── SHA256SUMS.txt
└── VERSION
```

## Instalação rápida

1. Abra a Arduino IDE.
2. Instale o suporte para ESP32 e selecione uma placa ESP32-S3 compatível.
3. Instale as bibliotecas **TFT_eSPI** e **DabbleESP32**.
4. Configure o `TFT_eSPI` para o seu ILI9341 e os pinos utilizados no seu hardware.
5. Abra:

```text
firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino
```

6. Compile e grave no ESP32-S3.
7. Abra o aplicativo Dabble, conecte-se ao dispositivo Bluetooth `ESP32-S3-GAMEPAD` e use o módulo GamePad.
8. Monitor Serial: **115200 baud**.

Veja `docs/INSTALLATION.md` para detalhes.

## Configuração TFT_eSPI

A configuração de hardware validada para **ESP32-S3 N16R8 + ILI9341 + XPT2046** está incluída em:

```text
config/TFT_eSPI/User_Setup.h
```

Antes de compilar, use esse arquivo como `User_Setup.h` da biblioteca `TFT_eSPI`. Ele contém a pinagem do display/touch e as frequências SPI usadas neste projeto.

## Controles

| Dabble | Função |
|---|---|
| D-pad UP/DOWN/LEFT/RIGHT | Mover a nave digitalmente |
| Joystick analógico X/Y | Mover a nave proporcionalmente |
| CROSS / CIRCLE | Atirar |
| START | Iniciar / pausar / continuar / reiniciar |
| SELECT | Voltar ao menu durante Game Over |

## Estado da v1.1.0

Esta versão adiciona suporte ao **joystick analógico** sem remover os controles digitais da v1.0.0. O D-pad tem prioridade quando acionado, evitando que drift do analógico altere a velocidade do movimento digital.

A `v1.0.0` permanece preservada como baseline inicial do projeto.

## Projeto relacionado

Estrutura de repositório inspirada no projeto do mesmo autor:

**HALO2600-RETRO — Atari 2600 Emulator for ESP32-S3**  
https://github.com/JonasKL3/HALO2600-RETRO___Atari-2600-Emulator-for-ESP32-S3

## Bibliotecas de terceiros

Este projeto utiliza bibliotecas externas, incluindo:

- TFT_eSPI
- DabbleESP32
- Arduino ESP32 core

Cada biblioteca permanece sujeita aos termos de sua própria licença. Veja `licenses/THIRD_PARTY_NOTICE.md`.

## Licença

Nenhuma licença ampla de redistribuição do código autoral é concedida automaticamente por este pacote. Consulte `LICENSE-NOTICE.md` antes de reutilizar ou redistribuir o código.
