# Changelog

Maintainer / Developer: Alexandro G. Corrêa <alex.linux@gmail.com>

## Release 05 - 11/Jul/2026

### Fix: pause/unpause locked up the game after one cycle

- The `pause` partyline handler (`src/main.c`, `net_connected()`) had its
  channel-status transition inverted and gated the command on
  `STATE_INGAME` only. Pausing (`pause 1`) left the channel `INGAME`
  while unpausing (`pause 0`) set it to `STATE_PAUSED`, so a game could
  be paused and unpaused exactly once; after that the channel was stuck
  in `STATE_PAUSED` while actually running, and every subsequent
  pause/unpause **and** "stop game" -- all gated on the channel status --
  was silently ignored until the game ended on its own. (It also left the
  sudden-death timer ticking during a pause, since the channel was still
  `INGAME`.) Now `pause 1` is accepted only while `INGAME` and sets
  `PAUSED`; `pause 0` is accepted only while `PAUSED` and sets `INGAME`.
- Relatedly, "stop game" (`startgame 0`) is now accepted while the
  channel is `STATE_PAUSED` too, so an op can stop a paused game directly
  instead of having to unpause it first.

### Code-review round: crashes, memory safety and small logic fixes

A pass over `src/` fixing a set of latent defects found by review. Each
was validated by a clean `-Wall` build plus the two scriptable test
suites (`config-migration`, `query-port`) and a full Docker run.

Memory-safety / crashes:

- **Use-after-free in `net_telnet_init()`** (`src/main.c`): the "too few
  conversions" bail-out (a malformed `tetrisstart` INIT string) was
  missing its `return`, so after `lostnet()` had already `free()`d the
  connection it fell through into every check below (`"server"` nickname,
  nick ban, version, duplicate nick, ...), dereferencing freed memory.
  Now returns immediately, like every other bail-out in that function.
- **Memory leak in `net_telnet_init()`** (`src/main.c`): `dec`, allocated
  by `tet_dec2str()`, was never freed on any path -- one small leak per
  connection. Freed once the nickname/version are parsed out of it.
- **Buffer overflow in `lvprintf()` / `lprintf()`** (`src/main.c`) and
  `tprintf()` (`src/net.c`): all three formatted with unbounded
  `vsprintf()` into a fixed static buffer. `lvprintf()` in particular
  logs client-controlled data (e.g. the "Invalid Command" log echoes the
  raw ~1KB line into a 768-byte buffer), overflowing it *before* the
  post-hoc length truncation could run. Switched to `vsnprintf()`.
- **Use-after-free in `got_term()`** (`src/main.c`): the SIGTERM handler
  called `lostnet()` (which frees the node, and possibly the whole
  channel) and then walked `->next` on the freed pointers. On shutdown
  there's nothing to gain from `lostnet()`'s bookkeeping, so it now just
  closes each socket, capturing the `next` pointers first.
- **Out-of-bounds access in `gameread()`** (`src/game.c`): the trailing-
  space strip lacked the `j>=0` guard its sibling readers
  (`securityread()`, `readbanlist()`) already had, so a line that became
  empty after comment/CR stripping read/wrote `buf[-1]`. Guard added.

Logic / correctness:

- **`strclen()` returned the wrong value** (`src/main.c`): documented as
  "length minus colour codes", it accumulated that into `count` but
  returned `i` (the full length). `/who` uses it to pad nick/team columns
  to a fixed *visible* width, so any name carrying colour codes was
  mis-aligned. Now returns `count`.
- **`/motd` checked the wrong permission flag** (`src/main.c`): it was
  gated by `game.command_list` (copy/paste from `/list`) instead of its
  own `game.command_motd`, which was being ignored entirely.
- **`/winlist` printed `unsigned long` scores with `%4d`**
  (`src/main.c`): a type/format mismatch (varargs UB); changed to `%4lu`
  to match the value's actual type.
- **Unresolved-host fallback logged the server's own IP** (`src/main.c`,
  `net_telnet()`): the dotted-quad was built from `n->addr` (the
  listening socket) instead of `net->addr` (the connecting client).
