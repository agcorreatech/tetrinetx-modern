/*
  game.c
  
*/

/* securitywrite() */
/*   Writes out the security structure (registered admin accounts) into a */
/*   text format game.secure file, one [nickname] block per admin. */
int securitywrite()
  {
    FILE *file_out;
    int i;
    
    file_out = fopen(FILE_SECURE, "w");
    if (file_out == NULL)
      return(-1);
      
    fprintf(file_out,"# TetriNET (linux) Security configuration file\n");
    fprintf(file_out,"\n");
    fprintf(file_out,"# This file contains one block per admin account, in the form:\n");
    fprintf(file_out,"#   [nickname]\n");
    fprintf(file_out,"#   password=somepass\n");
    fprintf(file_out,"#\n");
    fprintf(file_out,"# Typing \"/op <password>\" or \"/admin <password>\" in the partyline\n");
    fprintf(file_out,"# authenticates as the admin whose [nickname] matches the nickname you are\n");
    fprintf(file_out,"# CURRENTLY connected with (there is no separate username to type -- your\n");
    fprintf(file_out,"# TetriNET nickname IS the username, which is an extra layer of protection\n");
    fprintf(file_out,"# since nicknames are unique server-wide and can't be changed mid-session).\n");
    fprintf(file_out,"#\n");
    fprintf(file_out,"# Any text after a # is ignored, and can be used as comments.\n");
    fprintf(file_out,"\n");
    fprintf(file_out,"# Example:\n");
    fprintf(file_out,"#[alex]\n");
    fprintf(file_out,"#password=supersecret\n");
    fprintf(file_out,"\n");

    for (i=0; i<MAXADMINS; i++)
      {
        if (security.adminlist[i].inuse)
          {
            fprintf(file_out,"[%s]\n", security.adminlist[i].nick);
            fprintf(file_out,"password=%s\n", security.adminlist[i].password);
            fprintf(file_out,"\n");
          }
      }

    fprintf(file_out,"# End of File\n");   
    
    fclose(file_out);
    
    lvprintf(3,"Wrote new security configuration to %s\n", FILE_SECURE);
    return(0);
  }

/* find_or_add_admin_slot(nick) - Returns the index of the admin slot for */
/*   this nickname (existing or newly allocated), or -1 if the table is full */
int find_or_add_admin_slot(char *nick)
  {
    int i, free_slot;

    free_slot = -1;
    for (i=0; i<MAXADMINS; i++)
      {
        if (security.adminlist[i].inuse && !strcasecmp(security.adminlist[i].nick, nick))
          return i;
        if (!security.adminlist[i].inuse && (free_slot==-1))
          free_slot = i;
      }
    if (free_slot == -1)
      return -1;

    security.adminlist[free_slot].inuse = 1;
    strncpy(security.adminlist[free_slot].nick, nick, NICKLEN-1);
    security.adminlist[free_slot].nick[NICKLEN-1] = 0;
    security.adminlist[free_slot].password[0] = 0;
    return free_slot;
  }

/* securityread() */
/*   Reads game.secure (one [nickname] block per admin, each with a */
/*   "password=" tag) into the security structure. */
/*   BACKWARD COMPATIBILITY: a bare "op_password=X" tag found OUTSIDE of any */
/*   [nickname] block (the old, single-shared-password format) is migrated */
/*   automatically into an admin account named "admin", and the file is */
/*   rewritten in the new format -- so upgrades don't silently lose access. */
int securityread(void)
  { 
  
    FILE *file_in;
    char buf[513];
    char id_tag[81];
    char id_value[81];
    int i,j,error;
    int cur_admin;
    char needs_migration;
    
    for (i=0; i<MAXADMINS; i++)
      security.adminlist[i].inuse = 0;

    file_in = fopen(FILE_SECURE, "r");
    if (file_in == NULL)
      return(-1);

    cur_admin = -1;
    needs_migration = 0;
      
    while(!feof(file_in))
      {
        if(fscanf(file_in," %512[^\n]\n", buf) != 1)
        {
          printf("Error: Failed to read security file: %s.\n",FILE_SECURE);
        }
        
        i=0; j=strlen(buf);
        while( (i<j) && (buf[i]!='#') ) i++;
        if (buf[i]=='#') buf[i] = '\0'; /* Truncate string to # char */
        
        j=strlen(buf)-1;
        while( (j>=0) && (buf[j]==' ') )
          {
            buf[j]='\0';       /* Strip trailing spaces */
            j--;
          }
          
        j=strlen(buf);
        if (j != 0)
          {
            error = 1;

            /* Is it a new admin block declaration? [nickname] */
            if ( (buf[0]=='[') && (buf[strlen(buf)-1]==']') )
              {
                sscanf(buf,"[%60[^]\n]",id_tag);
                cur_admin = find_or_add_admin_slot(id_tag);
                error = 0;
              }
            else
              {
                sscanf(buf,"%80[^= ] = %80s", id_tag, id_value);

                if (!strcasecmp(id_tag,"password") && (cur_admin>=0))
                  {
                    strncpy(security.adminlist[cur_admin].password, id_value, PASSLEN-1);
                    security.adminlist[cur_admin].password[PASSLEN-1]=0;
                    error=0;
                  }

                /* Legacy format: a bare op_password=X tag, outside any block */
                if (!strcasecmp(id_tag,"op_password"))
                  {
                    int legacy_slot = find_or_add_admin_slot("admin");
                    if (legacy_slot >= 0)
                      {
                        strncpy(security.adminlist[legacy_slot].password, id_value, PASSLEN-1);
                        security.adminlist[legacy_slot].password[PASSLEN-1]=0;
                        needs_migration = 1;
                      }
                    error=0;
                  }
            
                if (error==1)
                  {
                    lvprintf(2,"%s: Unknown Identifier: %s\n", FILE_SECURE, buf);
                  }
              }
          }
      }
    fclose(file_in);
    lvprintf(3,"Read security configuration from %s\n", FILE_SECURE);

    if (needs_migration)
      {
        lvprintf(1,"%s: Migrated legacy 'op_password' to an admin account named 'admin' -- edit %s to rename it and/or add more admins. You must now be connected with the nickname 'admin' to use /op or /admin.\n", FILE_SECURE, FILE_SECURE);
        securitywrite();
      }

    return(0);
  }


/* init_security() */
/*   Initialises the security structure (no admins registered) */
void init_security(void)
  {
    int i;
    for (i=0; i<MAXADMINS; i++)
      security.adminlist[i].inuse = 0;
  }

/* check_admin_login(nick, password) - Returns 1 if nick+password matches a */
/*   registered admin account. nick is matched case-insensitively (same as */
/*   nickname comparisons elsewhere), password is matched case-sensitively. */
char check_admin_login(char *nick, char *password)
  {
    int i;
    for (i=0; i<MAXADMINS; i++)
      {
        if ( security.adminlist[i].inuse
             && !strcasecmp(security.adminlist[i].nick, nick)
             && !strcmp(security.adminlist[i].password, password) )
          return 1;
      }
    return 0;
  }


/* --------------------------------------------------------------------- */
/* Ban list (game.ban): IP entries (wildcards supported) and nickname     */
/* entries, each recording a reason, timestamp, and the admin who applied */
/* it. Kept in memory (banlist[]) and rewritten to disk on every change,  */
/* rather than re-parsed from scratch on every connection attempt.        */
/* --------------------------------------------------------------------- */

/* init_banlist() - Clears the in-memory ban list */
void init_banlist(void)
  {
    int i;
    for (i=0; i<MAXBANS; i++)
      banlist[i].inuse = 0;
  }

/* find_free_ban_slot() - Returns the index of a free slot, or -1 if full */
int find_free_ban_slot(void)
  {
    int i;
    for (i=0; i<MAXBANS; i++)
      if (!banlist[i].inuse)
        return i;
    return -1;
  }

