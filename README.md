# SPACE-SHOOTER-ESP32S3

**SPACE SHOOTER v5.0.0** para ESP32-S3 N16R8, TFT ILI9341 2,8″ (320×240) e Dabble BLE GamePad. O toque XPT2046 não é usado.

## Novidades da v5.0.0

- Campanha de **50 níveis**, **10 bosses** de três fases e modo Endless.
- **9 tipos de inimigos**, 6 armas e 5 cenários.
- **5 naves**, moedas e loja com **6 upgrades persistentes**.
- **15 conquistas**, dash (TRIANGLE), estatísticas e transições pixel art.
- Melhorias de desempenho herdadas da v4 e correções adicionais no inimigo blindado e na recompensa do boss final.

## ⚠️ Correção essencial — DabbleESP32 e reinicializações

Foi reproduzido na ESP32-S3 um reset aleatório com `assert failed: heap_caps_free`. O backtrace decodificado apontou para `DabbleClass::freeMemoryAllocated()` em `DabbleESP32.cpp:517`. A implementação pública aloca um vetor de ponteiros sem inicializá-los e pode tentar liberá-los após receber um quadro BLE incompleto. **O autor aplicou o patch e confirmou que os resets observados pararam.** Isso não exclui outros tipos de reset.

**O firmware sozinho não contém essa correção: ela precisa ser aplicada na biblioteca DabbleESP32 instalada no computador.** Siga obrigatoriamente o guia [docs/DABBLE_BLE_RESET.md](docs/DABBLE_BLE_RESET.md) nas instalações afetadas, especialmente após reinstalar ou atualizar a biblioteca. Script de correção com backup: [tools/CORRIGIR-DABBLE-ESP32S3.ps1](tools/CORRIGIR-DABBLE-ESP32S3.ps1).

## Instalação

1. Instale core Arduino-ESP32, TFT_eSPI e DabbleESP32.
2. Corrija a DabbleESP32 conforme [guia BLE](docs/DABBLE_BLE_RESET.md).
3. Configure a TFT_eSPI usando [config/TFT_eSPI/User_Setup.h](config/TFT_eSPI/User_Setup.h).
4. Abra [firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino](firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino), compile e grave na ESP32-S3 N16R8.
5. Conecte o módulo GamePad do Dabble ao dispositivo BLE `ESP32-S3-GAMEPAD`.

Manual: [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Controles e progresso

D-pad/analógico movimentam; CROSS/CIRCLE atiram; TRIANGLE dá dash; START pausa; SELECT volta. Na loja, selecione naves/upgrades e confirme com CROSS. Skins do menu: VERDE, DOURADA, CIANO e ROXA (índices internos preservados). O namespace NVS `shooter` mantém score e progresso da v4.

## Documentação

- [docs/RELEASE_NOTES_V5.md](docs/RELEASE_NOTES_V5.md) — novidades e correções.
- [docs/DEEP_GAMEPLAY_TESTS_V5.md](docs/DEEP_GAMEPLAY_TESTS_V5.md) — alcance e limites das simulações.
- [docs/DABBLE_BLE_RESET.md](docs/DABBLE_BLE_RESET.md) — diagnóstico do ELF, correção e reversão.
- [CHANGELOG.md](CHANGELOG.md) — histórico de versões.

Tags e Releases anteriores à v5 são mantidas no GitHub. A branch `main` contém o firmware oficial mais recente.
