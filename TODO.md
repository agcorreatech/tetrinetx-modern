# TO DO List

## Concluído

- ~~20/jan/2021 - Create commands to start / stop / restart the tetrinet
  service in linux~~ ✅ Feito — ver `contrib/systemd/` (unit file
  `tetrinetx.service` + `install-tetrinetx-service.sh` +
  `contrib/systemd/README.md`).

## Bugs

- ~~20/jan/2021 - Jogo não finaliza quando tem só 1 jogador e este
  perde~~ ✅ Corrigido em `src/main.c` (handler de `playerlost`). Ver
  `CHANGELOG.md`.
- ~~20/jan/2021 - Atualizar winlist a cada fim de jogo para todos os
  jogadores~~ ✅ Corrigido em `src/game.c` (`sendwinlist()`). Ver
  `CHANGELOG.md`.