- **`pause`-unpause log format typo** (`src/main.c`): `"#%2-%s"` ->
  `"#%s-%s"`.

Robustness:

- **`read_motd()` no longer takes the whole server down** (`src/main.c`):
  a missing/unreadable `game.motd` (cosmetic, optional) used to call
  `fatal()`, killing every connection and exiting the process. Now it
  logs a warning and skips the MOTD for that client.
- **`while(!feof())` read loops stop on a failed read** (`src/game.c`,
  `gameread()` / `securityread()` / `readbanlist()`): a failed `fscanf`
  used to leave the previous line in `buf` and re-process it (a stray
  trailing `[BAN]` could even allocate a spurious empty ban). They now
  `break`, which also drops a spurious startup error print.
- **`/whois` and `/priority` argument guards** (`src/main.c`): with no
  argument, `MSG+7` / `MSG+10` pointed one byte past the terminator.
  Guarded so a missing argument reads an empty string instead.

### Repo housekeeping

- `CLAUDE.md` (guidance for Claude Code sessions working in this repo)
  added to `.gitignore` -- kept locally, not tracked/shared via git.

### Startup: quieter FQDN-fallback warning

- `getmyhostname()` (`src/net.c`) no longer `printf()`s its "could not
  determine a fully qualified hostname" warning to stdout on every
  startup -- expected on any host without a configured FQDN (very common
  on containers), not an error. Still recorded in `game.log` via
  `lvprintf()` when `verbose>=2`, for troubleshooting.

### Fix: `game.conf` string values corrupted by CRLF line endings

- `gameread()` (`src/game.c`) parsed each line with `"%[^\n]"`, which
  happily includes a trailing `\r` when `game.conf` has CRLF line
  endings. Numeric fields (`atoi()`) were silently immune, but string
  fields copied verbatim -- `pidfile`, `bindip`, `topic` -- were not:
  `pidfile` ended up as the literal filename `"game.pid\r"`, a
  different name from `"game.pid"` as far as the filesystem is
  concerned, so the daemon wrote its PID to a file nothing else could
  ever find by its expected name. This is what caused the Docker
  entrypoint's PID-file wait loop to time out and restart-loop, even
  though the server itself had started up fine. Now strips a trailing
  `\r` from each config line right after reading it.

### Fix: server crash on every client connection (`read_motd()`)

- `read_motd()` (`src/main.c`) looped on `"!feof(file_in)"`, which only
  becomes true *after* a read attempt has already run past the last
  line -- so every call did one extra, guaranteed-to-fail `fscanf()`
  past the real content, and that ordinary end-of-file was (wrongly)
  treated as a fatal I/O error. `fatal()` kills every connected socket
  and `exit()`s the *whole server process*, so this crashed the entire
  server on every client connection that got far enough to be sent the
  MOTD (not just the one being served) -- experienced by the client as
  an abrupt "*** Server has Shut Down" disconnect. Fixed by looping on
  the `fscanf()` result itself.
- Also fixed a stray `\r` in MOTD lines (same CRLF issue as above,
  affecting `game.motd` this time), which was being sent embedded in
  the `pline` packet right before its `\xff` terminator.

### Fix: `/whois` command-prefix collision with `/who`

- `/whois <nick>` also matched `/who`'s `strncasecmp(MSG, "/who", 4)`
  check (independent, unconditional `if`s, not `else if`), since
  `"/whois"` starts with `"/who"` -- every `/whois` dumped the full
  `/who` player table right after the whois info. Same latent bug
  existed between `/ban` and `/banlist`. Fixed both by requiring the
  matched prefix be followed by end-of-string or a space.
- `/whois`'s field labels ("Team:", "Client version:", ...) now use
  fixed-width padding instead of a single `\t` each, so values line up
  in the same column regardless of label length.

### New: colored, boxed ASCII-art MOTD

- `read_motd()` now supports an optional leading `[NAME]` colour tag
  per line in `game.motd` (e.g. `[YELLOW]...`), stripped before
  sending; falls back to the previous plain-blue behaviour for
  tag-less or unrecognised tags, so old motd files keep working
  unchanged.
