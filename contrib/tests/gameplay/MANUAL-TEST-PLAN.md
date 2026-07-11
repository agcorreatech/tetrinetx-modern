# Manual gameplay test plan

These tests require a real, authenticated TetriNET game session (the
encrypted `tetrisstart` INIT handshake — see `src/crack.c` — makes this
impractical to script for this pass; see `contrib/tests/README.md`). Run them
with a real TetriNET 1.13 client (the Windows client bundled at
`tetrinet_windows_client_v1.13/`, or any compatible client) connected to
a locally-built server.

## Setup

```bash
cd src && sed -i -e 's/\r$//' compile.linux && bash compile.linux && cd ..
mkdir -p /tmp/tetrinetx-manual-test && cp bin/tetrix-modern.linux /tmp/tetrinetx-manual-test/
cd /tmp/tetrinetx-manual-test && ./tetrix-modern.linux
```

Point your TetriNET client at `127.0.0.1`, port `31457`. Use at least
**two** client instances/windows for anything involving more than one
player (most of this plan). A quick way to get a second, distinctly
named connection is simply opening the client twice with a different
nickname each time.

Tear down between unrelated test sections by stopping the server
(`pkill tetrix-modern.linux`) and deleting the generated `game.*` files,
so each section starts from a clean slate unless it says otherwise.

---

## 1. Connecting, chat, basics

- [ ] Connect with nickname `alice`. Confirm you land in channel `#lobby`
      (default channel, topic "Server Lobby", priority 1 — created
      automatically together with `#1x1` when `game.conf` defines no
      channels).
- [ ] Run `/list`. **Expected:** `#lobby` (priority 1) and `#1x1`
      (priority 2, max **2** players, topic "Game 1x1") are both listed.
- [ ] Fill `#lobby` to its 6 players, then connect a 7th client.
      **Expected:** the 7th player lands in `#1x1` (the next-lowest
      priority with room). Fill `#1x1` too (2 players) and connect a 9th
      client — **expected:** a `#lobby1` channel is created automatically
      and the player lands there.
- [ ] Connect a second client with nickname `alice` again (same, exact
      spelling). **Expected:** rejected with "Nickname already exists on
      server!" — confirms nicknames are unique server-wide (this is what
      the `/op` nickname-based auth below relies on).
- [ ] Send a partyline chat message from `alice`. Confirm the second
      client (`bob`, connected normally) sees it.
- [ ] `/me waves` — confirm it shows as an action (not a plain chat line)
      to other players.

## 2. `/help` (filtered by current permission)

- [ ] As a normal (unauthenticated) player, run `/help`. **Expected:**
      only commands your current level can use are listed — no
      `--- Admin Commands ---` section at all, and no `/priority`,
      `/clear`, `/persistant`, `/save`, `/reset`, `/ban`, `/unban`,
      `/banlist` (all admin-only by default now).
