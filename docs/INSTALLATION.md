# Instalação — SPACE SHOOTER v2.0.0

## 1. Requisitos

- ESP32-S3 N16R8
- ILI9341 2.8\"
- Arduino IDE ou ambiente compatível com Arduino ESP32
- Biblioteca TFT_eSPI
- Biblioteca DabbleESP32

A biblioteca Preferences faz parte do core ESP32.

## 2. Configurar TFT_eSPI

O projeto inclui a configuração utilizada nos testes:

`config/TFT_eSPI/User_Setup.h`

Copie esse arquivo para a localização de configuração da sua instalação da TFT_eSPI, substituindo ou selecionando o setup conforme o método utilizado no seu ambiente.

A pinagem completa está documentada em `docs/HARDWARE.md`.

## 3. Firmware

Abra:

`firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino`

Selecione a placa ESP32-S3 correspondente ao seu hardware e compile.

## 4. Controle

No smartphone, utilize o módulo GamePad do Dabble e conecte ao dispositivo BLE:

`ESP32-S3-GAMEPAD`

## 5. Teste recomendado

Após gravar:

1. Verifique a tela inicial e a identificação `v2.0.0` no canto inferior direito.
2. Teste D-pad nas quatro direções.
3. Teste o analógico e confirme que a nave para no centro.
4. Teste CROSS/CIRCLE.
5. Teste START e SELECT.
6. Entre em SCORE e volte ao menu.
7. Verifique se high score e estatísticas permanecem após reiniciar o ESP32.
