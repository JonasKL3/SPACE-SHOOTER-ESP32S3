# Verificação de software v6.6.0 — ESP32-S3

## Ambiente

Compilação hospedada com g++ C++17, AddressSanitizer e UndefinedBehaviorSanitizer, usando stubs de TFT_eSPI, Arduino, NVS, DabbleESP32 e, no teste específico, cabeçalhos simulados do ESP32/ESP-IDF. O arquivo `.ino` real do pacote é incluído nos testes, não uma reimplementação separada.

## Testes

| Teste | Alcance | Resultado |
| --- | --- | --- |
| `test_regressions.cpp` | 40 verificações: migração v5, colisões, 5 pets, debug, reset, i-frames | Aprovado |
| `test_v656_features.cpp` | 94 verificações: boss intro, Dabble, bombas, progresso e controle | Aprovado |
| `test_v660_features.cpp` | 90 verificações: bloqueio/desbloqueio Rush, 20 skins, save individual, menus aninhados, reset | Aprovado |
| `test_boss_rush_long.cpp` | 663.168ms simulados; 10 bosses e 10 intros; 207.252 verificações | Aprovado |
| `test_long_gameplay.cpp` | Campanha 611.568ms (50 níveis/10 bosses) + Endless 960.000ms | Aprovado |
| `test_fuzz.cpp` | Cinco sessões de 680s; 346 derrotas/reinícios, sem violações de memória detectadas | Aprovado |
| Diagnósticos ESP32 simulados | Compilação e execução dos trechos protegidos por `#if defined(ESP32)` | Aprovado |

**Limitações:** o teste de longa duração usa relógio de jogo simulado e roteiro acelerado para atingir fases/bosses; não constitui teste físico nem mede taxa real de FPS. Não substitui compilação com bibliotecas físicas Arduino-ESP32, DabbleESP32, TFT_eSPI ou medições de heap/stack no ESP32-S3. Portanto, não foi verificada a estabilidade do BLE físico, SPI, flash NVS real ou fonte de alimentação.

## Reprodução

No Linux com g++/ASan/UBSan: `bash tests/run_all_tests.sh` na raiz do ZIP/repositório. Arquivos `tests/stubs/` simulam hardware. Para teste Arduino IDE, use a pasta `firmware/SPACE_SHOOTER_ESP32S3` e o patch externo da biblioteca DabbleESP32 quando necessário.
