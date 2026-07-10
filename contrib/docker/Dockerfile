# syntax=docker/dockerfile:1
#
# TetriNET X Modern Server - Docker build
#
# Uso (a partir da raiz do repositório):
#   docker build -f contrib/docker/Dockerfile -t tetrinetx .
#   docker run -d --init --name tetrinetx -p 31457:31457 -p 31456:31456 tetrinetx
#
# O --init é importante: o binário do tetrinetx faz seu próprio "double-fork"
# ao daemonizar (ver contrib/docker/entrypoint.sh e contrib/docker/README.md),
# e sem um init de verdade como PID 1 do container, o processo órfão gerado
# por esse double-fork pode ficar temporariamente como zumbi até ser colhido.
#
# Veja contrib/docker/README.md para docker-compose e mais detalhes.

# ---------------------------------------------------------------------------
# Etapa 1: build - compila o binario com o compilador C
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS build

# build-essential (não só "gcc") -- garante libc6-dev junto (headers como
# signal.h, stdio.h, etc), que o pacote "gcc" sozinho nem sempre traz com
# --no-install-recommends. Sem isso o build falha com
# "fatal error: signal.h: No such file or directory".
RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY src/ ./src/

# O script original tem quebras de linha CRLF; normalizamos antes de rodar.
RUN sed -i -e 's/\r$//' src/compile.linux && \
    mkdir -p bin && \
    cd src && bash compile.linux

# ---------------------------------------------------------------------------
# Etapa 2: runtime - imagem final, enxuta, sem toolchain de compilacao
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

# Usuario/grupo dedicados, sem privilegios de login (mesma logica usada em
# contrib/systemd/install-tetrinetx-service.sh para a instalacao via systemd)
RUN groupadd --system tetrinetx && \
    useradd --system --gid tetrinetx --home-dir /opt/tetrinetx \
            --no-create-home --shell /usr/sbin/nologin tetrinetx

# /opt/tetrinetx = binario + arquivos de configuracao padrao (imagem, somente leitura na pratica)
# /data          = diretorio de trabalho real do servidor, feito para ser montado como volume
WORKDIR /opt/tetrinetx

COPY --from=build /src/bin/tetrix-modern.linux ./tetrix-modern.linux
COPY bin/game.conf ./defaults/game.conf
COPY bin/game.motd ./defaults/game.motd
COPY contrib/docker/entrypoint.sh /usr/local/bin/entrypoint.sh

# Normaliza defensivamente eventuais quebras de linha CRLF no entrypoint.sh
# (ex: um checkout feito no Windows antes do .gitattributes deste repositório
# existir, ou uma edição futura com um editor que insira CRLF). Sem isso, o
# shebang "#!/usr/bin/env bash" viraria "#!/usr/bin/env bash\r" -- um
# interpretador inválido -- e o container falharia silenciosamente ao tentar
# rodar o entrypoint.
RUN sed -i -e 's/\r$//' /usr/local/bin/entrypoint.sh && \
    mkdir -p /data && \
    chmod +x /usr/local/bin/entrypoint.sh ./tetrix-modern.linux && \
    chown -R tetrinetx:tetrinetx /opt/tetrinetx /data

USER tetrinetx

# 31457 = porta do jogo (telnet), 31456 = porta de query (ver src/main.h)
EXPOSE 31457/tcp 31456/tcp

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
