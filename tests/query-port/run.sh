#!/usr/bin/env bash
#
# query-port/run.sh
#
# End-to-end test of the server's plain-text query commands: instead of
# sending the encrypted "tetrisstart <nick> <version>" INIT string, a
# client can send one of a handful of plain-text commands right after
# connecting, and the server answers directly -- no encryption needed.
# See net_telnet_init() (src/main.c) for the exact list.
#
# IMPORTANT FINDING (documented here since it surprised us during initial
# testing): despite the name, these do NOT require the separate "query
# port" (31456/tcp, QUERY_PORT in src/main.h). That port is currently NOT
# listening at all -- init_query_port() is commented out in main()
# (src/main.c). These plain-text commands actually work on the regular
# game port, 31457/tcp. This script verifies both facts: the plain-text
# commands work on 31457, and 31456 is confirmed closed.
#
set -uo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../.." >/dev/null 2>&1 && pwd)"
BINARY="${REPO_ROOT}/bin/tetrix-modern.linux"

TESTDIR="$(mktemp -d)"
FAILURES=0

pass() { echo "  [PASS] $1"; }
fail() { echo "  [FAIL] $1"; FAILURES=$((FAILURES+1)); }

cleanup() {
  pkill -9 -f "${TESTDIR}/tetrix-modern.linux" >/dev/null 2>&1
  rm -rf "${TESTDIR}"
}
trap cleanup EXIT

if [[ ! -x "${BINARY}" ]]; then
  echo "Binary not found at ${BINARY} -- build it first:"
  echo "  cd src && sed -i -e 's/\\r\$//' compile.linux && bash compile.linux"
  exit 1
fi

cp "${BINARY}" "${TESTDIR}/tetrix-modern.linux"
cd "${TESTDIR}"
./tetrix-modern.linux
sleep 1

query() {
  # query <command> -> prints the raw response (empty on error/timeout)
  python3 - "$1" <<'PYEOF'
import socket, sys
cmd = sys.argv[1]
try:
    s = socket.create_connection(("127.0.0.1", 31457), timeout=3)
    s.settimeout(3)
    s.sendall((cmd + "\n").encode())
    data = s.recv(4096)
    sys.stdout.write(data.decode(errors="replace"))
    s.close()
except Exception as e:
    sys.stdout.write(f"ERROR: {e}")
PYEOF
}

echo "== playerquery =="
resp="$(query playerquery)"
echo "  response: ${resp}"
if echo "${resp}" | grep -q "Number of players logged in: 0"; then
  pass "playerquery reports 0 players logged in (expected, nobody connected)"
else
  fail "unexpected playerquery response: ${resp}"
fi

echo
echo "== version =="
resp="$(query version)"
echo "  response: ${resp}"
if echo "${resp}" | grep -q "+OK"; then
  pass "version query returned +OK"
else
  fail "unexpected version response: ${resp}"
fi

echo
echo "== listchan =="
resp="$(query listchan)"
echo "  response: ${resp}"
if echo "${resp}" | grep -q "tetrinet" && echo "${resp}" | grep -q "+OK"; then
  pass "listchan shows the default 'tetrinet' channel and returns +OK"
else
  fail "unexpected listchan response: ${resp}"
fi

echo
echo "== listuser (no one connected -- should just be +OK) =="
resp="$(query listuser)"
echo "  response: ${resp}"
if echo "${resp}" | grep -q "+OK"; then
  pass "listuser returned +OK"
else
  fail "unexpected listuser response: ${resp}"
fi

echo
echo "== getwinlist (empty winlist -- should just be +OK) =="
resp="$(query getwinlist)"
echo "  response: ${resp}"
if echo "${resp}" | grep -q "+OK"; then
  pass "getwinlist returned +OK"
else
  fail "unexpected getwinlist response: ${resp}"
fi

echo
echo "== confirming port 31456 (QUERY_PORT) is NOT listening =="
closed="$(python3 - <<'PYEOF'
import socket
try:
    s = socket.create_connection(("127.0.0.1", 31456), timeout=2)
    s.close()
    print("OPEN")
except Exception:
    print("CLOSED")
PYEOF
)"
if [[ "${closed}" == "CLOSED" ]]; then
  pass "port 31456 is closed, as expected (init_query_port() is commented out in main())"
else
  fail "port 31456 unexpectedly accepted a connection -- has init_query_port() been re-enabled? Update this test and the docs if so."
fi

pkill -f tetrix-modern.linux >/dev/null 2>&1
sleep 1

echo
if [[ "${FAILURES}" -eq 0 ]]; then
  echo "All query tests passed."
  exit 0
else
  echo "${FAILURES} check(s) FAILED. See [FAIL] lines above."
  exit 1
fi
