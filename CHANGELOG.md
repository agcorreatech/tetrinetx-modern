# Changelog

Maintainer / Developer: Alexandro G. Corrêa <alex.linux@gmail.com>

## Release 04 - 12/Jul/2026

- The server now seeds 20 default rooms instead of 4: the lobby plus
  nine game modes (#classic, #pure, #speed, #sudden, #rush,
  #lines, #nolines, #bomb, #chaos), each also available as a 2-player
  1x1 twin (#tetrinet1x1, #classic1x1, ...) — so a fresh install offers
  a full variety of game styles out of the box. The lobby, #classic and
  #pure score on the global winlist; every other room keeps its own.
- New per-channel description text (description= in game.conf): shown
  as a "Channel description:" message every time a player enters the
  room, briefly explaining how that room's game works. It does not
  appear in /list, and rooms created by players don't have one.
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
- The /op success message now reads "Your security level is now:
  SERVER ADMIN" (was "AUTHENTICATED OP"), matching the wording /help
  uses for the admin role.
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
- Game actions are now announced with WHO did them, in the client's
  classic red style with the action and nickname in bold:
  "*** The Game Has Started/Ended/Paused/Unpaused by <nick>" (pause and
  unpause previously showed nothing at all). The winner announcement is
  now "*** The Game Has Ended - The winner is <nick>! Congratulations!"
  ("... Team <team>! ..." in team games) and the no-winner one is
  "*** The Game Has Ended - No Winner! :(" — replacing the old
  "-=== ... WON ===-" / "-=== Game Over - no winner ===-" texts. The
  bundled client's own generic "The Game Has Started/Ended" lines were
  blanked out so the attributed server lines aren't shown twice
  (unpatched clients will see both).
- When the last opponent leaves mid-game (disconnects, /joins another
  channel or is kicked), the remaining player/team is now declared the
  winner — same announcement, winlist points and extended stats as a
  normal victory. Previously the game just ended silently with no
  winner, punishing the player who stayed.
- /help usage strings standardized on <player-number> (was an
  inconsistent mix of <playernumber>, <playernum> and <newnum>), and
  /join's argument clarified to <#channel|channel-number>.
- Bundled Windows client (TETRINET.EXE inside the client zip) updated
  with two clearer messages: "*** <nick> is Now Alone" now reads
  "*** <nick> has No Team", and "*** Server has Shut Down" — which the
  client showed on ANY disconnect, even when only that player was
  dropped — now reads "*** You are Disconnected". These texts are
  produced by the client itself, not by the server, so they could only
  be changed inside the client executable (in-place binary text patch;
  everything else in the client is untouched).
- Repository organization: docker/, tests/ and the historical
  OLD.HISTORY/OLD.WISHLIST files moved under contrib/; the changelog is
  now written in English with dated release headings.
- Fixed: after being kicked or moved to another channel, the player's
  client kept showing the OLD room's player list — the server now sends
  the full player-panel cleanup on every channel move.
- Fixed: switching channels with /join mid-game did not end the old
  game nor declare the remaining player the winner.
- Fixed: /kick could target yourself; self-kick is now rejected.
- Fixed: a winner's "level reached" could record memory garbage into the
  extended stats when the game ended before the first level-up.
- Fixed: clearing the winlist left the extended stats (wins, best/avg
  level) and the CSV export behind, permanently out of sync.
- New per-channel winlists: /ownwinlist <0/1> gives a channel its own
  separate winlist (kept in game.winlist.<name>, auto-saved to
  game.conf); /winlists lists every winlist on the server; /winlist
  accepts [n] [#channel|global] to view any of them; /clear now takes a
  mandatory target (global or #channel) and announces to the affected
  players WHO reset it. Extended stats and the CSV export remain
  global-winlist-only.
- When a player tops out, the channel now sees "*** <nick> Has Lost the
  Game" (same red/bold style); when the loss also ends the game, the
  winner announcement is always the last line.
- /kick that removes one of the last two playing players now ANNULS the
  game — no winner, no points — so a chanop can't kick their final
  opponent to steal the win. A voluntary /join or a disconnect still
  crowns the survivor; kicks with 3+ players playing don't end the game.
- A room emptied mid-game or mid-pause (players leaving, disconnecting
  or timing out) always resets to its normal state with no winner — the
  next joiner can no longer walk into a ghost game or ghost pause.
- New connections now ALWAYS land in a lobby room (lobby, lobby1, ...,
  created on demand) — never in a game room like #1x1, even with free
  slots; game rooms are entered explicitly with /join. Channel priority
  now only orders the /list display.
- /topic on preset/persistent channels is reserved for authenticated
  admins; regular players can only retitle rooms they created (and
  /help hides /topic where it can't be used).
- /topic, /persistant, /priority and /ownwinlist now save to game.conf
  automatically — no separate /save needed.
- Inactivity disconnection: default lowered to 10 minutes out of game,
  applies to everyone (chanops included), and the player now gets an
  explicit "You have been disconnected due to inactivity." message.
- The deadline for admin-account nicknames to authenticate with /op was
  reduced from 60 to 25 seconds.
- /ban now requires a reason (shown to the target, the channel and
  /banlist) and rejects the command without one.
- /password rejects passwords outside 6-11 characters with a clear
  message instead of silently truncating them.
- The kick-cooldown rejection now says exactly how long is left
  ("Try again in 4 minutes.") instead of "a few minutes".
- Pause/unpause is accepted from any playing member, not only the
  chanop (the bundled client still only offers the button to the game
  moderator).
- Using /kick while it's disabled in game.conf now answers "You do NOT
  have access to that command!" instead of the confusing
  "Invalid /COMMAND!".
- Nicer output formatting: /whois with bold labels and "No Team" instead
  of a blank, /banlist as aligned two-line blocks per ban, and /list
  with the priority column zero-padded.
- Two more default rooms on a fresh install: #sudden ("Sudden Death")
  and #sudden1x1 (2 players, "1x1 Sudden Death"), both with sudden
  death armed by default (2 minutes into every game). Default /list
  priorities: lobby rooms count up from 01 following their names
  (lobby=01, lobby1=02, ...), #1x1=30, #sudden=31, #sudden1x1=32.
- #1x1, #sudden and #sudden1x1 keep their OWN winlists by default
  instead of scoring on the global one; every winlist (global and
  per-channel) is exported to its own CSV file with the same columns
  (game.winlist.csv / game.winlist.<name>.csv), including the extended
  stats, which are now tracked per channel winlist too.
- A channel can also be set to score on NO winlist at all
  (/ownwinlist 2, or own_winlist=2 in game.conf) — wins there count
  nowhere and produce no CSV.
- On entering any room the player is told what it scores on: the global
  winlist, the room's own winlist, or no winlist at all.
- /winlist [n] with no other argument now shows the top n of EVERY
  winlist on the server (Global first, then each channel's own), one
  block per winlist; /winlist [n] <#channel|global> still narrows it to
  a single one.

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