- Sends a blank `pline 0` before and after the MOTD content, from code
  rather than as an empty line in the file itself (an empty line there
  would silently truncate the rest of the file -- `"%[^\n]"` requires
  at least one character).
- New `bin/game.motd`: a boxed banner with maintainer/repo info and a
  `/help` pointer, encoded as Windows-1252/cp1252 rather than UTF-8 --
  the real TetriNET 1.13 client renders accented characters (e.g.
  "Corrêa") correctly in that encoding but mangles UTF-8's multi-byte
  sequences.

### New: /password command; /help polish for authenticated admins

- New `/password <new-password>` partyline command: an authenticated
  admin changes the password of their OWN account (the one matching
  their current nickname) -- there is deliberately no way to change
  another account's password. Persisted to `game.secure` immediately;
  values over the 11-character cap are truncated with a notice.
- The `--- Admin Commands ---` header in `/help` now gets a blank
  spacer line before it and is rendered bold black, matching the
  Channel Configuration section's style.
- `/help` no longer lists `/op` for an already-authenticated admin
  (nothing left for them to gain from it); it still shows for everyone
  else whenever `command_op` is enabled.

### New: restricted nicknames -- admin-account nicks must authenticate

- Connecting under a nickname that has an admin account in `game.secure`
  now arms a 60-second deadline (`RESTRICTED_NICK_AUTH_SECS` in
  `src/main.h`): if the player hasn't authenticated with `/op` by then,
  they are disconnected with "Restricted nickname. Authentication
  required to stay connected." Nothing is announced on connect, on
  purpose -- an impostor probing admin nicknames learns nothing until
  the disconnect. A successful `/op` disarms the deadline; non-admin
  nicknames are unaffected. Enforced once per second alongside the
  regular socket timeouts in `check_timeouts()`.

### Changed: /help list reorganized

- General section reordered to: `/list`, `/join`, `/who`, `/whois`,
  `/msg`, `/me`, `/winlist`, `/motd` -- and `/motd`'s description is now
  "Displays the server welcome message".
- `/op`'s description shortened to "Gain SERVER ADMIN status" (still the
  last entry of the list).

### Changed: /help header formatting

- The `HELP - Server Commands - ...` header line is now bold black.
- A blank spacer line is sent before `--- Channel Configuration ---`,
  and that section header is now bold black too (same `pline 0 \xff`
  blank-line technique the MOTD uses).

### Changed: Docker image no longer ships config files

- With all defaults now embedded in the binary (entry below), the Docker
  image stopped carrying the `/opt/tetrinetx/defaults/` copies of
  `game.conf`/`game.motd`, and `entrypoint.sh` no longer copies them
  into `/data` on first run -- the binary itself generates `game.conf`,
  `game.motd` and `game.secure` on first boot (and never overwrites
  existing files, so volume edits still persist across restarts and
  rebuilds). `contrib/docker/README.md` updated accordingly.

### Changed: all default files fully embedded in the binary; maxchannels=99

- Starting the binary in an empty directory now regenerates ALL of
  `game.conf`, `game.motd` and `game.secure` matching the shipped
  defaults, with no external files needed:
  - `write_motd()` default was still the old plain 3-line text; it now
    embeds the colored, boxed ASCII-art banner byte-for-byte identical
    to `bin/game.motd` (including the Windows-1252 `ê` -- see the
    Release 05 MOTD entry for why cp1252, not UTF-8).
  - `game.conf` and `game.secure` were already generated (with the
    default channels and the default admin account from the entries
    above).
- Default `maxchannels` raised 10 -> 99 (and `MAXLOBBYVARIANTS` raised
  to match, so lobby overflow variants can actually use the room).
- `bin/game.conf` regenerated from the new binary -- the committed copy
  had drifted (missing `command_whois`/`command_ban`/`command_banlist`/
  `winlist_export_txt`/`main_channel_name`, stale `command_priority=2`,
  no `[lobby]`/`[1x1]` preset blocks). `bin/game.motd` needed no change
  (the embedded banner reproduces it exactly).

