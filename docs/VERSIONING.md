# Versionamento

O projeto utiliza etiquetas `MAJOR.MINOR.PATCH` como histórico de lançamentos. A branch `main` mantém os arquivos da versão estável mais recente; cada GitHub Release está associada a uma tag e um estado específico do repositório.

- `PATCH` (ex.: `v3.0.1`): correções compatíveis.
- `MINOR` (ex.: `v3.1.0`): funcionalidades novas compatíveis.
- `MAJOR` (ex.: `v3.0.0`): atualização estrutural ou importante do jogo.

Para publicar: validar no hardware, fazer commit na `main`, criar a Release no GitHub com tag nova apontando para o commit da versão, e anexar o ZIP completo. Evite alterar ZIPs de releases anteriores após a publicação.

A v3.0.0 adiciona campanha, modo endless, chefes e seleção de skins. Ao migrar, mantenha a partição NVS para preservar estatísticas e desbloqueios.
