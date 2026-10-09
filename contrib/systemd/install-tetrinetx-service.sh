#!/usr/bin/env bash
#
# install-tetrinetx-service.sh
#
# Instala o TetriNET X Modern Server como um servico systemd (tetrinetx),
# permitindo gerenciar o servidor com:
#
#   sudo systemctl start   tetrinetx
#   sudo systemctl stop    tetrinetx
#   sudo systemctl restart tetrinetx
#   sudo systemctl status  tetrinetx
#
# O que este script faz:
#   1. Compila o servidor (se o binario ainda nao existir em bin/)
#   2. Cria um usuario/grupo de sistema dedicado "tetrinetx" (sem login)
#   3. Copia o binario + arquivos de configuracao para /opt/tetrinetx
#   4. Instala o unit file contrib/systemd/tetrinetx.service em
#      /etc/systemd/system/
#   5. Recarrega o systemd e habilita o servico (start automatico no boot)
#
# Uso:
#   sudo ./install-tetrinetx-service.sh
#
set -euo pipefail

# --- Configuraveis ---------------------------------------------------------
INSTALL_DIR="/opt/tetrinetx"
SERVICE_USER="tetrinetx"
SERVICE_GROUP="tetrinetx"
UNIT_FILE_NAME="tetrinetx.service"
SYSTEMD_DIR="/etc/systemd/system"

# --- Caminhos relativos a este script --------------------------------------
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../.." >/dev/null 2>&1 && pwd)"
SRC_DIR="${REPO_ROOT}/src"
BIN_DIR="${REPO_ROOT}/bin"
BINARY_NAME="tetrix-modern.linux"
UNIT_FILE_SRC="${SCRIPT_DIR}/${UNIT_FILE_NAME}"

# --- Checagens basicas ------------------------------------------------------
if [[ "${EUID}" -ne 0 ]]; then
  echo "Este script precisa ser executado como root (use: sudo $0)" >&2
  exit 1
fi

if [[ ! -f "${UNIT_FILE_SRC}" ]]; then
  echo "Erro: nao encontrei ${UNIT_FILE_SRC}." >&2
  exit 1
fi

# --- 1. Compilar o servidor, se necessario ---------------------------------
if [[ ! -x "${BIN_DIR}/${BINARY_NAME}" ]]; then
  echo "[1/5] Binario nao encontrado, compilando o servidor..."
  (cd "${SRC_DIR}" && sed -i -e 's/\r$//' compile.linux && bash compile.linux)
else
  echo "[1/5] Binario ja existe em ${BIN_DIR}/${BINARY_NAME}, pulando compilacao."
fi

if [[ ! -x "${BIN_DIR}/${BINARY_NAME}" ]]; then
  echo "Erro: a compilacao nao gerou ${BIN_DIR}/${BINARY_NAME}." >&2
  exit 1
fi

# --- 2. Usuario/grupo de sistema dedicados ---------------------------------
echo "[2/5] Configurando usuario/grupo de sistema '${SERVICE_USER}'..."
if ! getent group "${SERVICE_GROUP}" >/dev/null 2>&1; then
  groupadd --system "${SERVICE_GROUP}"
fi
if ! id -u "${SERVICE_USER}" >/dev/null 2>&1; then
  useradd --system --gid "${SERVICE_GROUP}" --home-dir "${INSTALL_DIR}" \
          --no-create-home --shell /usr/sbin/nologin "${SERVICE_USER}"
fi

# --- 3. Copiar binario e arquivos de configuracao para /opt/tetrinetx ------
echo "[3/5] Instalando arquivos em ${INSTALL_DIR}..."
mkdir -p "${INSTALL_DIR}"

cp -f "${BIN_DIR}/${BINARY_NAME}" "${INSTALL_DIR}/"

# Arquivos de configuracao: soh copia se ainda nao existirem em destino,
# para nao sobrescrever uma configuracao ja em producao em reinstalacoes.
for f in game.conf game.motd; do
  if [[ -f "${BIN_DIR}/${f}" && ! -f "${INSTALL_DIR}/${f}" ]]; then
    cp -f "${BIN_DIR}/${f}" "${INSTALL_DIR}/"
  fi
done

chown -R "${SERVICE_USER}:${SERVICE_GROUP}" "${INSTALL_DIR}"
chmod 750 "${INSTALL_DIR}"
chmod 755 "${INSTALL_DIR}/${BINARY_NAME}"

# --- 4. Instalar o unit file ------------------------------------------------
echo "[4/5] Instalando ${UNIT_FILE_NAME} em ${SYSTEMD_DIR}..."
cp -f "${UNIT_FILE_SRC}" "${SYSTEMD_DIR}/${UNIT_FILE_NAME}"
chmod 644 "${SYSTEMD_DIR}/${UNIT_FILE_NAME}"

# --- 5. Recarregar e habilitar o servico -----------------------------------
echo "[5/5] Recarregando systemd e habilitando o servico..."
systemctl daemon-reload
systemctl enable tetrinetx.service

echo
echo "Instalacao concluida."
echo "Use 'sudo systemctl start tetrinetx' para iniciar o servidor agora."
echo "Veja ${SCRIPT_DIR}/README.md para mais detalhes de uso."
