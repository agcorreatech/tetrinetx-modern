# Changelog

Maintainer / Developer: Alexandro G. Corrêa <alex.linux@gmail.com>

## Release 04 - 09/Jul/2026

### Admin authentication (multi-admin, nickname + password)

- `game.secure` moved from a single, shared `op_password` to a proper
  multi-admin format: one `[nickname]` block per admin, each with its own
  `password=` line (mirrors the same `[SECTION]` block style already used
  for preset channels in `game.conf`).
- `/op <password>` and `/admin <password>` (new alias, identical
  behaviour) now use the **already-connected player's own nickname** as
  the implicit username: the entered password is checked against the
  admin account whose `[nickname]` matches `n->nick`. Since nicknames are
  already unique server-wide and can't be changed mid-session, this adds
  a real extra layer of protection (both the correct nickname AND the
  correct password are required), not just a cosmetic change -- with no
  change to the command's calling syntax.
- Backward compatible: a `game.secure` still using the old, bare
  `op_password=X` line (outside any block) is migrated automatically on
  first read into an admin account named `admin`, logged clearly, and the
  file is rewritten in the new format.
- New: `check_admin_login()`, `find_or_add_admin_slot()` (`src/game.c`).

### Ban system: `/ban`, `/unban`, `/banlist`

- `/ban <playernumber> [reason]` now bans **both** the target's IP and
  nickname (two separate entries), so switching networks/VPNs doesn't let
  a banned player back in under the same nickname. The banned player is
  immediately disconnected (unlike `/kick`, see below).
- `/unban ip <ip>` / `/unban nick <nickname>` removes the matching entry
  explicitly by type, so undoing an IP ban never accidentally lifts the
  matching nickname ban (or vice versa).
- `/banlist` (admin-only, `command_banlist`) lists every active ban: type,
  target, date/time applied, admin responsible, and reason.
- `game.ban` moved from a flat list of wildcarded IPs to the same
  `[BAN]`-block format as `game.secure`'s admin blocks (`type`, `target`,
  `date`, `admin`, `reason`), loaded into memory once (`banlist[]`)
  instead of being re-parsed from disk on every single connection
  attempt. Backward compatible: any legacy bare wildcarded-IP lines
  outside a `[BAN]` block are migrated automatically into the new format
  on first read.
- IP bans are checked immediately after `accept()` in `net_telnet()`
  (earliest possible rejection); nickname bans are checked in
  `net_connected()`, right after the client's `tetrisstart` INIT string is
  parsed (the earliest point the server can know which nickname is being
  requested).
- New: `is_ip_banned()`, `is_nick_banned()`, `add_ban()`, `remove_ban()`,
  `readbanlist()`, `writebanlist()`, `init_banlist()`,
  `ip_matches_pattern()` (`src/game.c`).
- New `game.conf` tag: `command_ban` (default: 3, authenticated admins
  only) and `command_banlist` (default: 3).

### `/whois <nickname>`

- New command showing detailed information about one specific player
  (searched across all channels, same scope `/who` already uses): team,
  channel, gameslot, status (playing/lost/not playing), current level
  (while playing), TetriNET client version, and host/IP (admin-only,
  same visibility rule `/who` already applies).
- New `game.conf` tag: `command_whois` (default: 1, anyone).

### `/kick` no longer disconnects -- redirects to the server lobby

- **Behaviour change:** a kicked player is no longer disconnected.
  Instead, they're moved to the server's "lobby" room
  (`game.main_channel_name`, default `Lobby`), using `Lobby1`, `Lobby2`,
  ... automatically if the base room is full **or** if that's the very
  room being kicked from (no special-casing needed -- the lobby search
  simply excludes the room being left).
- The kicked nickname is blocked from rejoining that **specific** room
  for 5 minutes (`KICK_COOLDOWN_SECS`). The block is keyed by nickname
  (not by connection), so it survives a disconnect/reconnect during the
  cooldown window. Checked in `/join`'s channel-resolution step.
