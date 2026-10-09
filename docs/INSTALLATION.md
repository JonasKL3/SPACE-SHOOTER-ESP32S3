# Instalação — SPACE SHOOTER v3.0.0

## 1. Requisitos

- ESP32-S3 N16R8
- ILI9341 2.8" SPI
- Arduino IDE e core Arduino-ESP32
- Bibliotecas TFT_eSPI e DabbleESP32
- Dabble GamePad instalado no smartphone

`Preferences` está incluída no core ESP32.

## 2. Display

Use **`config/TFT_eSPI/User_Setup.h`** como configuração da TFT_eSPI, assegurando que a biblioteca carregue este setup em vez do padrão. Confira GPIOs, frequência de SPI e alimentação em `docs/HARDWARE.md`.

## 3. Código

Abra `firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino`. Selecione a placa ESP32-S3 correspondente. Não é necessário instalar bibliotecas para o XPT2046 nesta versão (touch não utilizado).

## 4. Compilar e gravar

Compile e grave o firmware. Caso a configuração TFT_eSPI contenha uma opção não suportada pela sua versão da biblioteca, revise-a antes de compilar; a configuração foi fornecida como referência do hardware do autor.

## 5. BLE

No Dabble, conecte ao dispositivo BLE `ESP32-S3-GAMEPAD`. Abra o módulo GamePad.

## 6. Conferência antes da publicação

A tela inicial deve mostrar `v3.0.0` no canto inferior direito. Siga `docs/TEST_PLAN_V3.md` para confirmar analógico, navegação, boss, NVS e skins no equipamento real.

**Nota:** o teste de sintaxe com APIs simuladas não substitui compilação Arduino nem teste em ESP32-S3.
