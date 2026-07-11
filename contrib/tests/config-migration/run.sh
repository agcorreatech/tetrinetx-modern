#!/usr/bin/env bash
#
# config-migration/run.sh
#
# End-to-end test of:
#   1. Fresh startup: game.conf/game.secure created with correct defaults
#      (including every new tag introduced in Release 04).
#   2. Legacy game.secure (bare "op_password=X", no [nickname] block) is
#      migrated automatically into an admin account named "admin".
#   3. Legacy game.ban (flat, one wildcarded IP per line) is migrated
#      automatically into the new [BAN]-block format.
#   4. A live IP ban actually rejects a real TCP connection attempt.
#
# Runs the server in its own temporary directory -- never touches the
# repository's own bin/ config files. Exits non-zero (and prints which
# check failed) on any assertion failure, so it's CI-friendly.
#
# NOTE: admin passwords (like the original single op_password before it)
# are capped at PASSLEN-1 = 11 characters (src/main.h: #define PASSLEN 12)
# -- longer values are silently truncated on read. This is a pre-existing
# constraint, not something introduced by the multi-admin feature; test
# passwords below are kept within that limit on purpose.
#
set -uo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." >/dev/null 2>&1 && pwd)"
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

# ---------------------------------------------------------------------------
echo "== Test 1: fresh startup creates game.conf with correct new defaults =="
./tetrix-modern.linux
sleep 1

check_conf_tag() {
  local tag="$1" expected="$2"
  local actual
  actual="$(grep -E "^${tag}=" game.conf | cut -d= -f2)"
  if [[ "${actual}" == "${expected}" ]]; then
    pass "${tag}=${expected}"
  else
    fail "${tag}: expected '${expected}', got '${actual}'"
  fi
}

check_conf_tag "command_kick" "2"        # intentionally unchanged (chanop-level)
check_conf_tag "command_priority" "3"    # moved to admin-only
check_conf_tag "command_ban" "3"
check_conf_tag "command_banlist" "3"
check_conf_tag "command_whois" "1"
check_conf_tag "winlist_export_txt" "1"
check_conf_tag "main_channel_name" "lobby"

if [[ -f game.secure ]]; then
  pass "game.secure created on fresh startup"
else
  fail "game.secure was not created"
fi

pkill -f tetrix-modern.linux >/dev/null 2>&1
sleep 1
rm -f game.conf game.secure game.ban game.log game.pid game.motd game.winlist game.winlist.csv game.winliststats

# ---------------------------------------------------------------------------
echo
echo "== Test 2: legacy game.secure (op_password=) migrates to [admin] block =="
cat > game.secure <<'EOF'
# legacy format
op_password=legacypass
EOF

./tetrix-modern.linux
sleep 1

if grep -q '^\[admin\]' game.secure && grep -q '^password=legacypass' game.secure; then
  pass "legacy op_password migrated into [admin] block with password preserved"
else
  fail "migration did not produce the expected [admin] block -- got:"
  sed 's/^/         /' game.secure
fi

if grep -qi "Migrated legacy 'op_password'" game.log; then
  pass "migration was logged"
else
  fail "no migration log message found"
fi

pkill -f tetrix-modern.linux >/dev/null 2>&1
sleep 1
rm -f game.log game.pid

# ---------------------------------------------------------------------------
echo
echo "== Test 3: legacy game.ban (flat wildcarded IP list) migrates to [BAN] blocks =="
cat > game.ban <<'EOF'
# legacy format
192.168.1.100
10.0.0.*
EOF

./tetrix-modern.linux
sleep 1

ban_blocks="$(grep -c '^\[BAN\]' game.ban)"
if [[ "${ban_blocks}" == "2" ]]; then
  pass "both legacy ban lines migrated into [BAN] blocks (found ${ban_blocks})"
else
  fail "expected 2 [BAN] blocks after migration, found ${ban_blocks} -- got:"
  sed 's/^/         /' game.ban
fi

if grep -q 'target=192.168.1.100' game.ban && grep -q 'target=10.0.0.\*' game.ban; then
  pass "both original IP patterns preserved (192.168.1.100 and 10.0.0.*)"
else
  fail "one or both original IP patterns missing after migration"
fi

pkill -f tetrix-modern.linux >/dev/null 2>&1
sleep 1
rm -f game.log game.pid

# ---------------------------------------------------------------------------
echo
echo "== Test 4: a live IP ban actually rejects a real TCP connection =="
cat > game.ban <<'EOF'
[BAN]
type=ip
target=127.0.0.1
date=1783646403
admin=test
reason=Automated test ban
EOF

./tetrix-modern.linux
sleep 1

response="$(python3 - <<'PYEOF'
import socket
try:
    s = socket.create_connection(("127.0.0.1", 31457), timeout=3)
    s.settimeout(3)
    data = s.recv(4096)
    print(data.decode(errors="replace"))
    s.close()
except Exception as e:
    print(f"ERROR: {e}")
PYEOF
)"

if echo "${response}" | grep -qi "banned"; then
  pass "banned IP was rejected with a 'banned' message: ${response}"
else
  fail "expected a 'banned' rejection message, got: ${response}"
fi

pkill -f tetrix-modern.linux >/dev/null 2>&1
sleep 1

# ---------------------------------------------------------------------------
echo
if [[ "${FAILURES}" -eq 0 ]]; then
  echo "All config-migration tests passed."
  exit 0
else
  echo "${FAILURES} check(s) FAILED. See [FAIL] lines above."
  exit 1
fi