- [ ] Note whether you're the channel's chanop (lowest gameslot number,
      i.e. first to join — "OP by position", no password needed). If so,
      confirm `/kick` **is** listed (it's chanop-level, not admin-only).
- [ ] After authenticating as admin (section 3below), run `/help` again.
      **Expected:** the same list as before, **plus** a
      `--- Admin Commands ---` section listing `/priority`, `/clear`,
      `/persistant`, `/save`, `/reset`, `/ban`, `/unban`, `/banlist`.
- [ ] Confirm `/set help` still appears in `/help` for the chanop of a
      *non-persistent* channel even without authenticating as admin (the
      `can_use_set()` bonus rule) — this is the one command with a
      permission rule more complex than a plain level check.

## 3. `/op` (multi-admin, nickname-implicit auth)

Default account first — on a **fresh** server (no `game.secure` yet):

- [ ] Confirm the generated `game.secure` contains the default account
      `[admin]` with `password=tetrinetx`, plus header comments explaining
      the `/op` nickname-implicit validation.
- [ ] Confirm the startup output/`game.log` shows the `WARNING` lines
      about the default admin account still being present, recommending
      changing both the account name and the password.
- [ ] Connect with nickname `admin`, run `/op tetrinetx`. **Expected:**
      "Your security level is now: AUTHENTICATED OP".
- [ ] Edit `game.secure` to rename the account/change the password,
      restart, and confirm the startup `WARNING` is gone.

Restricted nicknames (admin-account nicks must authenticate):

- [ ] Connect with a nickname that has an admin account in `game.secure`
      (e.g. `admin` on a fresh install) and do NOT run `/op`.
      **Expected:** no warning is shown on connect, but after ~60 seconds
      (`RESTRICTED_NICK_AUTH_SECS`) the player is disconnected with
      "Restricted nickname. Authentication required to stay connected."
- [ ] Reconnect with the same nickname and run `/op <password>` within
      60 seconds. **Expected:** authenticates normally and is NOT
      disconnected afterwards (play/idle a few minutes to confirm).
- [ ] Connect with a non-admin nickname and stay idle past 60 seconds.
      **Expected:** nothing happens (the deadline only arms for
      nicknames registered in `game.secure`).

`/password <new-password>` (admin changes own password only):

- [ ] As an authenticated admin, run `/password newsecret`.
      **Expected:** "Your admin password has been changed.", the
      `[<nick>]` block in `game.secure` now has `password=newsecret`,
      and a fresh `/op newsecret` (new connection) works while the old
      password fails.
- [ ] Run `/password` with no argument. **Expected:** usage message.
- [ ] Run `/password averylongpassword123` (over 11 chars).
      **Expected:** changed with a "(truncated to 11 characters)" note,
      and `/op averylongpa` (the first 11 chars) is what works.
- [ ] As a non-admin (or chanop-only) player, run `/password x`.
      **Expected:** "You do NOT have access to that command!", and
      `/password` does not appear in that player's `/help`.
- [ ] Confirm `/help` as an authenticated admin: blank line + bold
      `--- Admin Commands ---` header (same style as Channel
      Configuration), `/password` listed there, and NO `/op` entry
      (it only shows for players who haven't authenticated).

Then multi-admin: stop the server, edit `game.secure` to:

```
[alice]
password=alicepass

[bob]
password=bobpass
```

(passwords capped at 11 characters — `PASSLEN` in `src/main.h` — longer
values are silently truncated). Restart the server.

- [ ] Connect with nickname `alice`, run `/op alicepass`. **Expected:**
      "Your security level is now: AUTHENTICATED OP".
- [ ] Connect with nickname `alice`, run `/op bobpass` (the wrong
      password for this nick). **Expected:** "Invalid Password!".
- [ ] Connect with nickname `mallory` (not a registered admin at all),
      run `/op alicepass` (a valid password, but for a different
      nickname). **Expected:** "Invalid Password!" — confirms the
      password alone isn't enough; the nickname must match too.
- [ ] Run `/admin alicepass`. **Expected:** "Invalid /COMMAND!" — the
      old `/admin` alias was removed; `/op` is the only auth command.
- [ ] Restart the server with a **legacy-format** `game.secure`
      (`op_password=somepass`, no `[nickname]` block) and confirm: (a)
      the server logs a migration message, (b) `game.secure` is
      rewritten with a `[admin]` block containing that password, and
      (c) `/op somepass` now requires connecting with the nickname
      `admin` specifically to succeed.

## 4. `/who` and `/whois <nickname>`

With `alice` and `bob` both connected (in the same or different
channels):

- [ ] `/who` as `alice` — confirm both players are listed, with host/IP
      column present **only** if `alice` is currently an authenticated
      admin (section 3) or chanop.
- [ ] `/whois bob` — confirm team, channel, gameslot, status
      (not playing / playing / lost), and client version are shown.
      Confirm host/IP is shown only when `alice` is an authenticated
      admin.
- [ ] `/whois nonexistentnick` — confirm "No such player: nonexistentnick".
- [ ] Start a game (section 6) and run `/whois` on a currently-playing
      player — confirm a "Level" line appears (absent when not playing).

## 5. `/ban`, `/unban`, `/banlist` (admin-only)

As an authenticated admin (section 3), with `bob` connected in the same
channel:

- [ ] `/ban <bob's gameslot> testing the ban command` — confirm: (a) bob
      is disconnected immediately, (b) the channel sees a "was banned"
      message with the reason, (c) bob's client shows "You have been
      banned: testing the ban command" just before disconnecting.
- [ ] Try reconnecting as `bob` from the same machine. **Expected:**
      rejected — either by the IP ban (if reconnecting fast enough that
      the OS reuses the same source characteristics) or, regardless of
      IP/network, by the **nickname** ban as soon as the nickname `bob`
      is sent in the INIT string. Try connecting with a *different*
      nickname from the same machine — **expected:** this succeeds (only
      `bob` the nickname, and the specific banned IP, are blocked — not
      the whole machine under every possible nickname).
- [ ] `/banlist` — confirm two entries are listed for this ban: one
      `IP` type and one `NICK` type, both showing the same date, admin
      nickname, and reason.
- [ ] As a **non-admin** player, run `/banlist`. **Expected:** "You do
      NOT have access to that command!".
- [ ] `/unban nick bob` — confirm `/banlist` now shows only the `IP`
      entry (the `NICK` one is gone). Confirm `bob` can reconnect with
      that nickname again (from a different source IP, or after the IP
      ban is separately lifted).
- [ ] `/unban ip 12.34.56.78` (an IP that was never banned) — confirm
      "No matching ban entry found for (ip): 12.34.56.78".
- [ ] Inspect `game.ban` on disk — confirm the `[BAN]` block format
      (`type=`, `target=`, `date=`, `admin=`, `reason=`), matching what
      `/banlist` reported.

## 6. `/kick` — lobby redirect + 5-minute rejoin cooldown

As the channel's chanop (OP by position — no `/op` needed for
this one, since `/kick` stays at its original chanop-level default):

- [ ] With `bob` connected in `#lobby`, run `/kick <bob's gameslot>`.
      **Expected:** bob is **not disconnected** — his client receives
      messages that he was kicked from `#lobby` and cannot rejoin for
      5 minutes, and is moved to `#lobby1` (created automatically, since
      he was kicked from the lobby itself). Confirm `bob`'s client shows
      him now in `#lobby1` (via `/who` or the client's own channel
      display), still connected and able to chat/join other channels.
- [ ] As `bob` (still connected, now in `#lobby1`), try `/join #lobby`
      (the room he was just kicked from). **Expected:** rejected with a
      message saying he can't rejoin yet, with roughly how long is left.
- [ ] `/join` any **other** channel (not `#lobby`, not `#lobby1`) —
      confirm this succeeds normally (the cooldown only blocks the
      specific room kicked from).
- [ ] Disconnect `bob` entirely and reconnect with the **same nickname**
      `bob` before the 5 minutes are up, then try `/join #lobby`
      again. **Expected:** still rejected — the cooldown is keyed by
      nickname, not by connection, so it survives the reconnect.
- [ ] Wait for the 5 minutes to elapse (or, for a faster test, restart
      the server with a shorter `KICK_COOLDOWN_SECS` temporarily
      recompiled — note cooldowns are in-memory only and don't survive a
      server restart either way), then confirm `bob` can `/join
      #lobby` again normally.
- [ ] Fill `#lobby` to `maxplayers`, then kick a player from a different,
      unrelated room. **Expected:** they land in `#lobby1` instead
      (created automatically), not in the full `#lobby`.
- [ ] Set `command_kick=0` in `game.conf` (disables `/kick` for
      everyone) and restart. As a normal player who happens to be
      chanop, run `/kick <playernumber>` — **expected:** "You do NOT
      have access to that command!" (and `/kick` no longer appears in
      that player's `/help` either). As an **authenticated admin**
      (`/op`), run the same `/kick <playernumber>` —
      **expected:** it still works exactly as before, and `/kick`
      still appears in that admin's `/help` (in the general section,
      not under `--- Admin Commands ---` — `/kick`'s default,
      chanop-level nature doesn't change just because this particular
      config happens to make it admin-only in practice). Confirms
      admins can always use `/kick` regardless of how `command_kick`
      is configured, including fully disabled.

