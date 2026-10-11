# SPACE SHOOTER v6.6.0 — Boss Rush, Mechapunk & Configurações

Esta atualização mantém a base **v6.5.6 Arduino Hotfix** (`feedKonami(int k)`), corrigida e compilada/testada pelo autor na placa, e adiciona:

1. **Boss Rush:** dez bosses em sequência com intro, barra de HP e progressão própria. Exige campanha concluída ou debug; desbloqueio via conquista Konami permanece salvo. Terminar Rush não marca indevidamente a campanha como vencida.
2. **Cinco cascos com visual distinto:** interceptador angular, scout de asas longas, mecha blindado, laser rail e caçadora de quatro asas.
3. **Skins por nave:** quatro visuais coloridos por casco, nomes personalizados, acesso na seleção de naves pelo botão RIGHT e salvamento NVS independente.
4. **Configurações:** Controles, Status e RESET DATA foram movidos do menu principal; RESET continua pedindo confirmação.

**Testes com mocks C++:** Boss Rush de **11min03s simulados**, campanha de **10min11s**, Endless de **16min**, fuzz de **56min40s**; pelo menos **93min54s simulados** no conjunto. Testes de regressão, navegação dos menus, migração NVS e desenho dos vinte visuais executados com AddressSanitizer + UndefinedBehaviorSanitizer.

**Observação:** o firmware v6.6.0 ainda não foi compilado nem testado na ESP32-S3 física nesta entrega; antes de considerar estável, o autor deve compilar no Arduino IDE e testar principalmente BLE, sprites maiores, navegação e NVS. Essa atualização não resolve automaticamente o problema da DabbleESP32 externa.
