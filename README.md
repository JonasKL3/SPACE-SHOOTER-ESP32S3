# SPACE-SHOOTER-ESP32S3

**SPACE SHOOTER v6.0.0** — ESP32-S3 N16R8 + ILI9341 320×240 + Dabble BLE GamePad.

## Versão 6.0.0 — sem debug/Konami
- Campanha de 50 níveis, 10 bosses com três fases, Endless, 5 naves, 5 pets e upgrades.
- Bomba acionada pelo **SQUARE**, dash com **TRIANGLE** e armas evolutivas.
- Correções aprofundadas: fase presa no nível 2, inimigo blindado, tiros perfurantes, recompensa final, pausas, saves da v5 e confirmação de RESET DATA.
- **Não inclui o código Konami, popup de debug ou atalhos para 999.999 moedas.**

**Testes:** 89 minutos de gameplay **simulado**, 333.755 quadros e 2.882.412 verificações automatizadas (C++/ASan/UBSan). A validação de hardware físico ainda está pendente. Veja [notas de lançamento](docs/RELEASE_NOTES_V6.0.0.md).

## Instalação
1. Configure Arduino-ESP32, TFT_eSPI e DabbleESP32 para sua ESP32-S3 N16R8.
2. **Atenção ao erro `heap_caps_free`:** aplique/verifique o [patch da DabbleESP32](docs/DABBLE_BLE_RESET.md), que é externo ao firmware.
3. Configure TFT conforme [User_Setup.h](config/TFT_eSPI/User_Setup.h).
4. Compile e grave [o firmware](firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino). Abra Monitor Serial 115200 baud.
5. Teste por pelo menos 10 minutos reais, inclusive reconexões BLE, bosses, bomba, pets e pausa.

Versões anteriores continuam no [histórico de Releases](https://github.com/JonasKL3/SPACE-SHOOTER-ESP32S3/releases). Testes completos e logs estão no ZIP oficial preparado; o código da branch `release/v6.0.0` corresponde a esta versão.

[Changelog](CHANGELOG.md) · [Diagnóstico Dabble](docs/DABBLE_BLE_RESET.md) · [Instalação](docs/INSTALLATION.md)
