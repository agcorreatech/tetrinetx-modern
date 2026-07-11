# Rodando o tetrinetx com Docker / Docker Desktop

Esta pasta contém os arquivos para compilar e rodar o **TetriNET X Modern
Server** dentro de um container Docker.

## Arquivos

- `Dockerfile` — build multi-stage: compila o binário com `gcc` (Ubuntu
  24.04) em uma etapa, e copia só o binário + configs para uma imagem final
  enxuta, rodando com um usuário sem privilégios.
- `entrypoint.sh` — script de entrada do container (ver explicação abaixo,
  seção "Por que existe um entrypoint.sh").
- `docker-compose.yml` — forma mais simples de subir o serviço, já com
  portas e volume configurados; é o caminho recomendado para uso no Docker
  Desktop.
- `../.dockerignore` (raiz do repo) — evita levar `.git`, binário antigo,
  cliente Windows, etc. para o contexto de build.

## Pré-requisitos

- **Docker Desktop** instalado e aberto (Windows, macOS ou Linux).
- Não precisa ter `gcc` nem nada de toolchain C instalado na sua máquina —
  tudo isso acontece dentro do container de build.

## Uso rápido (recomendado): docker compose

A partir da **raiz do repositório**:

```bash
docker compose -f contrib/docker/docker-compose.yml up -d --build
```

Isso vai:
1. Compilar a imagem (`gcc` + `compile.linux`, exatamente como descrito no
   `README` principal do projeto).
2. Subir o container `tetrinetx` em segundo plano.
3. Publicar as portas `31457` (jogo) e `31456` (query) em `localhost`.
4. Criar um volume Docker `tetrinetx-data` para persistir `game.conf`,
   `game.motd`, `game.winlist`, `game.log` e `game.pid` entre reinícios.

Outros comandos úteis:

```bash
docker compose -f contrib/docker/docker-compose.yml logs -f      # acompanhar logs em tempo real
docker compose -f contrib/docker/docker-compose.yml restart      # reiniciar o servidor
docker compose -f contrib/docker/docker-compose.yml down         # parar e remover o container (mantém o volume/dados)
docker compose -f contrib/docker/docker-compose.yml down -v      # parar e remover TAMBÉM o volume (apaga winlist/config)
```

## Usando o Docker Desktop (interface gráfica)

Depois de rodar o `docker compose up -d --build` uma primeira vez pelo
terminal, o dia a dia pode ser feito direto pela interface do Docker
Desktop:

- Aba **Containers**: veja o container `tetrinetx` rodando, com as portas
  `31457`/`31456` mapeadas, o status (running/stopped) e um botão para
  **Start / Stop / Restart**.
- Clique no nome do container para ver **Logs** em tempo real (equivalente
  ao `docker compose logs -f`) e a aba **Files**, onde dá pra navegar até
  `/data` e conferir/editar `game.conf`, `game.motd`, `game.log` e
  `game.winlist` diretamente pela interface.
- Aba **Volumes**: o volume `tetrinetx-data` aparece listado, com os
  mesmos arquivos.
- Aba **Images**: a imagem `tetrinetx:local` fica disponível para rebuild
  (botão de rebuild ou apagar/recriar).

## Uso sem docker-compose (docker build / docker run direto)

Também a partir da raiz do repositório:

```bash
docker build -f contrib/docker/Dockerfile -t tetrinetx .
docker run -d --init --name tetrinetx \
  -p 31457:31457 -p 31456:31456 \
  -v tetrinetx-data:/data \
  tetrinetx
```

> **Por que `--init`?** Ver a seção "Zumbis e o `--init`" mais abaixo.

```bash
docker logs -f tetrinetx      # ver logs
docker restart tetrinetx      # reiniciar
docker stop tetrinetx         # parar
docker start tetrinetx        # iniciar de novo
docker rm -f tetrinetx        # remover o container (mantém o volume)
```

## Editando a configuração (`game.conf` / `game.motd`)

Os arquivos ficam no volume `tetrinetx-data`, montado em `/data` dentro do
container. Para editar `game.conf` (por exemplo, mudar `maxchannels` ou
`verbose`) sem precisar reconstruir a imagem:

