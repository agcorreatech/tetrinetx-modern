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