/* writebanlist() - Rewrites game.ban from the in-memory banlist[] array, */
/*   one [BAN] block per entry. */
void writebanlist(void)
  {
    FILE *file_out;
    int i;

    file_out = fopen(FILE_BAN, "w");
    if (file_out == NULL)
      return;

    fprintf(file_out,"# TetriNET (linux) Ban list\n");
    fprintf(file_out,"#\n");
    fprintf(file_out,"# Each ban is a block:\n");
    fprintf(file_out,"#   [BAN]\n");
    fprintf(file_out,"#   type=ip|nick\n");
    fprintf(file_out,"#   target=<IP (wildcards allowed, e.g. 192.168.1.*) or nickname>\n");
    fprintf(file_out,"#   date=<unix timestamp>\n");
    fprintf(file_out,"#   admin=<nickname of the admin who applied it>\n");
    fprintf(file_out,"#   reason=<free text>\n");
    fprintf(file_out,"#\n");
    fprintf(file_out,"# This file is managed automatically by /ban and /unban; manual edits are\n");
    fprintf(file_out,"# fine (e.g. to add a wildcard IP ban by hand), just follow the same format.\n");
    fprintf(file_out,"\n");

    for (i=0; i<MAXBANS; i++)
      {
        if (banlist[i].inuse)
          {
            fprintf(file_out,"[BAN]\n");
            fprintf(file_out,"type=%s\n", (banlist[i].type==BAN_TYPE_IP ? "ip" : "nick"));
            fprintf(file_out,"target=%s\n", banlist[i].target);
            fprintf(file_out,"date=%lu\n", (unsigned long)banlist[i].when);
            fprintf(file_out,"admin=%s\n", banlist[i].admin);
            fprintf(file_out,"reason=%s\n", banlist[i].reason);
            fprintf(file_out,"\n");
          }
      }

    fprintf(file_out,"# End of File\n");
    fclose(file_out);
    lvprintf(3,"Wrote ban list to %s\n", FILE_BAN);
  }

/* readbanlist() - Reads game.ban ([BAN] block format) into banlist[].     */
/*   BACKWARD COMPATIBILITY: a bare IP-pattern line found OUTSIDE of any   */
/*   [BAN] block (the old flat "one wildcarded IP per line" format) is    */
/*   migrated automatically into a proper [BAN] entry (type=ip), and the  */
/*   file is rewritten in the new format. */
void readbanlist(void)
  {
    FILE *file_in;
    char buf[513];
    char id_tag[81];
    char id_value[121];
    char ip1[4],ip2[4],ip3[4],ip4[4];
    int i,j;
    int cur_ban;
    char needs_migration;

    init_banlist();

    file_in = fopen(FILE_BAN, "r");
    if (file_in == NULL)
      return;

    cur_ban = -1;
    needs_migration = 0;

    while (!feof(file_in))
      {
        if (fscanf(file_in," %512[^\n]\n", buf) != 1)
          {
            /* EOF or blank remainder -- nothing to parse this pass */
          }

        i=0; j=strlen(buf);
        while ( (i<j) && (buf[i]!='#') ) i++;
        if (buf[i]=='#') buf[i]='\0';

        j=strlen(buf)-1;
        while ( (j>=0) && (buf[j]==' ') )
          {
            buf[j]='\0';
            j--;
          }

        j=strlen(buf);
        if (j != 0)
          {
            if ( (buf[0]=='[') && (buf[strlen(buf)-1]==']') )
              {
                sscanf(buf,"[%60[^]\n]",id_tag);
                if (!strcasecmp(id_tag,"BAN"))
                  {
                    cur_ban = find_free_ban_slot();
                    if (cur_ban >= 0)
                      {
                        banlist[cur_ban].inuse = 1;
                        banlist[cur_ban].type = BAN_TYPE_IP;
                        banlist[cur_ban].target[0]=0;
                        banlist[cur_ban].when = time(NULL);
                        banlist[cur_ban].admin[0]=0;
                        strncpy(banlist[cur_ban].reason,"No reason given",BANREASONLEN-1);
                        banlist[cur_ban].reason[BANREASONLEN-1]=0;
                      }
                  }
                else
                  cur_ban = -1;
              }
            else if (cur_ban >= 0)
              {
                sscanf(buf,"%80[^= ] = %120[^\n]", id_tag, id_value);

                if (!strcasecmp(id_tag,"type"))
                  banlist[cur_ban].type = (!strcasecmp(id_value,"nick") ? BAN_TYPE_NICK : BAN_TYPE_IP);
                else if (!strcasecmp(id_tag,"target"))
                  { strncpy(banlist[cur_ban].target,id_value,UHOSTLEN-1); banlist[cur_ban].target[UHOSTLEN-1]=0; }
                else if (!strcasecmp(id_tag,"date"))
                  banlist[cur_ban].when = (time_t)atol(id_value);
                else if (!strcasecmp(id_tag,"admin"))
                  { strncpy(banlist[cur_ban].admin,id_value,NICKLEN-1); banlist[cur_ban].admin[NICKLEN-1]=0; }
                else if (!strcasecmp(id_tag,"reason"))
                  { strncpy(banlist[cur_ban].reason,id_value,BANREASONLEN-1); banlist[cur_ban].reason[BANREASONLEN-1]=0; }
              }
            else
              {
                /* Legacy flat format: a bare wildcarded IP line, e.g. "192.168.1.*" */
                if (sscanf(buf," %4[0-9*] . %4[0-9*] . %4[0-9*] . %4[0-9*]",ip1,ip2,ip3,ip4)==4)
                  {
                    int slot = find_free_ban_slot();
                    if (slot >= 0)
                      {
                        banlist[slot].inuse = 1;
                        banlist[slot].type = BAN_TYPE_IP;
                        sprintf(banlist[slot].target,"%s.%s.%s.%s",ip1,ip2,ip3,ip4);
                        banlist[slot].when = time(NULL);
                        strncpy(banlist[slot].admin,"(migrated)",NICKLEN-1); banlist[slot].admin[NICKLEN-1]=0;
                        strncpy(banlist[slot].reason,"Migrated from legacy ban list",BANREASONLEN-1); banlist[slot].reason[BANREASONLEN-1]=0;
                        needs_migration = 1;
                      }
                  }
              }
          }
      }
    fclose(file_in);
    lvprintf(3,"Read ban list from %s\n", FILE_BAN);

    if (needs_migration)
      {
        lvprintf(1,"%s: Migrated legacy IP-only ban entries to the new format.\n", FILE_BAN);
        writebanlist();
      }
  }

/* ip_matches_pattern(ip, pattern) - Returns 1 if ip (as a plain dotted   */
/*   string) matches pattern (which may contain '*' wildcards per octet, */
/*   same syntax the original ban file always supported) */
char ip_matches_pattern(unsigned long ip, char *pattern)
  {
    char n1[4],n2[4],n3[4],n4[4];
    char p1[4],p2[4],p3[4],p4[4];

    sprintf(n1,"%lu",(unsigned long)(ip&0xff000000)/(unsigned long)0x1000000);
    sprintf(n2,"%lu",(unsigned long)(ip&0x00ff0000)/(unsigned long)0x10000);
    sprintf(n3,"%lu",(unsigned long)(ip&0x0000ff00)/(unsigned long)0x100);
    sprintf(n4,"%lu",(unsigned long)ip&0x000000ff);

    if (sscanf(pattern," %3[0-9*].%3[0-9*].%3[0-9*].%3[0-9*]",p1,p2,p3,p4) != 4)
      return 0;

    if ( ((!strcmp(n1,p1))||(!strcmp(p1,"*")))
      && ((!strcmp(n2,p2))||(!strcmp(p2,"*")))
      && ((!strcmp(n3,p3))||(!strcmp(p3,"*")))
      && ((!strcmp(n4,p4))||(!strcmp(p4,"*"))) )
      return 1;
    return 0;
  }

/* is_ip_banned(ip) - Returns 1 if this IP matches any IP-type ban entry */
char is_ip_banned(unsigned long ip)
  {
    int i;
    for (i=0; i<MAXBANS; i++)
      if ( banlist[i].inuse && (banlist[i].type==BAN_TYPE_IP) && ip_matches_pattern(ip,banlist[i].target) )
        return 1;
    return 0;
  }

/* is_nick_banned(nick) - Returns 1 if this nickname matches any nick-type */
/*   ban entry (case-insensitive) */
char is_nick_banned(char *nick)
  {
    int i;
    for (i=0; i<MAXBANS; i++)
      if ( banlist[i].inuse && (banlist[i].type==BAN_TYPE_NICK) && !strcasecmp(banlist[i].target,nick) )
        return 1;
    return 0;
  }