### New: default channels #lobby + #1x1, lobby overflow, auto-join order

- When `game.conf` defines no `[channel]` blocks, the server now seeds
  two default rooms at startup: **#lobby** (topic "Server Lobby",
  priority 1 — where players land on connect) and **#1x1** (max **2**
  players, topic "Game 1x1", priority 2). Both are persistent presets;
  on a brand-new install they are also written into the fresh
  `game.conf`. (Previously no channel existed until the first connection
  created a non-persistent `#tetrinet`.)
- **Auto-join priority semantics inverted**: players connecting are now
  placed in the room with the LOWEST non-zero priority that has space
  (1 fills first, then 2, ...); priority 0 still means "never
  auto-join". It used to be highest-value-wins, which would have sent
  everyone to #1x1 before #lobby. `game.conf` comments and the
  `/priority` feedback message document the new ordering.
- **Connect overflow now uses lobby variants**: when every room is full
  (or priority 0), the connection lands in `lobby1`, `lobby2`, ...
  (created on demand, same helper the `/kick` redirect uses) instead of
  the old auto-created `tetrinet2`, `tetrinet3`, ... rooms. "Server is
  Full!" is only sent once `maxchannels` is exhausted.
- `main_channel_name` default renamed `Lobby` -> `lobby` (matching is
  case-insensitive, so existing setups keep working).
- **Crash fix (latent, pre-existing)**: preset channels created by
  `gameread()` from `game.conf` never initialised `chan->net`, leaving a
  garbage player-list pointer — the first connection to touch such a
  channel (`numplayers()` walks that list) could crash the server. Went
  unnoticed because the stock `game.conf` had no channels; the new
  default channels made it crash reliably on the second boot.

### New: game.secure ships a default admin account, with a loud warning

- A fresh `game.secure` now contains a working default account:
  `[admin]` with password `tetrinetx`, so a new install has admin access
  out of the box (`/op tetrinetx` connected under the nickname `admin`).
- The file's header comments were rewritten to document how `/op`
  validation actually works (nickname-implicit username, both must
  match, passwords case-sensitive and capped at 11 chars) and to tell
  the owner to change the default account.
- While the default `[admin]`/`tetrinetx` pair is still present, every
  startup logs a WARNING (also echoed to stdout pre-daemonization)
  recommending changing BOTH the account name and the password.
- If `game.secure` vanishes mid-run, the `/op` handler recreates it the
  same way boot does — default account included.

### Test-suite updates for the new defaults

- `config-migration`: expects `main_channel_name=lobby`.
- `query-port`: `listchan` now asserts both `lobby` and `1x1`; the
  query helper reads until `+OK`/close instead of a single `recv()`
  (multi-channel replies span several TCP segments) and returns partial
  data on timeout (`playerquery` has no `+OK` terminator).
- `MANUAL-TEST-PLAN.md`: updated for `#lobby`/`#1x1`/`#lobby1` and the
  default-admin checks (warning present on fresh install, gone after
  changing the credentials).

### Repo housekeeping: CLAUDE.md actually untracked

- The earlier "CLAUDE.md added to .gitignore" change only added the
  ignore pattern; the file itself was still tracked (gitignore has no
  effect on already-tracked files), so edits kept showing up in git.
  Now removed from the index (`git rm --cached`) — the file stays on
  disk locally, completing the original intent.

### Changed: /help layout, /op-only auth, admin-only /priority, v1.13.26

