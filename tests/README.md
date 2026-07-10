# End-to-end tests

This folder documents (and, where possible, automates) end-to-end testing
of the tetrinetx server's features.

## Why this isn't all one automated test suite

The TetriNET game protocol (port 31457) requires an **encrypted INIT
handshake**: the client's `tetrisstart <nick> <version>` string is sent
through a custom, self-adapting chained cipher (10 interchangeable
transforms, auto-detected from the known plaintext prefix — see
`src/crack.c`). Writing a correct encoder for that cipher, just to script
end-to-end tests, was judged out of proportion for this pass (it's a
non-trivial reverse-engineering exercise in its own right, independent of
testing the actual server logic).

Because of that, this folder is split in two:

- **Scriptable** (`query-port/`, `config-migration/`): anything that
  doesn't require the encrypted handshake — the query port (31456, plain
  text) and the config-file migration logic (`game.secure`, `game.ban`).
  These have runnable `run.sh` scripts.
- **Manual** (`gameplay/MANUAL-TEST-PLAN.md`): anything that requires an
  actual authenticated game session (partyline commands like `/kick`,
  `/ban`, `/whois`, `/op`, `/help`, etc.) — a step-by-step checklist to
  run with a real TetriNET client.

## Quick start

```bash
# Build the server first (from the repository root)
cd src && sed -i -e 's/\r$//' compile.linux && bash compile.linux && cd ..

# Scriptable tests
bash tests/config-migration/run.sh
bash tests/query-port/run.sh

# Manual tests: see tests/gameplay/MANUAL-TEST-PLAN.md
```

Both `run.sh` scripts start their own throwaway copy of the server in a
temporary directory, so they're safe to run without touching your real
`game.conf`/`game.secure`/`game.ban`/etc., and they clean up (kill the
server, remove the temp directory) when done.

## What's covered where

| Feature | Where it's tested |
|---|---|
| Server starts, listens on 31457/31456 | `query-port/run.sh` |
| `game.conf` created with correct defaults (incl. all new tags) | `config-migration/run.sh` |
| `game.secure` legacy `op_password=` migration to `[nickname]` blocks | `config-migration/run.sh` |
| `game.ban` legacy flat IP-list migration to `[BAN]` blocks | `config-migration/run.sh` |
| IP ban rejects a connection at the TCP level | `config-migration/run.sh` |
| `playerquery`, `version`, `listchan`, `listuser`, `getwinlist` (query port) | `query-port/run.sh` |
| Nickname/team join, chat, `/me` | `gameplay/MANUAL-TEST-PLAN.md` |
| `/help` (filtered by current permission level) | `gameplay/MANUAL-TEST-PLAN.md` |
| `/who`, `/whois <nickname>` | `gameplay/MANUAL-TEST-PLAN.md` |
| `/op`, `/admin` (nickname-implicit multi-admin auth) | `gameplay/MANUAL-TEST-PLAN.md` |
| `/ban`, `/unban`, `/banlist` | `gameplay/MANUAL-TEST-PLAN.md` |
| `/kick` (lobby redirect + 5-minute rejoin cooldown) | `gameplay/MANUAL-TEST-PLAN.md` |
| `/priority`, `/persistant`, `/save`, `/reset`, `/clear` (admin-only) | `gameplay/MANUAL-TEST-PLAN.md` |
| `/topic`, `/list`, `/join`, `/msg`, `/move`, `/set` | `gameplay/MANUAL-TEST-PLAN.md` |
| Gameplay: starting a game, winning, single-player endgame fix | `gameplay/MANUAL-TEST-PLAN.md` |
| Winlist broadcast (all players receive it), `/winlist [n]` | `gameplay/MANUAL-TEST-PLAN.md` |
| Extended winlist metrics + `game.winlist.csv` export | `gameplay/MANUAL-TEST-PLAN.md` |

## Getting a TetriNET client for the manual tests

The manual test plan needs a real TetriNET client connected to the
server. The original Windows client is already bundled in this
repository at `tetrinet_windows_client_v1.13/` (run under Wine on
Linux/macOS, or natively on Windows). Any other TetriNET 1.13-compatible
client works too, as the protocol itself hasn't changed.