- `/kick` itself stays at its original permission level (chanop / "OP by
  position", `command_kick=2`) -- unlike the admin-only commands below,
  this one is left as a lightweight, position-based moderation tool for
  running the game smoothly. An authenticated admin (`/op`/`/admin`) can
  **always** use `/kick` regardless of this setting though, including
  when `command_kick=0` (disabled for everyone else) -- `can_use_kick()`
  encodes this bonus rule for both the command handler and `/help`, the
  same pattern already used for `/set`'s own bonus rule (see below).
- New: `create_channel()`, `find_or_create_lobby_channel()`,
  `move_player_to_channel()`, `add_kick_cooldown()`,
  `is_kick_cooldown_active()`, `can_use_kick()` (`src/main.c`).
  `move_player_to_channel()` is a new, self-contained function (not a
  refactor of `/join`'s existing handler, to avoid any risk of
  regressing that already-working protocol code path) that replays the
  same playerleave/playerjoin/field-resync sequence `/join` already
  sends when a player switches channels.

### Extended (victory-only) winlist metrics + plain-text CSV export

- The original in-game winlist is **unchanged**: same `struct winlist_t`,
  same `game.winlist` binary file, same `/winlist` command, same data
  shown to the TetriNET client.
- New, entirely separate tracking (`struct winliststats_t`,
  `game.winliststats`): total wins, last win date/time, best level reached
  in a winning game, and average level reached across wins -- recorded
  alongside every existing `updatewinlist()` call, never instead of it.
  Deliberately victory-only for now (a true "games played" count,
  including losses, would need a new recording point at the moment a
  player loses, not just when a winner is declared -- left out of scope).
- Bugfix needed to make "level reached" meaningful: the `lvl` protocol
  command was only ever relaying the level number to other players,
  never actually storing it -- `n->level` stayed permanently at its
  initial value. Now stored on receipt.
- `writewinlist()` now also calls `writewinlisttxt()` automatically,
  which exports the winlist (+ extended metrics where available) to a
  plain-text **CSV** file (`game.winlist.csv`): `rank,type,name,score,
  wins,last_win,best_level,avg_level`. Gated by the new `game.conf` tag
  `winlist_export_txt` (default: 1). Names are stripped of TetriNET
  colour-code control characters first (`strip_colour_codes()`, extracted
  from logic `sendwinlist()` already used, and reused by both).
- New: `init_winliststats()`, `readwinliststats()`, `writewinliststats()`,
  `updatewinliststats()`, `find_winliststats()`, `writewinlisttxt()`,
  `strip_colour_codes()` (`src/game.c`).

### `/help` rewritten: shows only what you can currently use

- Replaced ~20 near-identical, repetitive command blocks with a single
  data-driven table (`help_table[]`) and one loop.
- **Behaviour change:** `/help` now shows only the commands the
  requesting player can actually use *right now*, based on their current
  permission level -- rather than listing every enabled command for
  everyone with a "requires OP/`/op`" marker regardless of whether the
  viewer qualifies.
- Admin-only commands (`/ban`, `/unban`, `/banlist`, and the five
  reclassified commands below) are grouped under a separate
  `--- Admin Commands ---` header, which itself is only ever shown once
  at least one such command currently qualifies -- so a non-admin never
  sees an empty admin section.
- `/set` needed a dedicated `can_use_set()` check: it has a bonus
  permission rule on top of the normal level check (`command_set==4`
  additionally allows chanops on non-persistent channels), and a naive
  `passed_level(n, game.command_set)` would have hidden `/set` from
  *everyone*, since level 4 is not a normally-reachable security level.

### Command permission changes

- `/priority`, `/persistant`, `/save`, `/reset`, `/clear` are now
  authenticated-admin-only by default (`command_priority` raised from 2
  to 3; the other four were already 3). `/kick` intentionally stays at
  its original chanop-level default (2) -- see above.
- **Operational note:** default changes only affect fresh installs (no
  existing `game.conf`, or the specific tag missing from one). Existing
  deployments with `command_priority=2` (or similar) already written to
  their `game.conf` keep that value until edited manually.

### Documentation

- `OLD.HISTORY` and `OLD.WISHLIST` moved to `contrib/` (`git mv`, history
  preserved) -- kept purely as historical reference (the original
  1998-1999 build-by-build changelog and feature wishlist from the first
  tetrinetx author), no longer reflecting the current state of the
  project. Documented in `contrib/README`.
- This changelog is now written in English going forward, uses
  `Release NN - DD/Mon/YYYY` headings (matching Releases 01-03 below)
  instead of `Unreleased`, and credits the current maintainer.

### Additional bugfixes found during review (unrelated to the above)

- `src/main.c`, `/join` handler: `if ((nsock=n) && (nsock->type==
  NET_CONNECTED))` was an assignment where a comparison was clearly
  intended. It always evaluated true and, worse, overwrote the loop's own
  iterator variable, corrupting the traversal of `ochan->net` in the
  block that ends the old channel's game when a player leaves. No
  comparison against `n` was actually needed there (`n` had already been
  removed from that list by `remnet()` earlier in the same handler), so
  simplified to just `nsock->type == NET_CONNECTED`.
- `src/main.c`, `net_connected()`: the reserved-nickname (`"server"`)
  check called `killsock()`+`lostnet()` but fell through without a
  `return;`, continuing into every subsequent check (nickname ban,
  version check, duplicate nickname, channel assignment...) on a
  connection that had already been told "not allowed" and had its
  socket killed. Every other equivalent check in this function already
  returns immediately after `killsock()`+`lostnet()`; this one didn't.

### `contrib/tests/` -- end-to-end test documentation and automation

- `contrib/tests/README.md`: overview, explains why the TetriNET encrypted INIT
  handshake makes full protocol-level scripting out of scope for this
  pass, quick-start, coverage table.
- `contrib/tests/config-migration/run.sh`: automated -- fresh-install `game.conf`
  defaults (every new tag above), legacy `game.secure`/`game.ban`
  migration, and a live IP ban actually rejecting a real TCP connection.
- `contrib/tests/query-port/run.sh`: automated -- the plain-text query commands
  (`playerquery`/`version`/`listchan`/`listuser`/`getwinlist`). Also
  documents a finding: these do **not** need the separate query port
  (31456) -- that port isn't listening at all currently
  (`init_query_port()` is commented out in `main()`); the commands
  actually work on the regular game port, 31457.
- `contrib/tests/gameplay/MANUAL-TEST-PLAN.md`: step-by-step checklist for
  everything needing a real authenticated game session (every feature
  above, plus `/topic`/`/list`/`/join`/`/msg`/`/move`/`/set`, starting a
  game and winning, and the single-player endgame fix).

### Fixed: Docker files broken by CRLF on Windows checkouts

- The repository had no `.gitattributes`, so on a Windows machine with
  Git's default `core.autocrlf=true`, every text file -- including
  `contrib/docker/entrypoint.sh` -- got checked out with CRLF line
  endings. `docker build`/`docker compose up` then copied that CRLF
  version straight into the Linux image, turning the shebang
  `#!/usr/bin/env bash` into `#!/usr/bin/env bash\r` (not a valid
  interpreter), so the container failed to start.
- Added `.gitattributes`: forces LF for everything by default (so this
  can't recur for any current or future text file), with two explicit
  exceptions preserving the project's actual, deliberate conventions:
  `src/*.c`/`src/*.h` and `contrib/query/src/*` keep their original CRLF
  (`-text`, i.e. no normalization at all -- `eol=crlf` alone would
  *not* have been enough here, since the general `text=auto` rule would
  still normalize the stored blob to LF regardless; confirmed this the
  hard way while writing the fix), and `bin/tetrix-modern.linux` /
  `tetrinet_windows_client_v1.13/*.zip` are marked `binary`.
- Ran `git add --renormalize .` to apply the new rules to already-
  tracked files. Besides the Docker files, this also normalized five
  other plain files that had CRLF for no particular reason
  (`.gitignore`, `README`, `contrib/README`, `contrib/OLD.HISTORY`,
  `contrib/OLD.WISHLIST`) to LF; the deliberately-CRLF source files
  above were correctly left untouched.
- `contrib/docker/Dockerfile`'s runtime stage now also strips any `\r`
  from `entrypoint.sh` defensively (`sed -i -e 's/\r$//'`) right after
  copying it in, as a second layer of protection against this same
  class of problem (e.g. an existing Windows checkout made before this
  `.gitattributes` existed, or a future edit made with a CRLF-inserting
  editor).
- Verified: full clean compile (source files' CRLF confirmed
  unaffected); manually reproduced the failure mode (a CRLF copy of
  `entrypoint.sh`) and confirmed the Dockerfile's new `sed` step fixes
  it (`bash -n` passes after normalization, failed before).

### Eliminated remaining `-Wstringop-truncation`/`-Wformat-truncation` warnings

- Five bounded-copy call sites (two new admin-password copies in
  `securityread()`, three new/changed ban-field copies in
  `readbanlist()`, plus one pre-existing one in `net_connected()`'s
  channel-description handling, unrelated to this release but visible
  in the same build output) triggered GCC's truncation warnings --
  first as `-Wstringop-truncation` on the original `strncpy()` +
  manual-null-terminator pattern, then as `-Wformat-truncation` after
  switching those specific sites to `snprintf()`. Both warnings are
  GCC's static heuristic failing to prove an intentional, safe
  truncation is actually safe -- not a real bug in either case (the
  destination sizes were always correct), but noisy in CI/Docker build
  logs.
- Added `safe_strcpy(dest, destsize, src)` (`src/utils.c`/`src/utils.h`):
  computes the copy length at runtime (`strlen()` + a comparison) and
  uses `memcpy()`, rather than a literal `strncpy()`/`snprintf()` call
  GCC can statically analyze against the source's declared buffer size.
  This is the standard way to silence both of these particular GCC
  checks without a blanket `-Wno-...` flag or per-line pragmas, since
  neither warning is deep enough to trace a runtime-computed length
  back to a proven-safe bound. Used at all five sites above.
- Verified: full clean compile, zero warnings. Re-ran
  `contrib/tests/config-migration/run.sh` (which exercises the admin
  password and ban admin/reason/target fields this touches) --
  all assertions still pass.

### Docker build fix: missing libc headers (`signal.h` and friends)

- `contrib/docker/Dockerfile`'s build stage installed only the `gcc`
  package (with `--no-install-recommends`), which on Ubuntu only
  *recommends* `libc6-dev` rather than depending on it -- so the C
  library headers (`signal.h`, etc, all of `src/main.h`'s includes)
  were missing, and the build failed with `fatal error: signal.h: No
  such file or directory`. Switched to installing `build-essential`
  instead, which properly `Depends:` on `libc6-dev` (confirmed via
  `apt-cache depends` on both packages) alongside `gcc`, `g++`, `make`,
  and `dpkg-dev`.

### `docker/` and `tests/` moved into `contrib/`

- Both folders moved to `contrib/docker/` and `contrib/tests/`
  respectively (`git mv`, history preserved), grouping all
  deployment/tooling extras under `contrib/` alongside `contrib/systemd/`.
  All internal cross-references (`docker-compose.yml`'s build `context`
  and `dockerfile` path, the `Dockerfile`'s own `COPY` of
  `entrypoint.sh`, both `run.sh` scripts' repo-root detection, and every
  command example in the READMEs and this changelog) were updated for
  the new depth. `.dockerignore` was narrowed from excluding all of
  `contrib/` to excluding only the specific subfolders not needed by the
  image (`contrib/systemd/`, `contrib/tests/`, `contrib/query/`,
  `contrib/OLD.HISTORY`, `contrib/OLD.WISHLIST`, `contrib/README`),
  since the Dockerfile and entrypoint script themselves now live inside
  `contrib/docker/` and need to remain part of the build context.
  Documented in `contrib/README`.
- `bin/tetrix-modern.linux` updated to a freshly-built binary reflecting
  every change in this release (including the two bugfixes above).

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
