# Rodando o tetrinetx como serviço systemd (Linux)

Esta pasta contém os arquivos necessários para instalar o **TetriNET X Modern
Server** como um serviço gerenciado pelo `systemd`, permitindo iniciar,
parar e reiniciar o servidor de forma padronizada, com start automático no
boot da máquina.

## Arquivos

- `tetrinetx.service` — unit file do systemd.
- `install-tetrinetx-service.sh` — script que compila (se necessário),
  instala os arquivos em `/opt/tetrinetx`, cria um usuário de sistema
  dedicado (`tetrinetx`) e registra o serviço no systemd.

## Instalação

A partir da raiz do repositório:

```bash
cd contrib/systemd
sudo ./install-tetrinetx-service.sh
```

O script:
1. Compila o servidor (`src/compile.linux`) se o binário ainda não existir
   em `bin/tetrix-modern.linux`.
2. Cria o usuário/grupo de sistema `tetrinetx` (sem shell de login).
3. Copia o binário e os arquivos de configuração (`game.conf`, `game.motd`)
   para `/opt/tetrinetx`.
4. Instala `tetrinetx.service` em `/etc/systemd/system/`.
5. Executa `systemctl daemon-reload` e `systemctl enable tetrinetx`.

## Uso do dia a dia

> **Nota sobre a sintaxe:** o `systemctl` usa a ordem
> `systemctl <ação> <serviço>` (ex.: `systemctl start tetrinetx`), e não
> `systemctl tetrinetx <ação>`. Os comandos corretos são:

```bash
sudo systemctl start   tetrinetx    # inicia o servidor
sudo systemctl stop    tetrinetx    # para o servidor
sudo systemctl restart tetrinetx    # reinicia o servidor
sudo systemctl status  tetrinetx    # mostra status atual e últimas linhas de log
```

Outros comandos úteis:

```bash
sudo systemctl enable  tetrinetx    # garante start automático no boot (já feito pelo instalador)
sudo systemctl disable tetrinetx    # desativa o start automático no boot
journalctl -u tetrinetx -f          # acompanha o log do serviço em tempo real
```

## Sobre o funcionamento do serviço

O binário do `tetrinetx` faz o próprio *daemonize* (dá `fork()` duas vezes e
fecha `stdin`/`stdout`/`stderr`), e grava seu PID em
`/opt/tetrinetx/game.pid`. Por isso o unit file usa `Type=forking` e aponta
`PIDFile=/opt/tetrinetx/game.pid` — isso é o que permite ao systemd saber
quando o processo real subiu e monitorá-lo corretamente (incluindo reinícios
automáticos em caso de falha, via `Restart=on-failure`).

Os arquivos de configuração (`game.conf`, `game.motd`, `game.winlist`,
`game.log`, `game.pid`, etc.) usados pelo servidor são todos relativos ao
diretório de trabalho — por isso o `WorkingDirectory` no unit file aponta
para `/opt/tetrinetx`, onde tudo fica instalado.

O servidor escuta por padrão nas portas `31457` (telnet/jogo) e `31456`
(query) — ambas portas não-privilegiadas, então o serviço roda com um
usuário dedicado sem privilégios, sem necessidade de rodar como `root`.