/* add_ban(type, target, admin_nick, reason) - Adds a new ban entry and */
/*   persists the list to disk. Silently ignored if the table is full. */
void add_ban(char type, char *target, char *admin_nick, char *reason)
  {
    int slot = find_free_ban_slot();
    if (slot == -1)
      {
        lvprintf(1,"WARNING: Ban list full (MAXBANS=%d), could not add ban for '%s'\n", MAXBANS, target);
        return;
      }
    banlist[slot].inuse = 1;
    banlist[slot].type = type;
    strncpy(banlist[slot].target, target, UHOSTLEN-1); banlist[slot].target[UHOSTLEN-1]=0;
    banlist[slot].when = time(NULL);
    strncpy(banlist[slot].admin, admin_nick, NICKLEN-1); banlist[slot].admin[NICKLEN-1]=0;
    strncpy(banlist[slot].reason, (reason&&reason[0]) ? reason : "No reason given", BANREASONLEN-1);
    banlist[slot].reason[BANREASONLEN-1]=0;
    writebanlist();
  }

/* remove_ban(type, target) - Removes the ban entry matching type+target */
/*   exactly (case-insensitive). Returns 1 if one was removed. */
char remove_ban(char type, char *target)
  {
    int i;
    for (i=0; i<MAXBANS; i++)
      {
        if ( banlist[i].inuse && (banlist[i].type==type) && !strcasecmp(banlist[i].target,target) )
          {
            banlist[i].inuse = 0;
            writebanlist();
            return 1;
          }
      }
    return 0;
  }


/* --------------------------------------------------------------------- */
/* Kick cooldowns: temporarily blocks a nickname from rejoining a specific */
/* room after being /kick'ed from it. Kept purely in memory, keyed by      */
/* nickname (not by connection), so the block survives a disconnect and    */
/* reconnect during the cooldown window. Not persisted to disk -- a full   */
/* server restart clears all active cooldowns (see CHANGELOG).            */
/* --------------------------------------------------------------------- */

/* add_kick_cooldown(nick, channel_name, expires) - Registers (or refreshes) */
/*   a cooldown blocking this nickname from rejoining this room until the */
/*   given expiry timestamp. */
void add_kick_cooldown(char *nick, char *channel_name, time_t expires)
  {
    int i, slot;

    slot = -1;
    for (i=0; i<MAXKICKCOOLDOWNS; i++)
      {
        if ( kick_cooldowns[i].inuse
             && !strcasecmp(kick_cooldowns[i].nick,nick)
             && !strcasecmp(kick_cooldowns[i].channel_name,channel_name) )
          { slot = i; break; }
        if ( (!kick_cooldowns[i].inuse) && (slot==-1) )
          slot = i;
      }
    if (slot == -1)
      {
        lvprintf(1,"WARNING: Kick cooldown table full (MAXKICKCOOLDOWNS=%d), could not register cooldown for '%s'\n", MAXKICKCOOLDOWNS, nick);
        return;
      }

    kick_cooldowns[slot].inuse = 1;
    strncpy(kick_cooldowns[slot].nick, nick, NICKLEN-1); kick_cooldowns[slot].nick[NICKLEN-1]=0;
    strncpy(kick_cooldowns[slot].channel_name, channel_name, CHANLEN-1); kick_cooldowns[slot].channel_name[CHANLEN-1]=0;
    kick_cooldowns[slot].expires = expires;
  }

/* is_kick_cooldown_active(nick, channel_name) - Returns 1 if this nickname */
/*   is still blocked from rejoining this room. Expired entries are freed */
/*   (lazily) as they're encountered, so the table doesn't fill up with */
/*   stale entries over time. */
char is_kick_cooldown_active(char *nick, char *channel_name)
  {
    int i;
    time_t now = time(NULL);

    for (i=0; i<MAXKICKCOOLDOWNS; i++)
      {
        if (kick_cooldowns[i].inuse)
          {
            if (kick_cooldowns[i].expires <= now)
              { kick_cooldowns[i].inuse = 0; continue; }   /* expired, free it lazily */

            if ( !strcasecmp(kick_cooldowns[i].nick,nick) && !strcasecmp(kick_cooldowns[i].channel_name,channel_name) )
              return 1;
          }
      }
    return 0;
  }