## 7. Admin-only moderation commands

As an authenticated admin (section 3):

- [ ] `/priority 75` — confirm it works. As a **non-admin** chanop
      (OP by position only, no `/op`), confirm the same command is now
      **rejected** ("You do NOT have access to that command!") — this
      moved from chanop-level to admin-only in this release.
- [ ] `/clear` — confirm the winlist is cleared and all connected
      players receive the updated (empty) winlist.
- [ ] `/persistant 1` then `/save` — confirm the channel is written into
      `game.conf` as a `[CHANNELNAME]` preset block.
- [ ] `/reset` — confirm channel config reloads from `game.conf` (e.g. a
      manually-edited setting takes effect without a server restart).
- [ ] Repeat each of the above as a non-admin — confirm all four are
      rejected the same way `/priority` is above.

## 8. `/topic`, `/list`, `/join`, `/msg`, `/move`, `/set`

- [ ] `/topic Friendly game night` — confirm it shows in `/list`.
- [ ] `/list` — confirm channel name, player count, priority, and topic
      are shown, with the current channel highlighted.
- [ ] `/join #newroom` — confirm a new channel is created and you're
      moved into it (gameslot reassigned, `/who` reflects the new
      channel).
- [ ] `/msg <playernumber> hello there` — confirm only that player
      receives the private message.
