# Changelog

Maintainer / Developer: Alexandro G. Corrêa <alex.linux@gmail.com>

## Release 04 - 11/Jul/2026

- Server version bumped to v1.13.26, shown in /help, in the startup
  banner/log and in the "version" query reply.
- New multi-admin authentication: game.secure now holds one [nickname]
  block per admin, each with its own password. /op <password> is the only
  authentication command and checks the password against the account
  matching the player's own nickname — so both the right nickname AND the
  right password are required. Old single-password game.secure files are
  migrated automatically.
- New /password command: an authenticated admin can change their own
  admin password from the partyline, persisted to game.secure
  immediately.
- Restricted nicknames: connecting under a nickname that has an admin
  account requires authenticating with /op within 60 seconds, otherwise
  the player is disconnected — prevents impostors from occupying admin
  nicknames.
- A fresh game.secure ships a working default admin account
  ([admin] / tetrinetx), so a new install has admin access out of the
  box; the server warns loudly on every startup until the default
  credentials are changed.
- New ban system: /ban bans the target's IP and nickname together (and
  disconnects them immediately), /unban removes a specific entry by type
  (ip or nickname), and /banlist shows every active ban with date, admin
  responsible and reason. game.ban moved to a structured format; old
  files are migrated automatically.
- New /whois <nickname> command: detailed information about a player
  (team, channel, status, client version; host/IP visible to admins
  only).
- /kick no longer disconnects the player: they are redirected to the
  server lobby and blocked from rejoining that specific channel for
  5 minutes — a lighter moderation tool that still stops disruption.
- New extended winlist statistics (total wins, date of last win, best
  and average level in winning games), recorded alongside the classic
  winlist without changing it, plus automatic export to a plain-text CSV
  file (game.winlist.csv) for use outside the game.
- /help rewritten: it now lists only the commands the requesting player
  can actually use at that moment, grouped into sections — general,
  Channel Configuration and, visible to authenticated admins only, Admin
  Commands, Channel Admin Commands and Server Admin Commands — so every
  player sees a short, relevant list instead of a wall of text.
- /priority, /persistant, /save, /reset and /clear are now reserved for
  authenticated admins by default; /kick intentionally stays available
  to channel ops as an everyday moderation tool.
- Default channels: a fresh server now starts with #lobby (where players
  land on connect) and #1x1 (2 players max). Auto-join fills the room
  with the lowest priority number first, and when every room is full new
  connections land in on-demand lobby1/lobby2/... rooms instead of being
  turned away.
- All default files (game.conf, game.motd, game.secure) are now embedded
  in the binary and generated on first boot, so a new install needs no
  bundled config files at all. Default maxchannels raised from 10 to 99.
- New colored, boxed ASCII-art MOTD, with support for an optional color
  tag per line in game.motd (old plain files keep working unchanged).
- Fixed a crash that shut the entire server down whenever a client was
  sent the MOTD, and another that could crash it on the first connection
  to a channel preconfigured in game.conf.
- Fixed pause/unpause: a game could previously be paused and unpaused
  only once, after which the channel got stuck and even "stop game" was
  ignored until the game ended on its own.
- Fixed string values from game.conf and game.motd being silently
  corrupted by Windows (CRLF) line endings — the root cause of the
  Docker container's restart loop.
- Fixed /whois and /banlist being shadowed by /who and /ban (prefix
  collision showed both outputs), and /motd honoring the wrong
  permission flag.
- Code-review pass over src/ fixing latent memory-safety and logic bugs
  (use-after-free, buffer overflows, out-of-bounds accesses, memory
  leak, format-string mismatches), making the server more robust against
  malformed input and abnormal disconnects.
- Startup now logs every boot step, checks up front that the working
  directory is writable, and explains the most common port-bind failures
  with ready-to-run diagnostic commands — much easier to find out why a
  server won't start.
- Docker: the image no longer ships config files (the binary generates
  them itself), the build was fixed (missing libc headers), and
  .gitattributes was added so Windows checkouts can no longer break the
  Linux scripts with CRLF line endings.
- New automated test suites under contrib/tests/ (config migration and
  query-port commands), plus a step-by-step manual test plan for
  everything that needs a real authenticated game session.
- New beginner's guides in English and Brazilian Portuguese ("How to
  start and play" / "Como iniciar e jogar") at the repository root.
- Repository organization: docker/, tests/ and the historical
  OLD.HISTORY/OLD.WISHLIST files moved under contrib/; the changelog is
  now written in English with dated release headings.

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