/* gamewrite() */
/*   Writes out the game structure into a text format game.conf file */
int gamewrite(void)
  {
    FILE *file_out;
    struct channel_t *chan;
    
    file_out = fopen(FILE_CONF, "w");
    if (file_out == NULL)
      return(-1);
      
    fprintf(file_out,"# TetriNET (linux) Game configuration file\n");
    fprintf(file_out,"\n");
    fprintf(file_out,"# This file contains configuration for TetriNET, and will be autocreated\n");
    fprintf(file_out,"# with default values if it does not exist.\n");
    fprintf(file_out,"# Each configuration value consists of a TAG name, followed by an equal sign\n");
    fprintf(file_out,"# and a value. IE:\n");
    fprintf(file_out,"#      starting_level=1\n");
    fprintf(file_out,"# Any text after a # is ignored, and can be used as comments.\n");
    fprintf(file_out,"\n");
    fprintf(file_out,"# pidfile [game.pid] - Where should the Process ID be written\n");
    fprintf(file_out,"pidfile=%s\n", game.pidfile);
    fprintf(file_out,"\n");
    fprintf(file_out,"# bindip [0.0.0.0] - What IP should server be bound to (0.0.0.0 means all)\n");
    fprintf(file_out,"bindip=%s\n", game.bindip);
    fprintf(file_out,"\n");
    fprintf(file_out,"# maxchannels [10] - How many channels should be available on server\n");
    fprintf(file_out,"maxchannels=%d\n", game.maxchannels);
    fprintf(file_out,"\n");
    fprintf(file_out,"# timeout_ingame [60] - How many seconds of no activity during a game before timeout occurs\n");
    fprintf(file_out,"timeout_ingame=%d\n", game.timeout_ingame);
    fprintf(file_out,"\n");
    fprintf(file_out,"# timeout_outgame [1200] - How many seconds of no activity out of game before timeout occurs\n");
    fprintf(file_out,"timeout_outgame=%d\n", game.timeout_outgame);
    fprintf(file_out,"\n");
    fprintf(file_out,"# verbose [4] - How verbose the logs should be. 0=critical, 10=noisy\n");
    fprintf(file_out,"verbose=%d\n", game.verbose);
    fprintf(file_out,"\n\n");

    fprintf(file_out,"######## DEFAULT SETTINGS ################\n");
    fprintf(file_out,"# Settings under here will be used by default in\n");
    fprintf(file_out,"# any new channels created.\n");
    fprintf(file_out,"\n");
    fprintf(file_out,"# serverannounce [1] - Server Announces winners?\n");
    fprintf(file_out,"serverannounce=%d\n", game.serverannounce);
    fprintf(file_out,"\n");
    fprintf(file_out,"# pingintercept [1] - Intercept pings in game messages?\n");
    fprintf(file_out,"pingintercept=%d\n", game.pingintercept);
    fprintf(file_out,"\n");
    fprintf(file_out,"# stripcolour [1] - Strip colour from game messages?\n");
    fprintf(file_out,"stripcolour=%d\n", game.stripcolour);
    fprintf(file_out,"\n");
    fprintf(file_out,"# starting_level [1] - What level each player starts at\n");
    fprintf(file_out,"starting_level=%d\n", game.starting_level);
    fprintf(file_out,"\n");
    fprintf(file_out,"# lines_per_level [2] - How many lines to make before player level increases\n");
    fprintf(file_out,"lines_per_level=%d\n", game.lines_per_level);
    fprintf(file_out,"\n");
    fprintf(file_out,"# level_increase [1] - Number of levels to increase each time\n");
    fprintf(file_out,"level_increase=%d\n", game.level_increase);
    fprintf(file_out,"\n");
    fprintf(file_out,"# lines_per_special [1] - Lines to make to get a special block\n");
    fprintf(file_out,"lines_per_special=%d\n", game.lines_per_special);
    fprintf(file_out,"\n");
    fprintf(file_out,"# special_added [1] - Number of special blocks added each time\n");
    fprintf(file_out,"special_added=%d\n", game.special_added);
    fprintf(file_out,"\n");
    fprintf(file_out,"# special_capacity [18] - Capacity of Special block inventory\n");
    fprintf(file_out,"special_capacity=%d\n", game.special_capacity);
    fprintf(file_out,"\n");
    fprintf(file_out,"# classic_rules [1] - Play by classic rules?\n");
    fprintf(file_out,"classic_rules=%d\n", game.classic_rules);
    fprintf(file_out,"\n");
    fprintf(file_out,"# average_levels [1] - Average together all player's game level?\n");
    fprintf(file_out,"average_levels=%d\n", game.average_levels);
    fprintf(file_out,"\n");
    fprintf(file_out,"# sd_timeout [0] - Sudden death timeout. After this many secs, server will add lines. 0=disable\n");
    fprintf(file_out,"sd_timeout=%d\n",game.sd_timeout);
    fprintf(file_out,"\n");
    fprintf(file_out,"# sd_lines_per_add [1] - Number of lines server adds each time\n");
    fprintf(file_out,"sd_lines_per_add=%d\n",game.sd_lines_per_add);
    fprintf(file_out,"\n");
    fprintf(file_out,"# sd_secs_between_lines [30] - Number of secs to wait between adding each line\n");
    fprintf(file_out,"sd_secs_between_lines=%d\n",game.sd_secs_between_lines);
    fprintf(file_out,"\n");
    fprintf(file_out,"#sd_message [Time's up! It's SUDDEN DEATH MODE!] - Message to display when suddendeath triggers\n");
    fprintf(file_out,"sd_message=%s\n",game.sd_message);
    fprintf(file_out,"\n");
    fprintf(file_out,"# command_xxxxx rules. These set permissions to /commands in the partline\n");
    fprintf(file_out,"#    For all, the following apply:\n");
    fprintf(file_out,"#           0 = Disable command for anyone\n");
    fprintf(file_out,"#           1 = Enable anyone to use command\n");
    fprintf(file_out,"#           2 = Enable command only for people who are chanop or better\n");
    fprintf(file_out,"#           3 = Enable command ONLY for authenticated ops (/op or /admin)\n");
    fprintf(file_out,"#    Special Case:\n");
    fprintf(file_out,"#    join   4 = Can join other channels. Can't create new channel (unless authop)\n");
    fprintf(file_out,"#    set    4 = Chanop can only modify settings of NON-preset channels (unless authop)\n");
    fprintf(file_out,"#\n");
    fprintf(file_out,"command_help=%d\n", game.command_help);
    fprintf(file_out,"command_clear=%d\n", game.command_clear);
    fprintf(file_out,"command_kick=%d\n", game.command_kick);
    fprintf(file_out,"command_msg=%d\n", game.command_msg);
    fprintf(file_out,"command_op=%d\n", game.command_op);
    fprintf(file_out,"command_list=%d\n", game.command_list);
    fprintf(file_out,"command_join=%d\n", game.command_join);
    fprintf(file_out,"command_who=%d\n", game.command_who);
    fprintf(file_out,"command_whois=%d\n", game.command_whois);
    fprintf(file_out,"command_topic=%d\n", game.command_topic);
    fprintf(file_out,"command_priority=%d\n", game.command_priority);
    fprintf(file_out,"command_move=%d\n", game.command_move);
    fprintf(file_out,"command_winlist=%d\n", game.command_winlist);
    fprintf(file_out,"command_motd=%d\n", game.command_motd);
    fprintf(file_out,"command_set=%d\n", game.command_set);
    fprintf(file_out,"command_persistant=%d\n", game.command_persistant);
    fprintf(file_out,"command_save=%d\n", game.command_save);
    fprintf(file_out,"command_reset=%d\n", game.command_reset);
    fprintf(file_out,"command_ban=%d\n", game.command_ban);
    fprintf(file_out,"command_banlist=%d\n", game.command_banlist);
    fprintf(file_out,"\n");
    fprintf(file_out,"# winlist_export_txt [1] - Export the winlist to a plain-text CSV file (game.winlist.csv)?\n");
    fprintf(file_out,"winlist_export_txt=%d\n", game.winlist_export_txt);
    fprintf(file_out,"\n");
    fprintf(file_out,"# main_channel_name [Lobby] - Base name of the room(s) kicked players are\n");
    fprintf(file_out,"# redirected to. If full (or if it's the room they're being kicked FROM),\n");
    fprintf(file_out,"# variants Lobby1, Lobby2, ... are used/created automatically.\n");
    fprintf(file_out,"main_channel_name=%s\n", game.main_channel_name);
    fprintf(file_out,"\n\n");
    fprintf(file_out,"# BLOCK OCCURANCY [Percentage value 0-100]. Must add up to 100\n");
    fprintf(file_out,"block_leftl=%d\n", game.block_leftl);
    fprintf(file_out,"block_leftz=%d\n", game.block_leftz);
    fprintf(file_out,"block_square=%d\n", game.block_square);
    fprintf(file_out,"block_rightl=%d\n", game.block_rightl);
    fprintf(file_out,"block_rightz=%d\n", game.block_rightz);
    fprintf(file_out,"block_halfcross=%d\n", game.block_halfcross);
    fprintf(file_out,"block_line=%d\n", game.block_line);
    fprintf(file_out,"\n\n");
    fprintf(file_out,"# SPECIAL BLOCK OCCURANCY [Percentage value 0-100]. Must add up to 100\n");
    fprintf(file_out,"special_addline=%d\n", game.special_addline);
    fprintf(file_out,"special_clearline=%d\n", game.special_clearline);
    fprintf(file_out,"special_nukefield=%d\n", game.special_nukefield);
    fprintf(file_out,"special_randomclear=%d\n", game.special_randomclear);
    fprintf(file_out,"special_switchfield=%d\n", game.special_switchfield);
    fprintf(file_out,"special_clearspecial=%d\n", game.special_clearspecial);
    fprintf(file_out,"special_gravity=%d\n", game.special_gravity);
    fprintf(file_out,"special_quakefield=%d\n", game.special_quakefield);
    fprintf(file_out,"special_blockbomb=%d\n", game.special_blockbomb);
    fprintf(file_out,"\n\n");
    fprintf(file_out,"################ CUSTOM CHANNELS ###########\n");
    fprintf(file_out,"# Below exists (if any) definitions of preset non-removable\n"); 
    fprintf(file_out,"# channels. They exist in the following form:\n");
    fprintf(file_out,"#  [CHANNELNAME]  # Note NO # in front of name.\n");
    fprintf(file_out,"#  maxplayers=6   # Number of players allowed in (6max)\n");
    fprintf(file_out,"#  topic=My Topic # The channel Topic\n");
    fprintf(file_out,"#  priority=50    # Priority of channel\n");
    fprintf(file_out,"#  block_halfcross=12 #etc... any of the default options here\n");
    fprintf(file_out,"#\n");
    /* Now write any persistant channel info */
    chan=chanlist;
    while (chan!=NULL)
      {
        if (chan->persistant)
          {/* Found one. Write it */
            fprintf(file_out,"[%s]\n",chan->name);
            fprintf(file_out,"maxplayers=%d\n",chan->maxplayers);
            fprintf(file_out,"topic=%s\n",chan->description);
            fprintf(file_out,"priority=%d\n",chan->priority);
            
            if (chan->starting_level!=game.starting_level) fprintf(file_out,"starting_level=%d\n",chan->starting_level);
            if (chan->lines_per_level!=game.lines_per_level) fprintf(file_out,"lines_per_level=%d\n",chan->lines_per_level);
            if (chan->level_increase!=game.level_increase) fprintf(file_out,"level_increase=%d\n",chan->level_increase);
            if (chan->lines_per_special!=game.lines_per_special) fprintf(file_out,"lines_per_special=%d\n",chan->lines_per_special);
            if (chan->special_added!=game.special_added) fprintf(file_out,"special_added=%d\n",chan->special_added);
            if (chan->special_capacity!=game.special_capacity) fprintf(file_out,"special_capacity=%d\n",chan->special_capacity);
            if (chan->classic_rules!=game.classic_rules) fprintf(file_out,"classic_rules=%d\n",chan->classic_rules);
            if (chan->average_levels!=game.average_levels) fprintf(file_out,"average_levels=%d\n",chan->average_levels);
            if (chan->sd_timeout!=game.sd_timeout) fprintf(file_out,"sd_timeout=%d\n",chan->sd_timeout);
            if (chan->sd_lines_per_add!=game.sd_lines_per_add) fprintf(file_out,"sd_lines_per_add=%d\n",chan->sd_lines_per_add);
            if (chan->sd_secs_between_lines!=game.sd_secs_between_lines) fprintf(file_out,"sd_secs_between_lines=%d\n",chan->sd_secs_between_lines);
            if (strcasecmp(chan->sd_message,game.sd_message)) fprintf(file_out,"sd_message=%s\n",chan->sd_message);
            fprintf(file_out,"block_leftl=%d\n",chan->block_leftl);
            fprintf(file_out,"block_leftz=%d\n",chan->block_leftz);
            fprintf(file_out,"block_square=%d\n",chan->block_square);
            fprintf(file_out,"block_rightl=%d\n",chan->block_rightl);
            fprintf(file_out,"block_rightz=%d\n",chan->block_rightz);
            fprintf(file_out,"block_halfcross=%d\n",chan->block_halfcross);
            fprintf(file_out,"block_line=%d\n",chan->block_line);
            fprintf(file_out,"special_addline=%d\n",chan->special_addline);
            fprintf(file_out,"special_clearline=%d\n",chan->special_clearline);
            fprintf(file_out,"special_nukefield=%d\n",chan->special_nukefield);
            fprintf(file_out,"special_randomclear=%d\n",chan->special_randomclear);
            fprintf(file_out,"special_switchfield=%d\n",chan->special_switchfield);
            fprintf(file_out,"special_clearspecial=%d\n",chan->special_clearspecial);
            fprintf(file_out,"special_gravity=%d\n",chan->special_gravity);
            fprintf(file_out,"special_quakefield=%d\n",chan->special_quakefield);
            fprintf(file_out,"special_blockbomb=%d\n",chan->special_blockbomb);
            if (chan->stripcolour!=game.stripcolour) fprintf(file_out,"stripcolour=%d\n",chan->stripcolour);
            if (chan->serverannounce!=game.serverannounce) fprintf(file_out,"serverannounce=%d\n",chan->serverannounce);
            if (chan->pingintercept!=game.pingintercept) fprintf(file_out,"pingintercept=%d\n",chan->pingintercept);
            fprintf(file_out,"\n");
          }
        chan=chan->next;
      }
    
    fprintf(file_out,"# End of File\n");
    
    fclose(file_out);
    
    lvprintf(3,"Wrote new game configuration to %s\n", FILE_CONF);
    return(0);
  }