- [ ] `/move <playernumber> <newnumber>` — confirm the player's gameslot
      changes and everyone's player list updates accordingly.
- [ ] `/set help` then a couple of `/set <option> <value>` calls —
      confirm channel-specific settings (e.g. `starting_level`) take
      effect in the next game.

## 9. Gameplay: starting a game, winning, single-player fix

- [ ] With 2+ players in a channel, start a game. Play until one player
      tops out (loses). **Expected:** the loser gets `playerlost`
      broadcast, the game continues for the remaining player(s).
- [ ] Let the game finish normally with 2 players (one wins). Confirm:
      (a) `/winlist` shows the winner with an updated score on **both**
      clients (the winlist-broadcast fix from Release 03/04 — every
      connected player receives it, not just the first one in the
      channel's internal list), (b) the server announces the winner via
      partyline if `serverannounce` is on.
- [ ] With exactly **one** player in a channel, start a game and
      deliberately lose (top out). **Expected:** the game ends cleanly
      (not stuck in `STATE_INGAME` forever) and the partyline shows
      "Game Over - no winner" — this is the single-player endgame fix.

## 10. Pause / unpause / stop game

With 2+ players in a channel and a game started (section 9), as the
channel's chanop (the player who can start/stop the game):

- [ ] Click **Pause game**. **Expected:** every client shows the paused
      overlay and pieces stop falling.
- [ ] Click **Unpause** (continue). **Expected:** every client resumes.
- [ ] **Pause and unpause again, several times in a row.** **Expected:**
      it keeps working every time — this is the regression this section
      exists for. Before the fix, the game could be paused and unpaused
      exactly once, after which pause, unpause, **and** stop game were all
      silently ignored until the game ended on its own.
- [ ] While the game is **paused**, click **Stop game**. **Expected:** the
      game stops for everyone (the channel returns to the not-in-game
      state) — stopping is allowed directly from a paused game.
- [ ] Start a new game, pause it, and confirm sudden death does **not**
      advance while paused: with a short `sd_timeout` set (via `/set
      SUDDENDEATH` or `game.conf`), pause before the timeout would fire
      and confirm no server-added lines arrive until you unpause.

## 11. Winlist: extended metrics + CSV export

After at least one completed game with a winner (section 9):

- [ ] Inspect `game.winlist.csv` in the server's working directory.
      Confirm the header row `rank,type,name,score,wins,last_win,
      best_level,avg_level` and one data row per winlist entry.
- [ ] Confirm `wins` incremented, `last_win` shows a recent
      human-readable timestamp, and `best_level`/`avg_level` reflect the
      level reached when that entry won (requires the game client to
      have sent at least one `lvl` update during the winning game — most
      clients do this automatically as the player levels up).
- [ ] Play a second game where the **same** player/team wins again,
      reaching a different level than the first win. Confirm `wins`
      incremented to 2, `avg_level` is the average of both wins'
      levels, and `best_level` reflects whichever of the two was higher.
- [ ] Confirm the **in-game** `/winlist` command and the TetriNET
      client's own winlist display are completely unaffected by any of
      this — they show only name and score, exactly as before this
      release.
- [ ] Set `winlist_export_txt=0` in `game.conf`, restart, `/clear` the
      winlist (or finish another game). Confirm `game.winlist.csv` is
      **not** rewritten (the export is now disabled).
