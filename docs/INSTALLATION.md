# Instalação — SPACE SHOOTER v4.0.0

1. Instale o core Arduino-ESP32 e selecione a placa ESP32-S3 N16R8 correspondente.
2. Instale as bibliotecas **TFT_eSPI** e **DabbleESP32**. `Preferences` integra o core ESP32.
3. Configure a TFT_eSPI com `config/TFT_eSPI/User_Setup.h` deste repositório, confirmando que esse setup está efetivamente ativo.
4. Abra `firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino` na Arduino IDE.
5. Compile, grave e conecte o GamePad do Dabble ao nome BLE `ESP32-S3-GAMEPAD`.
6. Confirme se o menu inicial mostra `v4.0.0`; teste direções, tiro, navegação, HUD e desempenho.
7. Se for atualização da v3, **não apague a partição NVS**, para manter recorde, estatísticas e skins. A conclusão dos 20 níveis possui registro próprio.

> **Atenção:** `RESET DATA` limpa os dados salvos de jogo depois de uma confirmação explícita. Cuidado ao testar esse menu.

Para ver as métricas de desempenho, pausar durante o jogo e consultar `docs/PERFORMANCE_V4.md`.
Para validar o lançamento, use `docs/TEST_PLAN_V4.md`.
