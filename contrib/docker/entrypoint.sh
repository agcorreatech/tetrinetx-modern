#!/usr/bin/env bash
#
# entrypoint.sh
#
# O binario do tetrinetx faz seu proprio "daemonize" (da fork() duas vezes,
# fecha stdin/stdout/stderr e o processo pai termina imediatamente -- e
# assim que ele grava o PID em game.pid, ver src/main.c:writepid()).
#
# Isso e incompativel com o modelo padrao de container, onde o Docker espera
# que o processo principal (PID 1) fique rodando em primeiro plano: se a
# gente so chamasse o binario direto, o container encerraria na hora, pois
# o processo "pai" (o que o Docker esta observando) sai assim que o daemon
# real sobe em segundo plano.
#
# Este script contorna isso:
#   1. Inicia o binario normalmente (ele mesmo daemoniza e retorna na hora)
#   2. Espera o arquivo game.pid aparecer
#   3. Fica em primeiro plano monitorando se o processo (PID do game.pid)
#      continua vivo -- e assim que ele cair (crash ou SIGTERM tratado),
#      o container encerra tambem, refletindo o estado real do servico.
#   4. Repassa SIGTERM/SIGINT recebidos pelo Docker para o processo real.
#
set -euo pipefail

APPDIR="/opt/tetrinetx"
DATADIR="/data"
BINARY="${APPDIR}/tetrix-modern.linux"
PIDFILE="${DATADIR}/game.pid"
LOGFILE="${DATADIR}/game.log"

# Nao e preciso copiar arquivos de configuracao padrao: o proprio binario
# gera game.conf, game.motd e game.secure com os padroes embutidos quando
# eles nao existem no diretorio de trabalho (e nunca sobrescreve o que ja
# existir la, entao edicoes no volume persistem entre reinicios).
mkdir -p "${DATADIR}"

cd "${DATADIR}"

cleanup() {
  if [[ -f "${PIDFILE}" ]]; then
    local pid
    pid="$(cat "${PIDFILE}" 2>/dev/null || true)"
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
      kill -TERM "${pid}" 2>/dev/null || true
    fi
  fi
}
trap cleanup SIGTERM SIGINT

echo "Iniciando tetrinetx..."
"${BINARY}"

# Espera o processo daemonizado escrever o PID (normalmente e quase
# instantaneo, mas damos uma folga generosa)
for _ in $(seq 1 50); do
  [[ -f "${PIDFILE}" ]] && break
  sleep 0.1
done

if [[ ! -f "${PIDFILE}" ]]; then
  echo "Erro: o servidor nao gravou ${PIDFILE} -- provavelmente falhou ao iniciar." >&2
  exit 1
fi

SERVER_PID="$(cat "${PIDFILE}")"
echo "tetrinetx rodando com PID ${SERVER_PID} (log em ${LOGFILE})"

# Mantem o container em primeiro plano acompanhando o log (se/quando
# existir) e monitorando se o processo real ainda esta vivo.
touch "${LOGFILE}"
tail -F "${LOGFILE}" &
TAIL_PID=$!

while kill -0 "${SERVER_PID}" 2>/dev/null; do
  sleep 1
done

echo "tetrinetx (PID ${SERVER_PID}) encerrou."
kill "${TAIL_PID}" 2>/dev/null || true
