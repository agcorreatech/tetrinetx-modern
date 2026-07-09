# Changelog

## Unreleased

- **Fix:** partida não finalizava quando havia apenas 1 jogador na sala e
  este perdia. A verificação de fim de jogo em `playerlost` (`src/main.c`)
  exigia um oponente restante (`ns1 != NULL`) para disparar o encerramento;
  em uma partida solo isso nunca acontecia, deixando o canal preso em
  `STATE_INGAME` para sempre. Agora, quando não há nenhum outro jogador
  restante, o servidor encerra a partida normalmente e envia
  `pline 0 "-=== Game Over - no winner ===-"` para o jogador.
- **Fix:** a winlist (placar) não era enviada para todos os jogadores do
  canal ao final de uma partida. A função `sendwinlist()` (`src/game.c`)
  usava um `do...while` cuja condição de repetição (`n != NULL`) parava o
  laço logo na primeira iteração no modo "enviar para todos"
  (`sendwinlist(chan, NULL)`), fazendo com que só o primeiro jogador da
  lista interna recebesse a winlist atualizada. Corrigido para
  `n == NULL`, permitindo que o laço percorra toda a lista de jogadores
  quando o destino é "todos".
- **Added:** suporte a gerenciar o servidor como serviço `systemd` no
  Linux (`contrib/systemd/`), incluindo:
  - `tetrinetx.service`: unit file (`Type=forking`, já que o binário faz
    seu próprio double-fork/daemonize).
  - `install-tetrinetx-service.sh`: script que compila o servidor (se
    necessário), cria um usuário de sistema dedicado, instala os
    arquivos em `/opt/tetrinetx` e registra o serviço.
  - `README.md`: instruções de uso (`systemctl start|stop|restart|status
    tetrinetx`).
- **Security (reviewed):** CVE-1999-1060 (buffer overflow via long DNS
  hostname on connect, port 31457) revisada linha a linha. O vetor
  remoto real (`hostnamefromip()` em `src/net.c`, que resolve o
  hostname de quem conecta via `gethostbyaddr()`) já estava corrigido
  desde a Release 03, com `strncpy`/terminação nula corretas — não foi
  necessária nenhuma mudança de código para o CVE em si.
- **Hardening (relacionado, não é o CVE):** `getmyhostname()`
  (`src/net.c`), usada apenas para resolver o hostname do **próprio
  servidor** na inicialização (não é acionável remotamente por um
  jogador), ainda fazia `strcpy()` sem limite a partir da variável de
  ambiente `HOSTNAME` e do self-lookup de DNS. Mesmo não sendo o vetor
  do CVE-1999-1060, é a mesma classe de bug escrevendo no mesmo campo
  `n->host`; a função agora recebe o tamanho do buffer e usa
  `strncpy`/terminação nula em todas as cópias.

- **Removed:** `TODO` / `TODO.md` — todos os itens que estavam listados
  (bug do fim de jogo com 1 jogador, bug da winlist e o suporte a
  systemd) já foram implementados; o histórico do Git preserva o
  conteúdo antigo, caso seja necessário consultar.
- **Fixed (docs):** `README` apontava para um arquivo `CHANGELOG` que
  não existe mais (agora é `CHANGELOG.md`); corrigidos também pequenos
  erros de digitação ("implementetion", "beign", "Pronpt", "wls") e
  adicionada uma seção apontando para `contrib/systemd/README.md`.

_________________________________________________________________________________

## Release 03 - 24/Jun/2023

- Added a message to linux terminal when you run the server.
- Changed compile shell script to use bash instead of sh.
- Fixed issue CVE-1999-1060 Buffer overflow in Tetrix TetriNet daemon 1.13.16
  allows remote attackers to cause a denial of service
- Added information to README how to compile the server using Visual Studio Code
  and WSL (linux) for Windows.
- Added Tetrinet 1.13 for Windows (game client) original from St0rmCat.
- Added .gitignore file to skip some files

_________________________________________________________________________________

## Release 02 - 20/Jan/2021

- Fixed a bug that crashes the server while reading the motd file, that was
  optional. It has been created two new functions in "main.c" file to handle it:
  read_motd() and write_motd(). Now there is always a "message of the day" file.
  When you use the command "/motd", this info is shown again. Also added this to
  /help command.
- Fixed a bug in the function lprintf(), that was preventing some info to be
  wrote correctly to the LOG file.
- Changed the header message of the log file when you start the server.
- Changed the option "game.maxchannels" to "10" in "game.conf". The old parameter
  was "1", that did not allow to create more game channels by default.

_________________________________________________________________________________

## Release 01 - 18/Jan/2020

- Finished the review of the code and replaced the old library varargs.h
  by stdarg.h. The functions lvprintf(), tprintf() and lprintf() was rewritten.
  No more errors, now the code can be compilled with modern GCC versions.
- All returns of the functions fscanf() has been treated as it should be, and
  there is no more warnings about it in the compilation.
- Fixed some bugs with variable formats and type conversions everywhere, that
  was giving warnings in the compilation. Now it's compiling like a charm!
- The code is now full functional and can be used! It is the original version
  of the tetrinetx, but working in 2020!
