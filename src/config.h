/*
  config.h
  
  Definitions in here are pretty safe to modify. Generally user defined
  stuff anyway ;)
  
*/

/* Location of the various external files */
#define FILE_MOTD    "game.motd"	/* Message of the Day File */
#define FILE_CONF    "game.conf"	/* Game configuration File */
#define FILE_WINLIST "game.winlist"	/* Winlist storage file */
#define FILE_WINLISTSTATS "game.winliststats"	/* Extended (victory-only) winlist metrics, binary */
#define FILE_WINLIST_TXT  "game.winlist.csv"	/* Plain-text CSV export of the winlist + extended metrics */
#define FILE_BAN     "game.ban"		/* List of banned IP's and nicknames */
#define FILE_LOG     "game.log"		/* Logfile */
#define FILE_PID     "game.pid"		/* Default PID */
#define FILE_SECURE  "game.secure"	/* Security file (admin accounts) */