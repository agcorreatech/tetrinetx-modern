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
  running the game smoothly.
- New: `create_channel()`, `find_or_create_lobby_channel()`,
  `move_player_to_channel()`, `add_kick_cooldown()`,
  `is_kick_cooldown_active()` (`src/main.c`). `move_player_to_channel()`
  is a new, self-contained function (not a refactor of `/join`'s existing
  handler, to avoid any risk of regressing that already-working protocol
  code path) that replays the same playerleave/playerjoin/field-resync
  sequence `/join` already sends when a player switches channels.

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

### `tests/` -- end-to-end test documentation and automation

- `tests/README.md`: overview, explains why the TetriNET encrypted INIT
  handshake makes full protocol-level scripting out of scope for this
  pass, quick-start, coverage table.
- `tests/config-migration/run.sh`: automated -- fresh-install `game.conf`
  defaults (every new tag above), legacy `game.secure`/`game.ban`
  migration, and a live IP ban actually rejecting a real TCP connection.
- `tests/query-port/run.sh`: automated -- the plain-text query commands
  (`playerquery`/`version`/`listchan`/`listuser`/`getwinlist`). Also
  documents a finding: these do **not** need the separate query port
  (31456) -- that port isn't listening at all currently
  (`init_query_port()` is commented out in `main()`); the commands
  actually work on the regular game port, 31457.
- `tests/gameplay/MANUAL-TEST-PLAN.md`: step-by-step checklist for
  everything needing a real authenticated game session (every feature
  above, plus `/topic`/`/list`/`/join`/`/msg`/`/move`/`/set`, starting a
  game and winning, and the single-player endgame fix).

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