/* gameread() */
/*   Reads from a text format game.conf file, data into the game structure */
int gameread(void)
  { /* Read data from game.conf into structure game */
    FILE *file_in;
    char buf[513];
    char id_tag[81];
    char id_value[81];
    int i,j,error;
    struct channel_t *chan;
    
    chan=NULL;
    
    file_in = fopen(FILE_CONF, "r");
    if (file_in == NULL)
      return(-1);
      
    while(!feof(file_in))
      {
        if(fscanf(file_in," %512[^\n]\n", buf) != 1)
        {
          printf("Error: Failed to read config file: %s.\n",FILE_CONF);
        }        
        i=0; j=strlen(buf);
        while( (i<j) && (buf[i]!='#') ) i++;
        if (buf[i]=='#') buf[i] = '\0'; /* Truncate string to # char */
        
        j=strlen(buf)-1;
        while(buf[j]==' ')
          {
            buf[j]='\0';       /* Strip trailing spaces */
            j--;
          }
          
        j=strlen(buf);
        if (j != 0)
          {
            error = 1;
            
            /* First, is it a new channel decleration? */
            if ( (buf[0]=='[') && (buf[strlen(buf)-1]==']') )
              {/* Yep it is */
                /* Does the channel exist? */
                sscanf(buf,"[%60[^]\n]",id_tag);
                chan=chanlist;
                while ( (chan!=NULL) && (strcasecmp(chan->name,id_tag)) )
                  chan=chan->next;
                  
                if (chan==NULL)
                  { /* New channel */
                    chan=chanlist;
                    chanlist=malloc(sizeof(struct channel_t));
                    chanlist->next=chan;
                    chan=chanlist;

                    chan->maxplayers=DEFAULTMAXPLAYERS;
                    chan->status=STATE_ONLINE;
                    chan->description[0]=0;
                    chan->priority=DEFAULTPRIORITY;
                    chan->sd_mode=SD_NONE;
                    chan->persistant=1;
                    strcpy(chan->name,id_tag);
                                
                    /* Copy default settings */
                    chan->starting_level=game.starting_level;
                    chan->lines_per_level=game.lines_per_level;
                    chan->level_increase=game.level_increase;
                    chan->lines_per_special=game.lines_per_special;
                    chan->special_added=game.special_added;
                    chan->special_capacity=game.special_capacity;
                    chan->classic_rules=game.classic_rules;
                    chan->average_levels=game.average_levels;
                    chan->sd_timeout=game.sd_timeout;
                    chan->sd_lines_per_add=game.sd_lines_per_add;
                    chan->sd_secs_between_lines=game.sd_secs_between_lines;
                    strcpy(chan->sd_message,game.sd_message);
                    chan->block_leftl=game.block_leftl;
                    chan->block_leftz=game.block_leftz;
                    chan->block_square=game.block_square;
                    chan->block_rightl=game.block_rightl;
                    chan->block_rightz=game.block_rightz;
                    chan->block_halfcross=game.block_halfcross;
                    chan->block_line=game.block_line;
                    chan->special_addline=game.special_addline;
                    chan->special_clearline=game.special_clearline;
                    chan->special_nukefield=game.special_nukefield;
                    chan->special_randomclear=game.special_randomclear;
                    chan->special_switchfield=game.special_switchfield;
                    chan->special_clearspecial=game.special_clearspecial;
                    chan->special_gravity=game.special_gravity;
                    chan->special_quakefield=game.special_quakefield;
                    chan->special_blockbomb=game.special_blockbomb;
                    chan->stripcolour=game.stripcolour;
                    chan->serverannounce=game.serverannounce;
                    chan->pingintercept=game.pingintercept;
                  }
              }
            else
            {
            
            id_tag[0]=0;
            id_value[0]=0;
            sscanf(buf,"%80[^= ] = %80[^\n]", id_tag, id_value);
            /* Yuk bit */
            if (!strcasecmp(id_tag,"maxplayers"))
              {
                if (chan!=NULL)
                  chan->maxplayers=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"priority"))
              {
                if (chan!=NULL)
                  chan->priority=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"topic"))
              {
                if (chan!=NULL)
                  {
                    strncpy(chan->description, id_value, DESCRIPTIONLEN-1); chan->description[DESCRIPTIONLEN-1]=0;
                  }
                error=0;
              }
            
            if (!strcasecmp(id_tag,"pidfile"))
              {
                strncpy(game.pidfile, id_value, PIDFILELEN-1); game.pidfile[PIDFILELEN-1]=0;
                error=0;
              }
            if (!strcasecmp(id_tag,"bindip"))
              {
                strncpy(game.bindip, id_value, IPLEN-1); game.bindip[IPLEN-1]=0;
                error=0;
              }
            
              
            if (!strcasecmp(id_tag,"maxchannels"))
              {
                game.maxchannels=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"starting_level"))
              {
                if (chan==NULL)
                  game.starting_level=atoi(id_value);
                else
                  chan->starting_level=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"lines_per_level"))
              {
                if (chan==NULL)
                  game.lines_per_level=atoi(id_value);
                else
                  chan->lines_per_level=atoi(id_value);
                error=0;
              }
            
            if (!strcasecmp(id_tag,"level_increase"))
              {
                if (chan==NULL)
                  game.level_increase=atoi(id_value);
                else
                  chan->level_increase=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"lines_per_special"))
              {
                if (chan==NULL)
                  game.lines_per_special=atoi(id_value);
                else
                  chan->lines_per_special=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"special_added"))
              {
                if (chan==NULL)
                  game.special_added=atoi(id_value);
                else
                  chan->special_added=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"special_capacity"))
              {
                if (chan==NULL)
                  game.special_capacity=atoi(id_value);
                else
                  chan->special_capacity=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"classic_rules"))
              {
                if (chan==NULL)
                  game.classic_rules=atoi(id_value);
                else
                  chan->classic_rules=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"average_levels"))
              {
                if (chan==NULL)
                  game.average_levels=atoi(id_value);
                else
                  chan->average_levels=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"sd_timeout"))
              {
                if (chan==NULL)
                  game.sd_timeout=atoi(id_value);
                else 
                  chan->sd_timeout=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"sd_lines_per_add"))
              {
                if (chan==NULL)
                  game.sd_lines_per_add=atoi(id_value);
                else
                  chan->sd_lines_per_add=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"sd_secs_between_lines"))
              {
                if (chan==NULL)
                  game.sd_secs_between_lines=atoi(id_value);
                else
                  chan->sd_secs_between_lines=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"sd_message"))
              {
                if (chan==NULL)
                  {
                    strncpy(game.sd_message, id_value, SDMSGLEN-1); game.sd_message[SDMSGLEN-1]=0;
                  }
                else
                  {
                    strncpy(chan->sd_message, id_value, SDMSGLEN-1); chan->sd_message[SDMSGLEN-1]=0;
                  }
                error=0;
              }
            
            if (!strcasecmp(id_tag,"command_clear"))
              {
                game.command_clear=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_kick"))
              {
                game.command_kick=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_msg"))
              {
                game.command_msg=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_op"))
              {
                game.command_op=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_list"))
              {
                game.command_list=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_join"))
              {
                game.command_join=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_persistant"))
              {
                game.command_persistant=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_save"))
              {
                game.command_save=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_reset"))
              {
                game.command_reset=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_who"))
              {
                game.command_who=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_whois"))
              {
                game.command_whois=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_ban"))
              {
                game.command_ban=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_banlist"))
              {
                game.command_banlist=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"winlist_export_txt"))
              {
                game.winlist_export_txt=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"main_channel_name"))
              {
                strncpy(game.main_channel_name, id_value, CHANLEN-1); game.main_channel_name[CHANLEN-1]=0;
                error=0;
              }
            if (!strcasecmp(id_tag,"command_topic"))
              {
                game.command_topic=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_priority"))
              {
                game.command_priority=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_move"))
              {
                game.command_move=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_winlist"))
              {
                game.command_winlist=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_motd"))
              {
                game.command_motd=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_set"))
              {
                game.command_set=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"command_help"))
              {
                game.command_help=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"serverannounce"))
              {
                if (chan==NULL)
                  game.serverannounce=atoi(id_value);
                else
                  chan->serverannounce=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"pingintercept"))
              {
                if (chan==NULL)
                  game.pingintercept=atoi(id_value);
                else
                  chan->pingintercept=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"stripcolour"))
              {
                if (chan==NULL)
                  game.stripcolour=atoi(id_value);
                else
                  chan->stripcolour=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"timeout_ingame"))
              {
                game.timeout_ingame=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"timeout_outgame"))
              {
                game.timeout_outgame=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"verbose"))
              {
                game.verbose=atoi(id_value);
                error=0;
              }
            
            if (!strcasecmp(id_tag,"block_leftl"))
              {
                if (chan==NULL)
                  game.block_leftl=atoi(id_value);
                else
                  chan->block_leftl=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"block_leftz"))
              {
                if (chan==NULL)
                  game.block_leftz=atoi(id_value);
                else
                  chan->block_leftz=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"block_square"))
              {
                if (chan==NULL)
                  game.block_square=atoi(id_value);
                else
                  chan->block_square=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"block_rightl"))
              {
                if (chan==NULL)
                  game.block_rightl=atoi(id_value);
                else
                  chan->block_rightl=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"block_rightz"))
              {
                if (chan==NULL)
                  game.block_rightz=atoi(id_value);
                else
                  chan->block_rightz=atoi(id_value);  
                error=0;
              }
            if (!strcasecmp(id_tag,"block_halfcross"))
              {
                if (chan==NULL)
                  game.block_halfcross=atoi(id_value);
                else
                  chan->block_halfcross=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"block_line"))
              {
                if (chan==NULL)
                  game.block_line=atoi(id_value);
                else
                  chan->block_line=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"special_addline"))
              {
                if (chan==NULL)
                  game.special_addline=atoi(id_value);
                else
                  chan->special_addline=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"special_clearline"))
              {
                if (chan==NULL)
                  game.special_clearline=atoi(id_value);
                else
                  chan->special_clearline=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"special_nukefield"))
              {
                if (chan==NULL)
                  game.special_nukefield=atoi(id_value);
                else
                  chan->special_nukefield=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"special_switchfield"))
              {
                if (chan==NULL)
                  game.special_switchfield=atoi(id_value);
                else
                  chan->special_switchfield=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"special_clearspecial"))
              {
                if (chan==NULL)
                  game.special_clearspecial=atoi(id_value);
                else
                  chan->special_clearspecial=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"special_randomclear"))
              {
                if (chan==NULL)
                  game.special_randomclear=atoi(id_value);
                else
                  chan->special_randomclear=atoi(id_value); 
                error=0;
              }
            if (!strcasecmp(id_tag,"special_gravity"))
              {
                if (chan==NULL)
                  game.special_gravity=atoi(id_value);
                else
                  chan->special_gravity=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"special_quakefield"))
              {
                if (chan==NULL)
                  game.special_quakefield=atoi(id_value);
                else
                  chan->special_quakefield=atoi(id_value);
                error=0;
              }
            if (!strcasecmp(id_tag,"special_blockbomb"))
              {
                if (chan==NULL)
                  game.special_blockbomb=atoi(id_value);
                else
                  chan->special_blockbomb=atoi(id_value); 

                error=0;
              }
              
            if (error==1)
              {

                lvprintf(2,"%s: Unknown Identifier: %s\n", FILE_CONF, buf);
              }
            }
          }
      }
    fclose(file_in);
    lvprintf(3,"Read game configuration from %s\n", FILE_CONF);
    return(0);
  }

