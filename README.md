# SPACE SHOOTER ESP32-S3 — v6.6.0

Nova atualização do jogo baseada na **v6.5.6 corrigida no Arduino IDE e testada pelo autor na placa**. A v6.6.0 adiciona o Boss Rush, novos modelos de nave e seleção individual de skins, além de um menu de Configurações.

## Novidades

- **Boss Rush:** dez bosses consecutivos, começando pelo Sentinel e terminando no Void, cada um com entrada animada. Só pode ser iniciado após vencer a campanha **ou** ativar o modo debug (X + SELECT na pausa ou Konami na loja). O desbloqueio por debug fica salvo como conquista; RESET DATA apaga esse desbloqueio.
- **Cinco naves com silhuetas próprias:** Padrão Interceptor, Exploradora Scout, Pesada Mech, Laser Lance e Caçadora Hunter, com asa, casco, cockpit, núcleos, propulsores e armamentos distintos. O desenho é feito por primitivas TFT sem bitmaps externos.
- **Quatro skins por nave, 20 combinações identificadas por nomes próprios.** Abra `NAVES`, escolha a nave, pressione **RIGHT** para entrar em **SKINS** e depois **CROSS** para selecionar. As opções são salvas separadamente por nave.
- **CONFIGURACOES:** concentra CONTROLES, STATUS e RESET DATA, que mantém a confirmação em duas etapas.

## Modos de jogo

| Modo | Disponibilidade | Como funciona |
| --- | --- | --- |
| Campanha | Inicial | 50 níveis e 10 bosses |
| Endless | Inicial | Sobrevivência sem limite de níveis |
| Boss Rush | Vitória da campanha ou debug | 10 bosses em sequência, sem ondas normais; finalização após o Void |

## Compatibilidade e instalação

- Hardware: ESP32-S3 N16R8, display ILI9341 320×240, Arduino IDE, bibliotecas TFT_eSPI e DabbleESP32.
- O namespace **`shooter6`** da NVS permanece igual. Saves existentes (moedas, conquistas, naves, campanha e upgrades) são preservados. A skin antiga da v6.5.6 é utilizada como valor inicial das cinco novas skins se ainda não existirem chaves individuais (`skin0` … `skin4`).
- A biblioteca DabbleESP32 instalada localmente ainda precisa ter a [correção do `heap_caps_free`](docs/DABBLE_BLE_RESET.md) quando afetada.
- Para compilar: abra `firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino` no Arduino IDE, verifique a configuração TFT e grave na placa.
- Evite abrir `.ino` diretamente de uma pasta temporária do WinRAR; extraia o ZIP em uma pasta persistente.

## Verificações

`bash tests/run_all_tests.sh` executa C++17, AddressSanitizer e UndefinedBehaviorSanitizer em um ambiente de **mocks de Arduino, TFT, BLE e NVS**. Inclui campanha, Endless, Boss Rush completo e estresse aleatório. **Esses testes não equivalem à compilação no core ESP32 real nem à validação de Bluetooth ou display físico.**

Veja [relatório técnico](docs/RELATORIO_TESTES_v6.6.0.md) e [CHANGELOG](CHANGELOG.md).
