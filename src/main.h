/*
  main.h

  Argh, this is sooo messy. I have to clean it someday ;) 
*/

#define TIME_WITH_SYS_TIME 1           
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <netinet/in.h>
#ifndef LINUX    /* linux gets wacko if you include both */
#ifndef __CYGWIN32__  /* Cygnus Win32 isn't all that happy with it either
*/
#include <netinet/tcp.h>
#endif
#endif           /* but virtually every other OS NEEDS both */
#include <arpa/inet.h>      /* is this really necessary? */
#include <stdarg.h> /* ALEX - 16-01-2020 - replaces varargs.h */
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

/* almost every module needs some sort of time thingy, so... */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif 

/* Defines */
#define TETVERSION "1.13"		/* What Tetrinet version we are for */
#define SERVERBUILD "16"		/* What build we are at */
#define NICKLEN 30			/* Maximum length of Nickname */
#define VERLEN 10			/* Maximum length of Tetrinet version */
#define UHOSTLEN 30			/* Maximum length of Hostname */
#define TEAMLEN NICKLEN			/* Maximum length of teamname */
/*#define MAXNET 80*/			/* Maximum network sockets */
#define MAXWINLIST 100			/* Maximum entries on Winlist */
#define TELNET_PORT 31457		/* Telnet port to listen on */
#define QUERY_PORT 31456		/* Query port to listen on */
#define FIELD_MAXX 12			/* Maximum horizontal size of playing field */
#define FIELD_MAXY 22			/* Maximum vertical size of playing field */
#define PIDFILELEN 80			/* Length of name of PID file */
#define SDMSGLEN 80			/* Length of Sudden Death Message */
#define PASSLEN 12			/* Length of Password */
#define MAXPLAYERS 6			/* Maximum number of players allowed */
#define CHANLEN 16			/* Length of channel names */
#define IPLEN 16			/* Size of IP */
#define DEFAULTCHANNEL "tetrinet"	/* Default channel name */
#define DEFAULTMAXPLAYERS 6		/* Default max players in channel */
#define DEFAULTPRIORITY 50		/* Default priority */
#define DESCRIPTIONLEN 31		/* Description */

#define STATE_OFFLINE 0 	/* Offline */
#define STATE_ONLINE  1		/* Not in game */
#define STATE_INGAME  2		/* In game */
#define STATE_PAUSED  3		/* In game but paused */

#define NET_FREE 0		/* Unused at the momen */
#define NET_TELNET 1		/* Person has just connected */
#define NET_TELNET_INIT 2	/* Person is sending init sequence */
#define NET_WAITINGFORTEAM 3	/* Waiting for initial team */
#define NET_CONNECTED 4		/* Everything works out, they're connected */
#define NET_QUERY 5		/* Person has connected to Query Port */
#define NET_QUERY_INIT 6	/* Person is sending query request */
#define NET_LOST 9		/* Lost connection */

#define STAT_NOTPLAYING 0	/* Not playing */
#define STAT_PLAYING 1		/* Currently playing */
#define STAT_LOST 2		/* Playing, but lost */

#define SD_NONE 0		/* No mode. Ignore timeout */
#define SD_INIT 1		/* Wait for timeout, it's first suddendeath */
#define SD_WAIT 2		/* Waiting between lines */

#define LEVEL_UNUSED 0		/* Unused level */
#define LEVEL_NORMAL 1		/* Unauthenticated Person */
#define LEVEL_OP     2		/* Op by position player */
#define LEVEL_AUTHOP 3		/* OP'ed person (/op) */

#define CYCLE 1				/* How many (s) is 1 cycle */

typedef unsigned long IP;

/* public structure of all the net connections */ 

struct channel_t {
  char name[CHANLEN+1];			/* Name of the channel */
  int priority;				/* Priority of channel */
  int maxplayers;			/* Maximum players allowed */
  unsigned char status;			/* STATE_XXXXXX */
  struct channel_t *next;		/* Next in the queue */
  char description[DESCRIPTIONLEN];	/* Description */
  char persistant;			/* 1=can't delete */
  struct net_t *net;			/* Net structure */
    
  int sd_timeleft;			/* Sudden Death Timeout */
  int sd_mode;				/* What SD mode we're in - SD_XXXX */

  /* Channel Settings */
  int starting_level;
  int lines_per_level;
  int level_increase;
  int lines_per_special;
  int special_added;
  int special_capacity;
  int classic_rules;
  int average_levels;
  int sd_timeout;
  int sd_lines_per_add;
  int sd_secs_between_lines;
  char sd_message[SDMSGLEN];
 