/* init_game() */
/*   Reset the game structure to default values, then try read the game data */
/*   If game.conf does not exist, create new with defaults, otherwise do some */
/*   sanity checks on the read in data */
void init_game(void)
  { /* Initialise game parameters */
  
    strncpy(game.pidfile, FILE_PID, PIDFILELEN-1); game.pidfile[PIDFILELEN-1]=0;
    strncpy(game.bindip, "0.0.0.0", IPLEN-1); game.bindip[IPLEN-1]=0;
    game.maxchannels=10;
    game.starting_level = 1;
    game.lines_per_level = 2;
    game.level_increase = 1;
    game.lines_per_special =1;
    game.special_added=1;
    game.special_capacity=18;
    game.classic_rules=1;
    game.average_levels=1;
    game.sd_timeout=0;
    game.sd_lines_per_add=1;
    game.sd_secs_between_lines=30;
    strncpy(game.sd_message,"Time's up! It's SUDDEN DEATH MODE!", SDMSGLEN-1);game.sd_message[SDMSGLEN-1]=0;
    game.command_clear=3;
    game.command_kick=2;
    game.command_msg=1;
    game.command_op=1;
    game.command_list=1;
    game.command_join=1;
    game.command_who=1;
    game.command_whois=1;
    game.command_topic=2;
    game.command_priority=3;
    game.command_move=2;
    game.command_winlist=1;
    game.command_motd=1;
    game.command_help=1;
    game.command_set=4;
    game.command_persistant=3;
    game.command_save=3;
    game.command_reset=3;
    game.command_ban=3;
    game.command_banlist=3;
    game.winlist_export_txt=1;
    strncpy(game.main_channel_name,"Lobby", CHANLEN-1); game.main_channel_name[CHANLEN-1]=0;
    game.serverannounce=1;
    game.pingintercept=1;
    game.stripcolour=1;
    game.timeout_ingame=60;
    game.timeout_outgame=1200;
    game.verbose=4;

    game.block_leftl=14;
    game.block_leftz=14;
    game.block_square=15;
    game.block_rightl=14;
    game.block_rightz=14;
    game.block_halfcross=14;
    game.block_line=15;
    game.special_addline=32;
    game.special_clearline=18;
    game.special_nukefield=1;
    game.special_randomclear=11;
    game.special_switchfield=3;
    game.special_clearspecial=14;
    game.special_gravity=1;
    game.special_quakefield=6;
    game.special_blockbomb=14;
    /* First, see if game.conf exists */
    if (gameread() == -1)
      { /* File does not exist, so lets create it */
        lvprintf(4,"No game definitions found. Creating game.conf file with defaults.\n");
        if (gamewrite() == -1)
          fatal("Can't write game.conf. Check permissions!",0);
      }
  
    if ((game.block_leftl+game.block_leftz+game.block_square+game.block_rightl
       +game.block_rightz+game.block_halfcross+game.block_line) != 100)
       {
         fatal("Block percentages MUST add up to 100%", 0);
       }
  
    if ((game.special_addline+game.special_clearline
        +game.special_nukefield+game.special_randomclear
        +game.special_switchfield+game.special_clearspecial
        +game.special_gravity+game.special_quakefield+game.special_blockbomb) != 100)
        {
          fatal("Special percentages MUST add up to 100%", 0);
        }
      
    if ( (game.starting_level < 1) || (game.starting_level > 100) )
      fatal("Starting level must be in the range 1 to 100",0);
    
    if ( (game.lines_per_level < 1) || (game.lines_per_level > 100) )
      fatal("Lines per level must be in the range 1 to 100",0);
    
    if ( (game.level_increase < 0) || (game.level_increase > 50) )
      fatal("Level increase must be in the range 0 to 50", 0);
    
    if ( (game.lines_per_special < 1) || (game.lines_per_special > 50) )
      fatal("Lines per special must be in the range 1 to 50", 0);
    
    if ( (game.special_added < 0) || (game.special_added > 50) )
      fatal("Specials added must be in the range 0 to 50", 0);
    
    if ( (game.special_capacity < 0) || (game.special_capacity > 18) )
      fatal("Special capacity must be in the range 0 to 18", 0);
    
    if ( (game.classic_rules < 0) || (game.classic_rules > 1) )
      fatal("Classic rules must be either 0 or 1",0);
    
    if ( (game.average_levels < 0) || (game.average_levels > 1) )
      fatal("Average player levels must be either 0 or 1", 0);
      
/*    if (gamewrite() == -1)
          fatal("Can't write game.conf. Check permissions!",0);*/
  }
  
  
