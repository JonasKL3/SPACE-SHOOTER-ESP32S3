# SPACE-SHOOTER-ESP32S3

**SPACE SHOOTER v6.5.6 (hotfix Arduino IDE aplicado)** — ESP32-S3 N16R8 / ILI9341 320×240 / Dabble BLE GamePad.

## Novidades da v6.5.6
- **Fases de 12 segundos**, preparação segura da arena e **entrada animada do boss**.
- **Bomba SQUARE** com contador de cargas e barra de recarga.
- Modo de debug **CROSS + SELECT enquanto PAUSED** e código Konami da loja (ambos dão moedas).
- Diagnóstico de reset, heap/PSRAM/stack e tempos dos subsistemas pelo Serial.
- Tratamento diferido de morte de boss e divisão de inimigos durante a verificação de colisões.

## Correções preservadas
- Campanha de **50 níveis**, 10 bosses e três fases, loja, cinco pets e melhorias.
- Colisão do inimigo blindado e tiros perfurantes, recompensa final do Void, invulnerabilidade após dano, pausa segura e prioridade do D-pad.
- Importação dos saves da v5 para o namespace `shooter6` e **confirmação para RESET DATA**.

## Testes
Na simulação C++17 com ASan/UBSan: **10min11s** campanha (50 níveis, 10 bosses), **16min** Endless, **56min40s** estresse aleatório (346 derrotas/reinícios). Total de **82min51s simulados**, 310.723 frames e 2.703.662 verificações, sem erro de memória detectado.

**Validação atualizada:** o autor compilou com sucesso na Arduino IDE e testou o firmware com o hotfix `feedKonami(int k)` na ESP32-S3. A duração do teste físico não foi informada; os 82min51s descritos acima são **simulados**. A biblioteca DabbleESP32 precisa manter o patch externo contra o antigo `heap_caps_free`.

## Instalação
1. Verifique core Arduino-ESP32, TFT_eSPI, DabbleESP32 e a [correção da biblioteca DabbleESP32](docs/DABBLE_BLE_RESET.md).
2. Confira a configuração [TFT_eSPI](config/TFT_eSPI/User_Setup.h).
3. Abra [firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino](firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino), compile e grave na placa ESP32-S3.
4. Abra o Monitor Serial em 115200 baud e verifique `[RESET_DIAG]`, `[HEALTH]` e `[SUBSYSTEMS]`.

**Versões antigas:** a [branch `release/v6.0.0`](../../tree/release/v6.0.0) mantém a versão **sem debug/Konami**. As versões v1 a v5 permanecem preservadas no histórico de Releases e tags.

[Notas v6.5.6](docs/RELEASE_NOTES_V6.5.6.md) · [Dabble BLE](docs/DABBLE_BLE_RESET.md) · [CHANGELOG](CHANGELOG.md)