```bash
docker compose -f contrib/docker/docker-compose.yml exec tetrinetx sh -c "cat /data/game.conf"
```

Edite localmente e copie de volta, ou edite diretamente pela aba **Files**
do Docker Desktop (Containers → tetrinetx → Files → `/data`). Depois,
reinicie o container para a nova configuração ser lida:

```bash
docker compose -f contrib/docker/docker-compose.yml restart
```

## Por que existe um `entrypoint.sh`?

O binário do `tetrinetx` já faz seu próprio *daemonize* internamente: ele
dá `fork()` duas vezes, fecha `stdin`/`stdout`/`stderr` e o processo "pai"
(o que foi chamado originalmente) termina assim que o daemon real sobe em
segundo plano — isso é visível em `src/main.c` (`writepid()` e a lógica
logo antes dele).

Isso conflita com o modelo padrão de containers, onde o **Docker espera
que o processo principal (PID 1) continue rodando em primeiro plano** — se
simplesmente chamássemos o binário direto como `ENTRYPOINT`, o container
encerraria imediatamente, pois o processo que o Docker está observando sai
na hora, mesmo com o servidor real ainda rodando em segundo plano.

O `entrypoint.sh` contorna isso:
1. Inicia o binário normalmente (ele mesmo daemoniza e retorna na hora).
2. Espera o arquivo `game.pid` aparecer.
3. Fica em primeiro plano acompanhando o log e monitorando se o processo
   real (PID do `game.pid`) continua vivo.
4. Quando esse processo cai (por erro ou por ter recebido `SIGTERM`), o
   script também encerra — e o container reflete corretamente o estado
   real do servidor (parado = parado, rodando = rodando).

## Sobre o diretório de dados (`/data`) vs. a imagem (`/opt/tetrinetx`)

- `/opt/tetrinetx` dentro da imagem contém apenas o binário compilado.
- `/data` é o diretório de trabalho real do servidor (onde ele lê/escreve
  `game.conf`, `game.motd`, `game.winlist`, `game.log`, `game.pid`) e é o
  que fica no volume Docker.
- Na primeira execução, o **próprio binário** gera `game.conf`,
  `game.motd` e `game.secure` em `/data` com os padrões embutidos nele —
  e nunca sobrescreve o que já existir lá, então qualquer alteração que
  você fizer em `/data/game.conf` é preservada entre reinícios e rebuilds
  da imagem. (Versões antigas da imagem carregavam uma cópia "modelo"
  desses arquivos em `/opt/tetrinetx/defaults/` e o `entrypoint.sh` os
  copiava no primeiro boot; isso deixou de ser necessário quando os
  padrões passaram a ser embutidos no binário.)

## Zumbis e o `--init`

O binário do `tetrinetx` daemoniza dando `fork()` **duas vezes**: o
processo intermediário sai logo depois do segundo fork, deixando o
processo neto (o servidor de verdade) órfão, que é então "adotado" pelo
processo `init` do sistema (normalmente PID 1), cuja responsabilidade é
"colher" (`wait()`) os processos órfãos quando eles terminam, liberando a
entrada deles da tabela de processos.

Um container Docker, por padrão, **não tem um init de verdade como PID
1** — o próprio `entrypoint.sh` (ou o binário) assume esse papel. Isso
significa que, sem ajuda, um processo órfão criado por esse double-fork
pode ficar temporariamente como **zumbi** (`<defunct>`) depois de
terminar, até que alguém o colha.

A solução padrão do Docker para isso é a flag `--init` (equivalente à
opção `init: true` no `docker-compose.yml`, já configurada aqui), que
injeta um init minimalista (`tini`) como PID 1 do container — feito
exatamente para colher órfãos e zumbis corretamente. Por isso ela já
vem habilitada por padrão no `docker-compose.yml` deste projeto, e é
recomendada também no `docker run` direto.

## Portas

O servidor escuta por padrão em duas portas (definidas em `src/main.h`):

- `31457/tcp` — porta do jogo (telnet/protocolo TetriNET)
- `31456/tcp` — porta de query

Ambas já vêm expostas (`EXPOSE`) no `Dockerfile` e publicadas
(`ports:`) no `docker-compose.yml`.