/* Initialise Winlist structure, to all empty */
void init_winlist(void)
  {
    int i;
    for(i=0;i<MAXWINLIST;i++)
      {
        winlist[i].score=0;
        winlist[i].inuse=0;
      }
  }  

/* Read Winlist structure from game.winlist */
void readwinlist(void)
  {
    int i;
    int valid;
    FILE *file_in;
    
    file_in = fopen(FILE_WINLIST,"r");

    if (file_in == NULL) return;
    
    for(i=0;i<MAXWINLIST;i++)
      {
        if(fread(&winlist[i], sizeof(struct winlist_t), 1, file_in) != 1)
        {
          printf("Error: Failed to read winlist file: %s.\n",FILE_WINLIST);
        }
      }
    fclose(file_in);
    
    i=0;
    valid=1;
    while ( (i<MAXWINLIST) && (winlist[i].inuse) && (valid))
      {
        valid = (winlist[i].score > 0);
        valid = valid && ( (winlist[i].status=='p') || (winlist[i].status=='t') );
        valid = valid && (strlen(winlist[i].name) <= NICKLEN);
        i++;
      }
      
    if (!valid)
      { /* Screwed up winlist */
        lvprintf(3,"Invalid Winlist - Resetting to 0!\n");
        init_winlist();
      }
    
  }
  
/* Write the winlist out to game.winlist*/
void writewinlist(void)
  {
    int i;
    FILE *file_out;
    
    file_out = fopen(FILE_WINLIST, "w");
    
    if (file_out == NULL) return;
    
    for(i=0;i<MAXWINLIST;i++)
      {
        fwrite(&winlist[i], sizeof(struct winlist_t), 1, file_out);
      }
    fclose(file_out);

    /* Keep the plain-text CSV export (and its extended, victory-only
       metrics) in sync every time the winlist itself is written -- this
       runs automatically at every call site (end of game, /clear, server
       shutdown) without needing to remember to call it separately. */
    writewinlisttxt();
  }

/* --------------------------------------------------------------------- */
/* Extended (victory-only) winlist metrics: wins, last win date, best     */
/* level reached, average level reached. Entirely separate from the       */
/* original winlist_t/game.winlist above -- the in-game/protocol-facing   */
/* winlist (struct winlist_t, /winlist command, TetriNET client display)  */
/* is never affected by any of this. These metrics are only ever surfaced */
/* through the plain-text CSV export (writewinlisttxt(), below).         */
/* --------------------------------------------------------------------- */

/* init_winliststats() - Clears the in-memory extended stats */
void init_winliststats(void)
  {
    int i;
    for (i=0; i<MAXWINLISTSTATS; i++)
      winliststats[i].inuse = 0;
  }

/* readwinliststats() - Reads game.winliststats (same binary-array style */
/*   as game.winlist) into winliststats[] */
void readwinliststats(void)
  {
    int i;
    FILE *file_in;

    file_in = fopen(FILE_WINLISTSTATS,"r");
    if (file_in == NULL) return;

    for (i=0; i<MAXWINLISTSTATS; i++)
      {
        if (fread(&winliststats[i], sizeof(struct winliststats_t), 1, file_in) != 1)
          {
            /* Short/missing file (e.g. first run after upgrading) -- stop here,
               remaining slots stay cleared from init_winliststats() */
            break;
          }
      }
    fclose(file_in);
  }

/* writewinliststats() - Writes winliststats[] out to game.winliststats */
void writewinliststats(void)
  {
    int i;
    FILE *file_out;

    file_out = fopen(FILE_WINLISTSTATS, "w");
    if (file_out == NULL) return;

    for (i=0; i<MAXWINLISTSTATS; i++)
      fwrite(&winliststats[i], sizeof(struct winliststats_t), 1, file_out);

    fclose(file_out);
  }

/* find_winliststats(name, status) - Returns the index of the matching */
/*   entry (same name+status key as winlist_t), or -1 if not found */
int find_winliststats(char *name, char status)
  {
    int i;
    for (i=0; i<MAXWINLISTSTATS; i++)
      if ( winliststats[i].inuse && (winliststats[i].status==status) && !strcasecmp(winliststats[i].name,name) )
        return i;
    return -1;
  }

/* updatewinliststats(name, status, level_reached) - Records one more */
/*   victory for this name/status: bumps the win count, updates the last */
/*   win timestamp, tracks the best level reached, and accumulates the */
/*   level sum (used to compute the average level at export time). */
/*   Called alongside updatewinlist() at the exact same call sites (end of */
/*   game, when a winner is declared) -- never instead of it. */
void updatewinliststats(char *name, char status, int level_reached)
  {
    int i;

    i = find_winliststats(name, status);
    if (i == -1)
      {
        for (i=0; (i<MAXWINLISTSTATS) && winliststats[i].inuse; i++)
          ; /* find first free slot */
        if (i == MAXWINLISTSTATS)
          {
            lvprintf(1,"WARNING: Winlist stats table full (MAXWINLISTSTATS=%d), could not track stats for '%s'\n", MAXWINLISTSTATS, name);
            return;
          }
        winliststats[i].inuse = 1;
        strncpy(winliststats[i].name, name, NICKLEN); winliststats[i].name[NICKLEN]=0;
        winliststats[i].status = status;
        winliststats[i].wins = 0;
        winliststats[i].best_level = 0;
        winliststats[i].level_sum = 0;
      }

    winliststats[i].wins++;
    winliststats[i].last_win = time(NULL);
    winliststats[i].level_sum += level_reached;
    if (level_reached > winliststats[i].best_level)
      winliststats[i].best_level = level_reached;

    writewinliststats();
  }

