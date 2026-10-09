# Versionamento — SPACE SHOOTER

O projeto mantém a versão estável atual na branch `main` e preserva as versões anteriores por commits, tags e GitHub Releases.

| Versão | Foco |
|---|---|
| v1.0.0 | Lançamento inicial |
| v1.1.0 | Analógico e input |
| v2.0.0 | Menu, estatísticas, power-ups e gameplay |
| v3.0.0 | Campanha, bosses e skins |
| v4.0.0 | 20 níveis, 5 bosses, 6 armas, cenários e otimizações de performance |

## Processo de publicação

1. Testar a versão na placa real.
2. Extrair o ZIP do pacote oficial e copiar seu **conteúdo interno** para a raiz da branch `main` (não criar uma pasta extra de versão na raiz).
3. Fazer commit com o código e a documentação da nova versão.
4. Criar uma **nova tag** `v4.0.0` apontando para o commit atualizado da `main`.
5. Criar a GitHub Release `SPACE SHOOTER v4.0.0 — Arsenal & Bosses Update`, marcar como latest e anexar `SPACE-SHOOTER-ESP32S3-v4.0.0.zip` como arquivo binário.
6. Não alterar as tags/releases já publicadas. O ZIP anexado à Release não substitui os arquivos versionados na `main`.
