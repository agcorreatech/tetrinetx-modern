/*
  game.h
  
  Contains any configuration type external file functions.
  This includes all functions to read and write the game configuration
  files, and of course the winlist functions.
  
*/

/* Write, in plain english, with comments, a game.conf file from current settings */
int gamewrite(void);

/* Read, from a text game.conf file, game settings */
int gameread(void);

/* Initialise game settings to default values */
void init_game(void);

/* Updates a current entry, or creates a new entry with score */
void updatewinlist(char *name, char status, int score);


/* Initialise Winlist structure */
void init_winlist(void);

/* Read Winlist, from winlist file */
void readwinlist(void);

/* Write Winlist to winlist file*/
void writewinlist(void);

/* Send Winlist to players */
void sendwinlist(struct channel_t *chan,struct net_t *n);

/* Strip TetriNET colour-code control characters out of a name (used both
   when sending the winlist to clients, and when exporting it to plain text) */
void strip_colour_codes(char *src, char *dest);

/* Extended (victory-only) winlist metrics: wins, last win date, best level,
   average level. Entirely separate from the original winlist above -- the
   in-game/protocol-facing winlist is never affected by any of this. */
void init_winliststats(void);
void readwinliststats(void);
void writewinliststats(void);
void updatewinliststats(char *name, char status, int level_reached);
int find_winliststats(char *name, char status);

/* Plain-text CSV export of the winlist (+ extended metrics where available).
   Called automatically from writewinlist(); gated by game.winlist_export_txt. */
void writewinlisttxt(void);

/* Admin accounts (game.secure): read/write/init, and login check.
   check_admin_login() compares against the already-connected player's own
   nickname -- see its callers in main.c ("/op" handler). */
int securitywrite(void);
int securityread(void);
void init_security(void);
char check_admin_login(char *nick, char *password);
char is_admin_nick(char *nick);

/* Ban list (game.ban): IP entries (wildcards supported, same syntax as
   before) and nickname entries, each with a reason/date/admin recorded.
   Kept in memory (banlist[]) and mirrored to disk on every change. */
void init_banlist(void);
void readbanlist(void);
void writebanlist(void);
char is_ip_banned(unsigned long ip);
char is_nick_banned(char *nick);
void add_ban(char type, char *target, char *admin_nick, char *reason);
char remove_ban(char type, char *target);

/* Kick cooldowns: temporarily blocks a nickname from rejoining a specific
   room after being /kick'ed from it. Kept purely in memory (not persisted
   across a full server restart -- see CHANGELOG for the rationale). */
void add_kick_cooldown(char *nick, char *channel_name, time_t expires);
char is_kick_cooldown_active(char *nick, char *channel_name);
