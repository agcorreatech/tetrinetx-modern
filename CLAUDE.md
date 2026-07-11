# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

TetriNET X Modern is a modernized fork of `tetrinetx` (the classic GNU
TetriNET Server, based on version 1.13.16, minus the IRC/`qirc` and
spectator features) — a 6-player Tetris-battle server speaking the
TetriNET 1.13 protocol. It's a single-process, single-threaded C daemon:
no external dependencies, no build system beyond one `gcc` invocation.

## Build

```bash
cd src
sed -i -e 's/\r$//' compile.linux   # strip CRLF before running (see "Line endings" below)
./compile.linux                      # gcc -O2 -fno-strength-reduce -Wall main.c -o ../bin/tetrix-modern.linux
```

Produces `bin/tetrix-modern.linux`. There is no Makefile — `main.c` is a
**unity build**: it directly `#include`s `utils.c`, `net.c`, `crack.c`,
and `game.c` (see the top of `src/main.c`), so `gcc` only ever compiles
that one file. When editing, keep this in mind — e.g. static-like helpers
in `game.c`/`net.c`/`crack.c` are effectively all one translation unit
with `main.c`.

The query tool (`contrib/query/`) is a separate, standalone helper with
its own `compile.linux` in `contrib/query/src/`.

## Running

```bash
./bin/tetrix-modern.linux
```

The binary daemonizes itself (double `fork()`, closes stdio, `setsid()`)
and writes its own PID to `game.pid` in the current working directory —
run it from the directory where `game.conf`/`game.motd`/etc. should live
(everything is read/written relative to CWD, not to the binary's
location). Ports (`src/main.h`): `31457/tcp` game/telnet port,
`31456/tcp` query port (currently **not** listening — `init_query_port()`
is commented out in `main()`; the plain-text query commands like
`playerquery`/`version`/`listchan` actually answer on `31457`, see
`net_telnet_init()`).

For process supervision beyond raw `fork()`+PID file, see
`contrib/systemd/README.md` (systemd unit, `Type=forking`) and
`contrib/docker/README.md` (Docker needs an `entrypoint.sh` shim because
the binary's own daemonizing conflicts with the "PID 1 stays foreground"
container model — read that README before touching container-related
files, it explains several non-obvious workarounds: the entrypoint
polling loop, `--init`/`tini` for reaping the orphaned double-fork
grandchild, and the `/opt/tetrinetx/defaults` → `/data` copy-on-first-run
pattern).

## Tests

```bash
# Build first (from repo root)
cd src && sed -i -e 's/\r$//' compile.linux && bash compile.linux && cd ..

# Scriptable (each spins up its own throwaway server in a tmp dir and cleans up after itself)
bash contrib/tests/config-migration/run.sh
bash contrib/tests/query-port/run.sh
```

