# SPACE SHOOTER v6.5.6 — Notas de lançamento

Nova versão integrada sobre a base testada, preservando as correções da v6.0.0.

### Recursos
- Fases normais de 12 segundos, preparação da arena com espera máxima de 6 segundos.
- Cutscene de boss descendente, com easing e aviso visual.
- Bomba no SQUARE: contador de cargas, cooldown e barra de recarga.
- X + SELECT durante a pausa: modo debug e crédito de moedas. Código Konami também disponível na loja.
- Diagnósticos `[RESET_DIAG]`, `[HEALTH]` e `[SUBSYSTEMS]` com breadcrumbs RTC, heap/PSRAM/stack e tempos máximos.
- Spawns e morte de boss adiados para não reutilizar slots no meio de loops de colisão.

### Correções mantidas
Todos os 5 pets, migração de saves v5, RESET DATA confirmado, blindado corrigido, hits únicos perfurantes, prioridade de D-pad, recompensa final e NVS fora de gameplay ativo.

### Testes
- Campanha 50 níveis / 10 bosses / 3 fases: **611.568 ms** (10min11s simulados).
- Endless: **960.000 ms** (16min simulados), nível 77.
- Fuzz: **5 × 680 s = 56min40s**, 346 derrotas/reinícios.
- **2.703.662 verificações**, 310.723 quadros nos testes prolongados, AddressSanitizer/UndefinedBehaviorSanitizer sem falhas detectadas.

**Riscos conhecidos:** a compilação com Arduino-ESP32 real e a validação na placa física ainda estão pendentes. O patch do DabbleESP32 é externo ao firmware e deve estar instalado antes de recompilar. Observe os diagnósticos no Serial.

A branch `release/v6.0.0` preserva o snapshot anterior **sem Konami/debug**, conforme solicitado.