- Server version bumped to **v1.13.26** (`SERVERBUILD` in `src/main.h`).
  Shown in the `/help` header, the Linux startup banner and boot log,
  and the plain-text `version` query reply. (`TETVERSION` stays `1.13` —
  that's the TetriNET protocol version clients are matched against.)
- `/help` now groups `/move`, `/kick`, `/topic` and `/set help` (in that
  order) under a new `--- Channel Configuration ---` header, using the
  same header formatting as the existing Admin Commands section. The
  `/topic` description was reworded to "Changes the channel description".
- The `--- Admin Commands ---` section of `/help` is now shown ONLY to
  authenticated admins (`LEVEL_AUTHOP`), regardless of how low the
  individual `command_*` levels are set in `game.conf` — a normal player
  never sees the section at all.
- `/priority` is now usable only by authenticated admins, whatever
  `command_priority` says (the config value can still disable it with 0,
  but can no longer open it to lower levels) — new shared
  `can_use_priority()` check used by both the handler and `/help`.
- **Removed the `/admin` alias**; `/op <password>` is now the only
  authentication command (typing `/admin` gets "Invalid /COMMAND!").
  Its `/help` entry is deliberately the LAST row of the list, with the
  description reworded to "Gain AUTHENTICATED SERVER ADMIN status".
  All generated-file comments (`game.conf`, `game.secure`), the legacy
  migration log message, and the test docs no longer mention `/admin`.

### New: beginner's guides (English + Brazilian Portuguese)

- Added `How to start and play.txt` and `Como iniciar e jogar.txt` at
  the repo root: step-by-step beginner documentation covering running
  the server (Docker compose, the prebuilt `bin/tetrix-modern.linux`
  binary directly — needs only a 2021+ x86_64 distro with glibc >= 2.34,
  no packages — or compiling from source as the fallback), running the
  bundled TetriNET 1.13 client (`TETRINET.EXE`, natively or via Wine),
  connecting (address/nickname/team, port 31457), and playing (moderator
  role, default keys, special blocks, winlist scoring, troubleshooting).
  Game-rule and key-binding details were sourced from the original
  `TETRINET.TXT` and `tetrinet.ini` inside the bundled client zip.

### Build: committed binary updated

- `bin/tetrix-modern.linux` rebuilt from the current source so the
  committed binary includes the pause/unpause fix and the code-review
  round above (it had been rebuilt during that session but the updated
  binary was left out of those commits). Verified byte-identical to a
  clean build of the same source via the Docker `build` stage.

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

### Startup (boot) debug logging

- Every initialization step in `main()` is now logged with a `BOOT:`
  prefix **before** it runs (13 numbered steps: config, network buffers,
  port binding, winlist/stats/security/banlist loading, motd, admin
  accounts) -- so if the process dies mid-startup, the last `BOOT:` line
  tells you exactly which step failed. Messages go to **both** stdout
  and `game.log` (everything happens before the daemonizing `fork()`,
  so stdout is still attached), because the two most common startup
  failures each blind exactly one of the two channels: a non-writable
  working directory silently disables `game.log` (its writer ignores
  `fopen()` failures), while a service manager hides stdout.
- New early environment check (`boot_check_environment()`): logs the
  working directory (with a reminder that *all* `game.*` files are
  read/written relative to it) and probes it for writability up front --
  failing immediately with the exact `errno` reason and a how-to-fix
  message, instead of a series of confusing downstream failures in
  `gamewrite()`/`writepid()`/the log itself.
- If `game.log` can't be opened, that is now announced loudly on stdout
  once (previously: silently ignored, leaving an empty/missing log with
  no explanation).
- Port-bind failure (`init_telnet_port()`) now prints the three most
  common causes with ready-to-run diagnostic commands (server already
  running / port in use by another process / bad `bindip` in
  `game.conf`), instead of just "Couldn't find telnet port".
- The post-daemonization "Wrote PID" message was only logged at
  verbosity 9 (default is 4), so the daemon's successful startup never
  actually appeared in the log; now logged at priority 1 as
  `BOOT: Daemon running (pid N) ... Startup complete.` -- the milestone
  the pre-fork messages tell the user to watch for.
- Verified by really exercising all three paths: a clean boot (13 steps
  visible on stdout and in `game.log`, ending with the daemon-running
  milestone), a port-conflict boot (fails at step 4/13 with the full
  diagnosis), and a read-only-directory boot as an unprivileged user
  (immediate fatal with `Permission denied`, correct explanation, and
  the log-unavailable warning on stdout).

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