There is no automated test for the actual game/partyline protocol: the
TetriNET INIT handshake (`tetrisstart <nick> <version>`) is sent through
a self-adapting encrypted cipher (10 interchangeable transforms,
auto-detected — see `src/crack.c`), and writing a client-side encoder
just for test scripting was judged out of scope. Anything that needs an
authenticated game session (`/kick`, `/ban`, `/op`, gameplay itself, etc.)
is instead a manual checklist: `contrib/tests/gameplay/MANUAL-TEST-PLAN.md`.
See `contrib/tests/README.md` for the full feature-to-test-location table
before assuming something is (or isn't) covered.

A real TetriNET 1.13 client is bundled at
`tetrinet_windows_client_v1.13/` (run under Wine on Linux/macOS, or
natively on Windows) for manual testing.

## Line endings — read before editing `src/`

`src/*.c`, `src/*.h`, and `contrib/query/src/*.c` are committed with
**CRLF line endings on purpose** (historical from the original codebase)
and are marked `-text` in `.gitattributes` — Git will not normalize them
on checkout or commit, on any platform. Every build step already strips
the `\r` at compile time via `sed` (see the Build section and
`contrib/docker/Dockerfile`), so this is transparent to compiling/running
— but if you edit these files with a tool that rewrites line endings, you
can silently reintroduce a mix of CRLF/LF in one file. Everything else in
the repo (scripts, docs, `contrib/docker/entrypoint.sh`, etc.) is forced
to LF (`* text=auto eol=lf`) — a CRLF shebang there breaks the container
build outright.

## Architecture

Everything lives in `src/`, four files sharing one address space via the
unity build:

- **`main.c`** (~3600 lines) — everything network/protocol/command
  related. `main()` runs a single-threaded event loop: `check_timeouts()`
  → `dequeue_sockets()` → `sockgets()` (blocking multiplex read across all
  sockets, ~1s granularity) → `net_activity()` dispatches by connection
  state (see the `NET_*` state machine below). The two biggest functions
  are `net_telnet_init()` (the INIT/handshake state — also handles the
  plain-text query commands `playerquery`/`version`/`listchan`/
  `listuser`/`getwinlist`) and `net_connected()` (~1700 lines — every
  partyline slash-command: `/kick`, `/ban`, `/op`, `/admin`, `/whois`,
  `/who`, `/topic`, `/set`, `/priority`, `/save`, `/reset`, `/clear`,
  etc., dispatched via `strncasecmp(MSG, "/cmd", n)` chains, each gated
  by a `game.conf` `command_*` permission flag and/or `passed_level()`).
- **`game.c`** — everything file-backed: reading/writing `game.conf`,
  `game.motd`, the winlist (`game.winlist` + extended stats in
  `game.winliststats` + CSV export `game.winlist.csv`), the admin account
  file `game.secure`, the ban list `game.ban`, and in-memory-only kick
  cooldowns. Each subsystem follows the same shape: an `init_*()` (set
  defaults), `read*()`/`write*()` pair, and fixed-size in-memory arrays
  declared in `main.h` (`security.adminlist[MAXADMINS]`,
  `banlist[MAXBANS]`, `winlist[MAXWINLIST]`, etc. — no dynamic growth, no
  malloc'd lists for these).
- **`net.c`/`net.h`** — low-level socket layer, adapted from eggdrop
  (credited in `net.h`); dynamic (not static-array) socket list,
  `sockgets()`/`tputs()`/`dequeue_sockets()` buffering.
- **`crack.c`/`crack.h`** — the client-independent decryption of the
  encrypted `tetrisstart` INIT string (auto-detects which of 10
  encoding transforms the client used, from the known plaintext prefix).

Key structures, all in `main.h`: `net_t` (one per connection — socket,
nick, team, state, playing field, security level), `channel_t` (a
room/game, with its own full ruleset — block/special weights, sudden
death settings — copied from `game_t` defaults at creation), `game_t`
(the global config loaded from `game.conf`), plus the security/ban/
winlist-stats/kick-cooldown structs added on top of the original
tetrinetx base (see `CHANGELOG.md` for the history/rationale of each).

Connection state machine (`NET_*` in `main.h`): `NET_TELNET` →
`NET_TELNET_INIT` (receiving encrypted INIT) → `NET_WAITINGFORTEAM` →
`NET_CONNECTED` (in a channel, partyline + gameplay active). `NET_QUERY`/
`NET_QUERY_INIT` exist for the separate query port but are currently
unused since that port isn't opened.

Security levels (`LEVEL_*`): `LEVEL_NORMAL` (unauthenticated) <
`LEVEL_OP` (channel-position op) < `LEVEL_AUTHOP` (`/op`/`/admin`
authenticated). `/op` and `/admin` are identical and use the
**already-connected player's own nickname** as the implicit login
username against `game.secure`'s `[nickname]` blocks — see
`check_admin_login()` in `game.c` and its callers in `main.c`.

`game.conf`/`game.secure`/`game.ban` all use the same `[SECTION]`-block
text format, and each has backward-compatible migration from older flat
formats (single `op_password=`, bare wildcarded IP list) applied
automatically on first read, with the file rewritten in the new format —
see `CHANGELOG.md` Release 04 for the details and rationale if extending
either file's format further.

`CHANGELOG.md` is actively maintained per release and is the best source
for *why* a given piece of behavior exists (most non-obvious design
decisions — e.g. why kick-cooldowns are memory-only, why bans check IP at
accept() time but nick at INIT-parse time — are explained there rather
than in code comments).