  int block_leftl;
  int block_leftz;
  int block_square;
  int block_rightl;
  int block_rightz;
  int block_halfcross;
  int block_line;
  int special_addline;
  int special_clearline;
  int special_nukefield;
  int special_randomclear;
  int special_switchfield;
  int special_clearspecial;
  int special_gravity;
  int special_quakefield;
  int special_blockbomb;

  int stripcolour;		/* Strip colour from gmsg's */
  int serverannounce;		/* Server announces winner */
  int pingintercept;		/* Intercept ping's */
  
  
};

/* Server Config */
struct game_t {
  char bindip[IPLEN+1];
  int maxchannels;
  int starting_level;
  int lines_per_level;
  int level_increase;
  int lines_per_special;
  int special_added;
  int special_capacity;
  int classic_rules;
  int average_levels;
  int sd_timeout;
  int sd_lines_per_add;
  int sd_secs_between_lines;
  char sd_message[SDMSGLEN+1];
  int timeout_ingame;
  int timeout_outgame;
 
  int block_leftl;
  int block_leftz;
  int block_square;
  int block_rightl;
  int block_rightz;
  int block_halfcross;
  int block_line;
  int special_addline;
  int special_clearline;
  int special_nukefield;
  int special_randomclear;
  int special_switchfield;
  int special_clearspecial;
  int special_gravity;
  int special_quakefield;
  int special_blockbomb;

  int stripcolour;		/* Strip colour from gmsg's */
  int serverannounce;		/* Server announces winner */
  int pingintercept;		/* Intercept ping's */
  
  int command_clear;		/* Allow clearing of winlist? */
  int command_kick;		/* Allow Kicks? */
  int command_msg;		/* Allow msg's? */
  int command_op;		/* Allow command OP */
  int command_winlist;		/* Allow winlist */
  int command_help;		/* Allow help */
  int command_list;		/* Allow list */
  int command_join;		/* Allow join */
  int command_who;		/* Allow who */
  int command_topic;		/* Allow topic */
  int command_priority;		/* Allow priority */
  int command_move;		/* Allow move */
  int command_set;		/* Allow set */
  int command_persistant;
  int command_save;
  int command_reset;
  
  int verbose;			/* Verbosity */
  char pidfile[PIDFILELEN+1];
};



struct net_t {
  int sock; 				/* Socket this player is on */
  IP addr; 				/* IP address of player */
  unsigned int port; 			/* Port number they connected to */
  char nick[NICKLEN+1];			/* Nickname of player */
  char team[TEAMLEN+1]; 		/* Teamname of player */
  char host[UHOSTLEN+1];		/* Resolved host */
  char version[VERLEN+1];		/* TetriNET version */
  int gameslot;				/* What slot (1-6) they occupy */
  int level;				/* What playing level they've reached */
  unsigned char field[FIELD_MAXX][FIELD_MAXY];   /* Playing Field of player */
  unsigned char status;			/* Current Status - STAT_XXXXX */
  int timeout;				/* Timeout on socket */
  int securitylevel;			/* What security LEVEL - LEVEL_XXX */
  
  struct channel_t *channel;		/* What channel we're on */
  unsigned char type; 			/* What this record type is - NET_XXXXXX */

  struct net_t *next;			/* Next in list */
}; 

/* Winlist Structure */
struct winlist_t {
  char status;				/* Type. p=player, t=team */
  char name[NICKLEN+1];			/* Name of player/team */
  unsigned long int score;		/* What they scored */
  char inuse;				/* 1=inuse 0=available */
};



/* Security structure */
struct security_t {
  char op_password[PASSLEN+1];		/* Password to take ops */
};










/* Colours defined here */
/* I found these defined in "TetriNET Color Addon for mIRC" by TNL */
#define BOLD		2
#define ITALIC		22
#define UNDERLINE	31
#define BLACK		4
#define DARKGRAY	6
#define SILVER		15
#define NAVY		17
#define BLUE		5
#define CYAN		3
#define GREEN		12
#define NEON		14
#define TEAL		23
#define BROWN		16
#define RED		20
#define MAGENTA		8
#define VIOLET		19
#define YELLOW		25
#define WHITE		24

/* net list */
struct net_t *gnet;			/* Start of global "socket" information */
struct game_t game;			/* Game Configuration */
struct winlist_t winlist[MAXWINLIST];	/* Winlist */
struct security_t security;		/* Security structure */
struct channel_t *chanlist;		/* Channel structure */

/* And the proto types */
void net_telnet_init(struct net_t *n, char *buf);
void net_telnet(struct net_t *n, char *buf);
void net_query_init(struct net_t *n, char *buf);
void net_query(struct net_t *n, char *buf);
void lostnet(struct net_t *n);
void net_connected(struct net_t *n, char *buf);
void net_waitingforteam(struct net_t *n, char *buf);
void init_main(void);
void lprintf();
void lvprintf();