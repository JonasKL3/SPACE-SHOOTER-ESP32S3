# SPACE SHOOTER v6.0.0

Firmware publicado como **v6.0.0 sem modo debug/Konami**, a pedido do autor. A versão inclui bomba (SQUARE), cinco pets, campanha completa de 50 níveis, dez bosses, melhorias de upgrades e correções aprofundadas de colisões, economia e progressão.

## Correções preservadas
- Campanha não fica presa no nível 2.
- Inimigo blindado sofre dano corretamente; tiros perfurantes não atingem o mesmo inimigo/boss repetidamente.
- Recompensa final do boss Void é creditada mesmo após encerrar a partida.
- I-frames após dano, tempo de pausa congelado e prioridade do D-pad em AUTO.
- Cinco pets equipáveis, mísseis órfãos liberados e RESET DATA com confirmação.
- Saves antigos v5 importados uma vez para o namespace `shooter6`, sem apagar os dados de origem.

## Testes reproduzíveis
**16min20s de campanha, 16min Endless, 56min40s fuzz**; no total 89 minutos **simulados**, 333.755 frames e 2.882.412 verificações automatizadas com AddressSanitizer/UndefinedBehaviorSanitizer. A campanha cobriu 50 níveis e 10 bosses. O pacote ZIP contém `tests/`, mocks e relatório detalhado.

**Limitações:** isso não substitui compilação com o core real do Arduino-ESP32, nem teste físico de 10 minutos. O firmware não corrige automaticamente a instalação local da biblioteca DabbleESP32. Confira o guia `docs/DABBLE_BLE_RESET.md` antes de compilar.

## Download e histórico
Código principal: `firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino`; a branch `release/v6.0.0` mantém o snapshot desta versão mesmo após a publicação da v6.5.6.