/* writewinlisttxt() - Exports the winlist (+ extended metrics where */
/*   available) to a plain-text CSV file (FILE_WINLIST_TXT), e.g. for a */
/*   webpage to display. Gated by game.winlist_export_txt (game.conf). */
/*   NOTE: the in-game winlist itself (struct winlist_t, /winlist command, */
/*   TetriNET client display) is completely unaffected -- this only ever */
/*   produces an additional, separate export file. */
void writewinlisttxt(void)
  {
    FILE *file_out;
    int i, j, rank;
    char name_clean[NICKLEN+1];
    char when_str[32];
    struct tm *tm_info;

    if (!game.winlist_export_txt) return;

    file_out = fopen(FILE_WINLIST_TXT, "w");
    if (file_out == NULL) return;

    fprintf(file_out, "rank,type,name,score,wins,last_win,best_level,avg_level\n");

    rank = 1;
    for (i=0; i<MAXWINLIST; i++)
      {
        if (winlist[i].inuse)
          {
            strip_colour_codes(winlist[i].name, name_clean);
            j = find_winliststats(name_clean, winlist[i].status);

            if ( (j>=0) && (winliststats[j].wins>0) )
              {
                tm_info = localtime(&winliststats[j].last_win);
                strftime(when_str, sizeof(when_str), "%Y-%m-%d %H:%M:%S", tm_info);
                fprintf(file_out, "%d,%s,\"%s\",%lu,%lu,%s,%d,%.2f\n",
                        rank,
                        (winlist[i].status=='t' ? "team" : "player"),
                        name_clean,
                        winlist[i].score,
                        winliststats[j].wins,
                        when_str,
                        winliststats[j].best_level,
                        (double)winliststats[j].level_sum / (double)winliststats[j].wins);
              }
            else
              {
                /* No extended stats yet for this entry (e.g. it predates this
                   feature, or hasn't won again since upgrading) -- leave the
                   extra columns blank rather than guessing. */
                fprintf(file_out, "%d,%s,\"%s\",%lu,,,,\n",
                        rank, (winlist[i].status=='t' ? "team" : "player"), name_clean, winlist[i].score);
              }
            rank++;
          }
      }

    fclose(file_out);
  }
  
/* updatewinlist(team/player name, team or player?, score to add) */
/*   Scans the winlist for this team or players name (second parameter hold */
/*   either a p or t for player or team respecively). If found, adds the score */
/*   to that entry's score, otherwise creates a new entry with the score */
/*   Then, does a quick sort on the winlist to ensure that it's in decending order */

void updatewinlist(char *name, char status, int score)
  { /* This would have been SOOO much easier with linked list array. Oh well */
    int i,j,found;
    struct winlist_t rec_winlist;
    
    /* First, try find if the name already exists */
    i=0;found=0;
    while ( (i<MAXWINLIST) && (winlist[i].inuse) && (!found))
      {
        if ( (!strcasecmp(winlist[i].name,name)) && (winlist[i].status==status)) 
          found=1;
        else i++;
      }
    if (found)
      {
        winlist[i].score+=score; /* Just add the score */
      }
    else
      { /* New entry. Bit harder. Have to determine where it should be.*/
        
        /* i contains location of last winlist entry */
        /* so put record here, and bubble it up to its location */
        if (i == MAXWINLIST )
          { /* Heck, winlist is full, check see if we can even add to list */
            i=MAXWINLIST-1;
          }
        if (winlist[i].score < score)
          {            
            winlist[i].score = score;
            name[NICKLEN]=0;
            strcpy(winlist[i].name, name);
            winlist[i].status = status;
            winlist[i].inuse = 1;
          }
      }
    
    /* And bubble it up */
    j=i;
    while( (j>0) && ( (!winlist[j-1].inuse) || (winlist[j-1].score < winlist[i].score) ))
      { /* Yep, it's higher than its parent so swap em */
        j--;
        rec_winlist = winlist[j];
        winlist[j]=winlist[i];
        winlist[i]=rec_winlist;
        i=j;
      } 
    
    
  }
  
  
/* strip_colour_codes(src, dest) - Copies src into dest, dropping all */
/*   TetriNET colour-code control characters. dest must be large enough */
/*   (same length as src is always sufficient, since this only removes */
/*   characters). Extracted from the logic sendwinlist() always used to */
/*   strip colour before showing a name in the TetriNET client's winlist; */
/*   also reused by the plain-text CSV export (writewinlisttxt()). */
void strip_colour_codes(char *src, char *dest)
  {
    char *P;
    int k;

    P = src;
    k = 0;
    while (*P != 0)
      {
        if(  (*P != BOLD)
          && (*P != ITALIC)
          && (*P != UNDERLINE)
          && (*P != BLACK)
          && (*P != DARKGRAY)
          && (*P != SILVER)
          && (*P != NAVY)
          && (*P != BLUE)
          && (*P != CYAN)
          && (*P != GREEN)
          && (*P != NEON)
          && (*P != TEAL)
          && (*P != BROWN)
          && (*P != RED)
          && (*P != MAGENTA)
          && (*P != VIOLET)
          && (*P != YELLOW)
          && (*P != WHITE)
          )
            {
              dest[k]=*P;
              k++;
            }
        P++;
      }
    dest[k]=0;
  }

/* Send winlist top10 to playernum (-1 = all) */
void sendwinlist(struct channel_t *chan,struct net_t *n)
  {
    int j;
    char name[NICKLEN+1];
    struct net_t *nsock;
    
    if (n==NULL)
      nsock=chan->net;
    else
      nsock=n;
       
    do
      {
        if ( ( (nsock->type == NET_CONNECTED)|| (nsock->type == NET_WAITINGFORTEAM)) )
          {
            tprintf(nsock->sock,"winlist");
            for(j=0;j<10;j++)
              {
                if (winlist[j].inuse)
                  {
                    /* TetriNET client does NOT like colour in the beginning of the name */
                    strip_colour_codes(winlist[j].name, name);
                    tprintf(nsock->sock, " %c%s;%lu", winlist[j].status, name, winlist[j].score);
                  }
              } 
            tprintf(nsock->sock,"\xff");
          }
        nsock=nsock->next;
      } while ( (n==NULL) && (nsock!=NULL) );
      /* BUGFIX: condition was (n!=NULL), which made the "send to all" case
         (n==NULL) stop after a single iteration, so only the first player
         in the channel linked list ever received the updated winlist. */
  }
  
  
/* Parse a field string, and update our internal knowledge of that player's field */
void parsefield(struct net_t *n, char *buf)
  {
    char *P;
    int blocktype,x,y;
    int bailout;
    
    
    /* First, check size of buf. If its FIELD_MAXX*FIELD_MAXY length, exactly, AND */
    /* its first char is not a block type, then it's a field dump! */
    if (strlen(buf)==(FIELD_MAXX*FIELD_MAXY))
      {
        if ( (buf[0] > '/') || (buf[0] < '!') )
          {
            P=buf;
            for(y=0;y<FIELD_MAXY;y++)
              for(x=0;x<FIELD_MAXX;x++)
                {

                  blocktype = *P;
                  P++;
                  n->field[x][y]=blocktype-'0';
                }
          }
      }
    else
      {
        P=buf;
        bailout=0;
        while ( !bailout && (*P != '\xff') && (*P != '\0') )
          {
            /* Read in type of block char */
            blocktype = *P;
        
            /* Sanity checking */
            if ( (blocktype > '/') || (blocktype < '!') ) 
              bailout=1;
            else
              {
                P++;
                blocktype-='!';
                while ( !bailout && (*P != '\xff') && (*P != '\0') && (*P >= '3') && (*P <= '>') )
                  {
                    x=*P;
                    P++;
                    y=*P;
                    P++;
                
                    /* Sanity checking again */
                    if ( (x >= '3') && (x <= '>') && (y >= '3') && (y <= 'H') )
                      { /* We have a co-ordinate. We have a block type. Set it! */
                        x-='3'; y-='3';
                        n->field[x][y] = blocktype;
                      }
                    else
                      bailout=1;
                  }
              }
          } 
        if (bailout) 
          { /* This should not happen!!! */

            lvprintf(2,"Slot(%s): Bailout error on field update!!",  n->nick);
          }
      }
        
  }
  
/* Send to sock_index_to, the field of sock_index_from */
void sendfield(struct net_t *ns_to, struct net_t *ns_from)
  {
    int x,y;
    char block;
    const char special[9] =
      {'a','c','n','r','s','b','g','q','o'};

    tprintf(ns_to->sock,"f %d ", ns_from->gameslot);
    for(y=0;y<FIELD_MAXY;y++)
      for(x=0;x<FIELD_MAXX;x++)
        {
          block = (ns_from->field[x][y]+'0'); 
          if ( (block >= '6') && (block <= '>') )
            { /* Special block, so break rules and put the letter instead */
              block = special[block-'0'-6];
            }
          tprintf(ns_to->sock,"%c", block);
        }
    tprintf(ns_to->sock,"\xff");
  }


