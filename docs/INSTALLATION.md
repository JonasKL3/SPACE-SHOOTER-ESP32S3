# Instalação — SPACE SHOOTER v5.0.0

Hardware: ESP32-S3 N16R8, ILI9341 320×240, Dabble BLE GamePad. Veja HARDWARE.md.

1. Instale Arduino-ESP32, TFT_eSPI e DabbleESP32.
2. **ATENÇÃO — bug de reset BLE**: a biblioteca DabbleESP32 pode provocar `assert failed: heap_caps_free`. Corrija sua instalação usando [DABBLE_BLE_RESET.md](DABBLE_BLE_RESET.md) **antes** da compilação. Atualizar a biblioteca pode remover o patch.
3. Ative o arquivo `config/TFT_eSPI/User_Setup.h` na TFT_eSPI.
4. Abra `firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino`, selecione a ESP32-S3 e PSRAM adequada, compile e grave.
5. Abra Monitor Serial em 115200 baud, conecte GamePad no `ESP32-S3-GAMEPAD` e teste direção, disparos, dash, menus, bosses e loja.
6. Preserve a NVS se quiser manter recordes, skins e progresso da v4.

Script Windows: feche Arduino IDE, baixe `tools/CORRIGIR-DABBLE-ESP32S3.ps1` para Downloads, abra a pasta no Explorer, digite `powershell` na barra de endereços e execute:
```powershell
powershell -ExecutionPolicy Bypass -File .\CORRIGIR-DABBLE-ESP32S3.ps1
```
O script cria backup e modifica apenas `DabbleESP32.cpp`. Reabra o Arduino IDE e **recompile**. Mais detalhes no guia BLE.
