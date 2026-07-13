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
    fprintf(file_out,"# Typing \"/op <password>\" in the partyline authenticates as the admin\n");
    fprintf(file_out,"# whose [nickname] block matches the nickname you are CURRENTLY connected\n");
    fprintf(file_out,"# with. There is no separate username to type: your TetriNET nickname IS\n");
    fprintf(file_out,"# the username. Both must match -- knowing a password is not enough, you\n");
    fprintf(file_out,"# also have to be connected under that exact nickname (nicknames are\n");
    fprintf(file_out,"# unique server-wide and can't be changed mid-session).\n");
    fprintf(file_out,"#\n");
    fprintf(file_out,"# Passwords are matched case-sensitively and are capped at %d characters\n", PASSLEN-1);
    fprintf(file_out,"# (longer values are silently truncated).\n");
    fprintf(file_out,"#\n");
    fprintf(file_out,"# A fresh install starts with the DEFAULT account below: [admin] with\n");
    fprintf(file_out,"# password \"tetrinetx\". CHANGE IT -- both the account name and the\n");
    fprintf(file_out,"# password -- before exposing the server. While the default is present,\n");
    fprintf(file_out,"# the server logs a warning at every startup.\n");
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

/* is_admin_nick(nick) - Returns 1 if this nickname has a registered admin
   account in game.secure. Used to "restrict" those nicknames: a player
   connecting under one must authenticate with /op within
   RESTRICTED_NICK_AUTH_SECS or be disconnected (see check_timeouts()). */
char is_admin_nick(char *nick)
  {
    int i;

    for (i=0; i<MAXADMINS; i++)
      if ( security.adminlist[i].inuse && !strcasecmp(security.adminlist[i].nick, nick) )
        return 1;
    return 0;
  }

/* set_admin_password(nick, newpass) - Changes the password of the admin
   account matching this nickname (and ONLY that one -- used by /password,
   where an authenticated admin may change their own password but nobody
   else's). Returns 1 on success, 0 if no such account exists. The caller
   is responsible for persisting with securitywrite(). */
char set_admin_password(char *nick, char *newpass)
  {
    int i;

    for (i=0; i<MAXADMINS; i++)
      if ( security.adminlist[i].inuse && !strcasecmp(security.adminlist[i].nick, nick) )
        {
          strncpy(security.adminlist[i].password, newpass, PASSLEN-1);
          security.adminlist[i].password[PASSLEN-1]=0;
          return 1;
        }
    return 0;
  }

/* security_seed_default_admin() - Seeds the built-in default admin account
   ([admin] with password "tetrinetx"), used when game.secure doesn't exist
   yet so a fresh install has a working admin login out of the box. */
void security_seed_default_admin(void)
  {
    int slot;

    slot = find_or_add_admin_slot("admin");
    if (slot >= 0)
      {
        strncpy(security.adminlist[slot].password, "tetrinetx", PASSLEN-1);
        security.adminlist[slot].password[PASSLEN-1]=0;
      }
  }

/* security_warn_default_admin() - Logs a loud warning if the well-known
   default admin account ([admin] / "tetrinetx") is still active, urging the
   owner to change both the account name and the password. Returns 1 if the
   default was found (so the boot sequence can also echo it to stdout). */
char security_warn_default_admin(void)
  {
    int i;

    for (i=0; i<MAXADMINS; i++)
      {
        if ( security.adminlist[i].inuse
             && !strcasecmp(security.adminlist[i].nick,"admin")
             && !strcmp(security.adminlist[i].password,"tetrinetx") )
          {
            lvprintf(0,"WARNING: %s still contains the DEFAULT admin account ([admin] with password \"tetrinetx\").\n", FILE_SECURE);
            lvprintf(0,"WARNING: Anyone who knows this default can take over the server. Edit %s and change BOTH the account name and the password.\n", FILE_SECURE);
            return 1;
          }
      }
    return 0;
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
        /* BUGFIX: stop on a failed read instead of re-processing the stale
           buf from the previous iteration (see the same fix in gameread). */
        if(fscanf(file_in," %512[^\n]\n", buf) != 1)
          break;

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
                    safe_strcpy(security.adminlist[cur_admin].password, PASSLEN, id_value);
                    error=0;
                  }

                /* Legacy format: a bare op_password=X tag, outside any block */
                if (!strcasecmp(id_tag,"op_password"))
                  {
                    int legacy_slot = find_or_add_admin_slot("admin");
                    if (legacy_slot >= 0)
                      {
                        safe_strcpy(security.adminlist[legacy_slot].password, PASSLEN, id_value);
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
        lvprintf(1,"%s: Migrated legacy 'op_password' to an admin account named 'admin' -- edit %s to rename it and/or add more admins. You must now be connected with the nickname 'admin' to use /op.\n", FILE_SECURE, FILE_SECURE);
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
        /* BUGFIX: this used to do nothing on a failed read, so the last
           parsed line was re-processed one extra time (a stray "[BAN]" tail
           could even allocate a spurious empty ban). Stop the loop instead. */
        if (fscanf(file_in," %512[^\n]\n", buf) != 1)
          break;

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
                  safe_strcpy(banlist[cur_ban].target, UHOSTLEN+1, id_value);
                else if (!strcasecmp(id_tag,"date"))
                  banlist[cur_ban].when = (time_t)atol(id_value);
                else if (!strcasecmp(id_tag,"admin"))
                  safe_strcpy(banlist[cur_ban].admin, NICKLEN+1, id_value);
                else if (!strcasecmp(id_tag,"reason"))
                  safe_strcpy(banlist[cur_ban].reason, BANREASONLEN+1, id_value);
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

/* kick_cooldown_secs_left(nick, channel_name) - Seconds until this */
/*   nickname may rejoin this room; 0 if no active cooldown. Used to */
/*   tell the player exactly how long is left instead of a vague */
/*   "a few minutes". */
int kick_cooldown_secs_left(char *nick, char *channel_name)
  {
    int i;
    time_t now = time(NULL);

    for (i=0; i<MAXKICKCOOLDOWNS; i++)
      {
        if ( kick_cooldowns[i].inuse
             && (kick_cooldowns[i].expires > now)
             && !strcasecmp(kick_cooldowns[i].nick,nick)
             && !strcasecmp(kick_cooldowns[i].channel_name,channel_name) )
          return (int)(kick_cooldowns[i].expires - now);
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
    fprintf(file_out,"# maxchannels [99] - How many channels should be available on server\n");
    fprintf(file_out,"maxchannels=%d\n", game.maxchannels);
    fprintf(file_out,"\n");
    fprintf(file_out,"# timeout_ingame [60] - How many seconds of no activity during a game before timeout occurs\n");
    fprintf(file_out,"timeout_ingame=%d\n", game.timeout_ingame);
    fprintf(file_out,"\n");
    fprintf(file_out,"# timeout_outgame [600] - How many seconds of no activity out of game before the player is disconnected for inactivity (applies to everyone, chanops included)\n");
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
    fprintf(file_out,"#           3 = Enable command ONLY for authenticated ops (/op)\n");
    fprintf(file_out,"#    Special Case:\n");
    fprintf(file_out,"#    join   4 = Can join other channels. Can't create new channel (unless authop)\n");
    fprintf(file_out,"#    set    4 = Chanop can only modify settings of NON-preset channels (unless authop)\n");
    fprintf(file_out,"#    kick   An authenticated admin (/op) can ALWAYS use /kick,\n");
    fprintf(file_out,"#           regardless of this setting -- including 0 (disabled for everyone\n");
    fprintf(file_out,"#           else). This is intentional: /kick doubles as a moderation tool.\n");
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
    fprintf(file_out,"# main_channel_name [lobby] - Base name of the server's lobby room(s):\n");
    fprintf(file_out,"# where players land when they connect, and where kicked players are\n");
    fprintf(file_out,"# redirected to. If full (or if it's the room they're being kicked FROM),\n");
    fprintf(file_out,"# variants lobby1, lobby2, ... are used/created automatically.\n");
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
    fprintf(file_out,"#  [CHANNELNAME]  # Note NO # in front of name. Max 10 characters\n");
    fprintf(file_out,"#                 # (longer wraps the client's /list line; truncated on read).\n");
    fprintf(file_out,"#  maxplayers=6   # Number of players allowed in (6max)\n");
    fprintf(file_out,"#  topic=My Topic # The channel Topic. Max 22 characters (same reason).\n");
    fprintf(file_out,"#  description=.. # Longer text describing how this room's game works.\n");
    fprintf(file_out,"#                 # Sent to a player every time they enter the room\n");
    fprintf(file_out,"#                 # (NOT shown in /list). Empty/absent = no message.\n");
    fprintf(file_out,"#  priority=50    # Ordering weight shown in /list. New connections always\n");
    fprintf(file_out,"#                 # land in a lobby room (main_channel_name, lobby1, ...),\n");
    fprintf(file_out,"#                 # so priority does NOT steer connect placement.\n");
    fprintf(file_out,"#  own_winlist=1  # What the channel's games score on (see /ownwinlist):\n");
    fprintf(file_out,"#                 # 0 = the global winlist (default), 1 = the channel's OWN\n");
    fprintf(file_out,"#                 # winlist (file game.winlist.<name>), 2 = NO winlist at all\n");
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
            if (chan->chan_desc[0]) fprintf(file_out,"description=%s\n",chan->chan_desc);
            fprintf(file_out,"priority=%d\n",chan->priority);
            if (chan->own_winlist) fprintf(file_out,"own_winlist=%d\n",chan->own_winlist);
            
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
    char id_value[CHANDESCLEN+1];	/* big enough for a description= value; every other key is strncpy-capped anyway */
    int i,j,error;
    struct channel_t *chan;
    
    chan=NULL;
    
    file_in = fopen(FILE_CONF, "r");
    if (file_in == NULL)
      return(-1);
      
    while(!feof(file_in))
      {
        /* BUGFIX: on a failed read (EOF or an unparsable tail) this used to
           only print an error and fall through, re-processing whatever stale
           content was still in buf from the previous iteration. Stop the
           loop instead -- the last real line has already been handled. */
        if(fscanf(file_in," %512[^\n]\n", buf) != 1)
          break;
        /* Strip a trailing '\r' (game.conf saved/edited with CRLF line
           endings, e.g. on Windows): %[^\n] stops at '\n' but happily
           includes a preceding '\r' as a normal character. Left alone,
           that '\r' ends up stuck onto whatever value follows an '='
           (pidfile, bindip, topic, ...), which is invisible in most
           displays but corrupts filenames/behaviour built from it --
           e.g. pidfile becoming "game.pid\r", a different filename from
           "game.pid" as far as the filesystem is concerned. */
        j=strlen(buf);
        if (j>0 && buf[j-1]=='\r') buf[j-1]='\0';
        i=0; j=strlen(buf);
        while( (i<j) && (buf[i]!='#') ) i++;
        if (buf[i]=='#') buf[i] = '\0'; /* Truncate string to # char */

        /* BUGFIX: added the (j>=0) guard. Without it, a line that is empty
           after comment/CR stripping makes j = -1 and this reads/writes
           buf[-1] (out-of-bounds). The sibling readers securityread() and
           readbanlist() already had this guard; gameread() didn't. */
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
            
            /* First, is it a new channel decleration? */
            if ( (buf[0]=='[') && (buf[strlen(buf)-1]==']') )
              {/* Yep it is */
                /* Does the channel exist? */
                sscanf(buf,"[%60[^]\n]",id_tag);
                /* Enforce the same name limit /join enforces on creation
                   (longer names overflow chan->name and wrap the client's
                   /list line) */
                if (strlen(id_tag) > CHANNAMELIMIT)
                  {
                    id_tag[CHANNAMELIMIT]=0;
                    lvprintf(1,"WARNING: channel name in %s longer than %d characters -- truncated to [%s]\n", FILE_CONF, CHANNAMELIMIT, id_tag);
                  }
                chan=chanlist;
                while ( (chan!=NULL) && (strcasecmp(chan->name,id_tag)) )
                  chan=chan->next;
                  
                if (chan==NULL)
                  { /* New channel */
                    chan=chanlist;
                    chanlist=malloc(sizeof(struct channel_t));
                    chanlist->next=chan;
                    chan=chanlist;

                    /* BUGFIX: chan->net was never initialised here, so preset
                       channels read from game.conf carried a garbage player
                       list pointer -- the first connection to touch them
                       (numplayers() walks chan->net) crashed the server. */
                    chan->net=NULL;

                    chan->maxplayers=DEFAULTMAXPLAYERS;
                    chan->status=STATE_ONLINE;
                    chan->description[0]=0;
                    chan->chan_desc[0]=0;
                    chan->own_winlist=WINLIST_GLOBAL;
                    init_winlist_array(chan->winlist);
                    init_winliststats_array(chan->winliststats);
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
            sscanf(buf,"%80[^= ] = %256[^\n]", id_tag, id_value);
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
                    if (strlen(id_value) > TOPICLIMIT)
                      {
                        id_value[TOPICLIMIT]=0;
                        lvprintf(1,"WARNING: topic of channel [%s] longer than %d characters -- truncated\n", chan->name, TOPICLIMIT);
                      }
                    strncpy(chan->description, id_value, DESCRIPTIONLEN-1); chan->description[DESCRIPTIONLEN-1]=0;
                  }
                error=0;
              }
            if (!strcasecmp(id_tag,"description"))
              {
                if (chan!=NULL)
                  {
                    strncpy(chan->chan_desc, id_value, CHANDESCLEN-1); chan->chan_desc[CHANDESCLEN-1]=0;
                  }
                error=0;
              }
            if (!strcasecmp(id_tag,"own_winlist"))
              {
                if (chan!=NULL)
                  {
                    chan->own_winlist=atoi(id_value);
                    if ( (chan->own_winlist < WINLIST_GLOBAL) || (chan->own_winlist > WINLIST_NONE) )
                      chan->own_winlist=WINLIST_GLOBAL;
                    if (chan->own_winlist==WINLIST_OWN)
                      read_channel_winlist(chan);
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
                if (strlen(id_value) > CHANNAMELIMIT)
                  {
                    id_value[CHANNAMELIMIT]=0;
                    lvprintf(1,"WARNING: main_channel_name longer than %d characters -- truncated to %s\n", CHANNAMELIMIT, id_value);
                  }
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
/* create_channel() is defined later in main.c (unity build: game.c is
   #included before main.c's own functions) -- forward declaration so
   create_default_channels() below can use it. */
struct channel_t *create_channel(char *name, char persistant);

/* create_default_channels() - Seeds the default rooms used when
   game.conf defines no [channel] blocks at all. Ten game-mode presets
   (the table below); each one becomes BOTH a 6-player room and a
   2-player "1x1" twin, 20 rooms total:
     - the first preset is the lobby (game.main_channel_name, priority 1:
       where new connections land; variants lobby1, lobby2, ... get
       priorities 2, 3, ...) and its twin is #tetris1x1 (standard rules)
     - the other 6-player rooms get priorities 30, 31, ...
     - the 1x1 twins get priorities 50, 51, ... in the same order
   The lobby, #classic and #pure score on the GLOBAL winlist; every other
   room (including ALL 1x1 twins) keeps its own. (priority only orders
   the /list display.) All are persistent presets, so they are written
   into game.conf. Each preset also carries the channel description text
   shown to a player on room entry (see announce_channel_description()). */

struct channel_preset_t {
  char *name;			/* 6-player room; NULL = game.main_channel_name (the lobby) */
  char *name1x1;		/* its 2-player twin */
  char *topic;			/* 6p topic; the twin gets "<topic> 1x1" unless topic1x1 is set */
  char *topic1x1;
  char *chan_desc;		/* "Channel description:" text, shared by both rooms */
  int priority;			/* 6p priority (twins get 50, 51, ... in table order) */
  char global_winlist;		/* 1 = the 6p room scores on the global winlist (twins never do) */
  /* Ruleset overrides; -1 = keep the game.conf global default */
  int starting_level, lines_per_level, level_increase;
  int lines_per_special, special_added, classic_rules;
  int sd_timeout, sd_secs_between_lines;
  int blocks[7];		/* leftl,leftz,square,rightl,rightz,halfcross,line ([0]==-1 = defaults) */
  int specials[9];		/* addline,clearline,nukefield,randomclear,switchfield,
				   clearspecial,gravity,quakefield,blockbomb ([0]==-1 = defaults) */
};

static const struct channel_preset_t default_presets[] = {
  { NULL, "tetris1x1", "Server Lobby", "Standard TetriNET 1x1",
    "Standard TetriNET: all specials enabled; clearing 2+ lines also sends lines to your opponents.",
    1, 1,   -1,-1,-1,  -1,-1,-1,  -1,-1,  {-1}, {-1} },
  { "classic", "classic1x1", "Classic Tetris", NULL,
    "Classic Tetris battle: NO specials. Clearing 2/3/4 lines sends 1/2/4 lines to your opponents.",
    30, 1,  -1,-1,-1,  999,0,1,   -1,-1,  {-1}, {-1} },
  { "pure", "pure1x1", "Pure Tetris", NULL,
    "Pure Tetris: no specials, no line attacks. Speed and clean stacking decide who survives.",
    31, 1,  -1,-1,-1,  999,0,0,   -1,-1,  {-1}, {-1} },
  { "speed", "speed1x1", "High Speed", NULL,
    "High speed: games start at level 10 and accelerate every line. Short, intense matches.",
    32, 0,  10,1,2,    -1,-1,-1,  -1,-1,  {-1}, {-1} },
  { "sudden", "sudden1x1", "Sudden Death", NULL,
    "Sudden death: 2 minutes in, the server starts adding a line to every field every 30 seconds.",
    33, 0,  -1,-1,-1,  -1,-1,-1,  120,-1, {-1}, {-1} },
  { "rush", "rush1x1", "Sudden Rush", NULL,
    "Aggressive sudden death: after just 1 minute the server adds a line every 10 seconds. Fast games, guaranteed.",
    34, 0,  -1,-1,-1,  -1,-1,-1,  60,10,  {-1}, {-1} },
  { "lines", "lines1x1", "Lines Only", NULL,
    "Line warfare: the only specials are Add Line (A) and Clear Line (C) - attack opponents or clean your own field.",
    35, 0,  -1,-1,-1,  -1,-1,-1,  -1,-1,  {-1}, {50,50,0,0,0,0,0,0,0} },
  { "nolines", "nolines1x1", "No I-Piece", NULL,
    "The I-piece never drops: no 4-line Tetris. Survive on the other six pieces.",
    36, 0,  -1,-1,-1,  -1,-1,-1,  -1,-1,  {17,17,16,17,17,16,0}, {-1} },
  { "bomb", "bomb1x1", "Block Bomb Only", NULL,
    "The only special is Block Bomb (O): detonate it to scatter blocks across your target's field.",
    37, 0,  -1,-1,-1,  -1,-1,-1,  -1,-1,  {-1}, {0,0,0,0,0,0,0,0,100} },
  { "chaos", "chaos1x1", "Special Chaos", NULL,
    "Special chaos: every special drops with equal chance and each cleared line yields 3 of them. Anything can happen.",
    38, 0,  -1,-1,-1,  1,3,-1,    -1,-1,  {-1}, {12,11,11,11,11,11,11,11,11} },
};

/* Applies one preset row to a freshly create_channel()'d room (which
   already carries the game.conf global defaults). */
static void apply_channel_preset(struct channel_t *chan, const struct channel_preset_t *p,
                                 char *topic, int priority, int maxplayers, char own_winlist)
  {
    strncpy(chan->description, topic, DESCRIPTIONLEN-1); chan->description[DESCRIPTIONLEN-1]=0;
    strncpy(chan->chan_desc, p->chan_desc, CHANDESCLEN-1); chan->chan_desc[CHANDESCLEN-1]=0;
    chan->priority=priority;
    chan->maxplayers=maxplayers;
    chan->own_winlist=own_winlist;
    if (own_winlist==WINLIST_OWN)
      read_channel_winlist(chan);

    if (p->starting_level!=-1) chan->starting_level=p->starting_level;
    if (p->lines_per_level!=-1) chan->lines_per_level=p->lines_per_level;
    if (p->level_increase!=-1) chan->level_increase=p->level_increase;
    if (p->lines_per_special!=-1) chan->lines_per_special=p->lines_per_special;
    if (p->special_added!=-1) chan->special_added=p->special_added;
    if (p->classic_rules!=-1) chan->classic_rules=p->classic_rules;
    if (p->sd_timeout!=-1) chan->sd_timeout=p->sd_timeout;
    if (p->sd_secs_between_lines!=-1) chan->sd_secs_between_lines=p->sd_secs_between_lines;

    if (p->blocks[0]!=-1)
      {
        chan->block_leftl=p->blocks[0];
        chan->block_leftz=p->blocks[1];
        chan->block_square=p->blocks[2];
        chan->block_rightl=p->blocks[3];
        chan->block_rightz=p->blocks[4];
        chan->block_halfcross=p->blocks[5];
        chan->block_line=p->blocks[6];
      }
    if (p->specials[0]!=-1)
      {
        chan->special_addline=p->specials[0];
        chan->special_clearline=p->specials[1];
        chan->special_nukefield=p->specials[2];
        chan->special_randomclear=p->specials[3];
        chan->special_switchfield=p->specials[4];
        chan->special_clearspecial=p->specials[5];
        chan->special_gravity=p->specials[6];
        chan->special_quakefield=p->specials[7];
        chan->special_blockbomb=p->specials[8];
      }
  }

void create_default_channels(void)
  {
    struct channel_t *chan;
    char topic1x1[DESCRIPTIONLEN+8];
    int i, count;

    count = sizeof(default_presets)/sizeof(default_presets[0]);
    for (i=0; i<count; i++)
      {
        const struct channel_preset_t *p = &default_presets[i];

        chan = create_channel(p->name!=NULL ? p->name : game.main_channel_name, 1);
        if (chan != NULL)
          apply_channel_preset(chan, p, p->topic, p->priority, DEFAULTMAXPLAYERS,
                               p->global_winlist ? WINLIST_GLOBAL : WINLIST_OWN);

        chan = create_channel(p->name1x1, 1);
        if (chan != NULL)
          {
            if (p->topic1x1!=NULL)
              snprintf(topic1x1, sizeof(topic1x1), "%s", p->topic1x1);
            else
              snprintf(topic1x1, sizeof(topic1x1), "%s 1x1", p->topic);
            apply_channel_preset(chan, p, topic1x1, 50+i, 2, WINLIST_OWN);
          }
      }

    lvprintf(1,"No channels configured -- created the %d default rooms (#%s + game modes and their 1x1 twins)\n", count*2, game.main_channel_name);
  }

void init_game(void)
  { /* Initialise game parameters */
  
    strncpy(game.pidfile, FILE_PID, PIDFILELEN-1); game.pidfile[PIDFILELEN-1]=0;
    strncpy(game.bindip, "0.0.0.0", IPLEN-1); game.bindip[IPLEN-1]=0;
    game.maxchannels=99;
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
    strncpy(game.main_channel_name,"lobby", CHANLEN-1); game.main_channel_name[CHANLEN-1]=0;
    game.serverannounce=1;
    game.pingintercept=1;
    game.stripcolour=1;
    game.timeout_ingame=60;
    game.timeout_outgame=600;	/* 10 minutes of inactivity out of game */
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
        create_default_channels(); /* before gamewrite(), so the fresh
                                      game.conf already lists them */
        if (gamewrite() == -1)
          fatal("Can't write game.conf. Check permissions!",0);
      }

    /* game.conf existed but defined no [channel] blocks: seed the same
       defaults in memory (the file itself is left untouched). */
    if (chanlist == NULL)
      create_default_channels();
  
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
  
  
/* init_winlist_array(wl) - Clears any MAXWINLIST-sized winlist array
   (the global one or a channel's own) to all-empty */
void init_winlist_array(struct winlist_t *wl)
  {
    int i;
    for(i=0;i<MAXWINLIST;i++)
      {
        wl[i].score=0;
        wl[i].inuse=0;
      }
  }

/* Initialise the GLOBAL Winlist structure, to all empty */
void init_winlist(void)
  {
    init_winlist_array(winlist);
  }

/* channel_winlist_filename(chan, buf, bufsize) - Builds the on-disk
   filename for a channel's own winlist: game.winlist.<name>, with any
   character outside [a-zA-Z0-9_-] in the channel name replaced by '_'
   (channel names come from user input; never let one steer a path). */
void channel_winlist_filename(struct channel_t *chan, char *buf, int bufsize)
  {
    int i,j;
    char c;

    snprintf(buf, bufsize, "%s.", FILE_WINLIST);
    j=(int)strlen(buf);
    for (i=0; (chan->name[i]!=0) && (j<bufsize-1); i++)
      {
        c=chan->name[i];
        if ( !( ((c>='a')&&(c<='z')) || ((c>='A')&&(c<='Z')) || ((c>='0')&&(c<='9')) || (c=='_') || (c=='-') ) )
          c='_';
        buf[j++]=c;
      }
    buf[j]=0;
  }

/* read_channel_winlist(chan) - Loads a channel's own winlist AND its
   extended stats from disk (game.winlist.<name> +
   game.winliststats.<name>) into chan->winlist / chan->winliststats.
   Missing files = fresh empty winlist (first time the channel goes
   own_winlist). */
void read_channel_winlist(struct channel_t *chan)
  {
    int i;
    FILE *file_in;
    char fname[300];

    init_winlist_array(chan->winlist);
    init_winliststats_array(chan->winliststats);

    channel_winlist_filename(chan, fname, sizeof(fname)-16);
    file_in = fopen(fname,"r");
    if (file_in != NULL)
      {
        for(i=0;i<MAXWINLIST;i++)
          {
            if (fread(&chan->winlist[i], sizeof(struct winlist_t), 1, file_in) != 1)
              break;
          }
        fclose(file_in);
      }

    strcat(fname, ".stats");
    file_in = fopen(fname,"r");
    if (file_in != NULL)
      {
        for(i=0;i<MAXWINLISTSTATS;i++)
          {
            if (fread(&chan->winliststats[i], sizeof(struct winliststats_t), 1, file_in) != 1)
              break;
          }
        fclose(file_in);
      }
  }

/* write_channel_winlist(chan) - Writes a channel's own winlist out to
   game.winlist.<name> (same binary array format as game.winlist), its
   extended stats to game.winlist.<name>.stats, and -- when the CSV
   export is enabled -- the same CSV export the global winlist gets, to
   game.winlist.<name>.csv. */
void write_channel_winlist(struct channel_t *chan)
  {
    int i;
    FILE *file_out;
    char fname[300];

    channel_winlist_filename(chan, fname, sizeof(fname)-16);
    file_out = fopen(fname, "w");
    if (file_out != NULL)
      {
        for(i=0;i<MAXWINLIST;i++)
          fwrite(&chan->winlist[i], sizeof(struct winlist_t), 1, file_out);
        fclose(file_out);
      }

    strcat(fname, ".stats");
    file_out = fopen(fname, "w");
    if (file_out != NULL)
      {
        for(i=0;i<MAXWINLISTSTATS;i++)
          fwrite(&chan->winliststats[i], sizeof(struct winliststats_t), 1, file_out);
        fclose(file_out);
      }

    /* game.winlist.<name>.csv -- same columns as the global export */
    channel_winlist_filename(chan, fname, sizeof(fname)-16);
    strcat(fname, ".csv");
    writewinlisttxt_to(fname, chan->winlist, chan->winliststats);
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

/* init_winliststats_array(stats) - Clears any MAXWINLISTSTATS-sized
   extended-stats array (the global one or a channel's own) */
void init_winliststats_array(struct winliststats_t *stats)
  {
    int i;
    for (i=0; i<MAXWINLISTSTATS; i++)
      stats[i].inuse = 0;
  }

/* init_winliststats() - Clears the GLOBAL in-memory extended stats */
void init_winliststats(void)
  {
    init_winliststats_array(winliststats);
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

/* find_winliststats_in(stats, name, status) - Returns the index of the */
/*   matching entry (same name+status key as winlist_t) in the given */
/*   stats array, or -1 if not found */
int find_winliststats_in(struct winliststats_t *stats, char *name, char status)
  {
    int i;
    for (i=0; i<MAXWINLISTSTATS; i++)
      if ( stats[i].inuse && (stats[i].status==status) && !strcasecmp(stats[i].name,name) )
        return i;
    return -1;
  }

/* find_winliststats(name, status) - Same, on the GLOBAL stats */
int find_winliststats(char *name, char status)
  {
    return find_winliststats_in(winliststats, name, status);
  }

/* updatewinliststats_in(stats, name, status, level_reached) - Records one */
/*   more victory for this name/status in the given stats array: bumps the */
/*   win count, updates the last win timestamp, tracks the best level */
/*   reached, and accumulates the level sum (used to compute the average */
/*   level at export time). Persisting is the CALLER's job. */
void updatewinliststats_in(struct winliststats_t *stats, char *name, char status, int level_reached)
  {
    int i;

    i = find_winliststats_in(stats, name, status);
    if (i == -1)
      {
        for (i=0; (i<MAXWINLISTSTATS) && stats[i].inuse; i++)
          ; /* find first free slot */
        if (i == MAXWINLISTSTATS)
          {
            lvprintf(1,"WARNING: Winlist stats table full (MAXWINLISTSTATS=%d), could not track stats for '%s'\n", MAXWINLISTSTATS, name);
            return;
          }
        stats[i].inuse = 1;
        strncpy(stats[i].name, name, NICKLEN); stats[i].name[NICKLEN]=0;
        stats[i].status = status;
        stats[i].wins = 0;
        stats[i].best_level = 0;
        stats[i].level_sum = 0;
      }

    stats[i].wins++;
    stats[i].last_win = time(NULL);
    stats[i].level_sum += level_reached;
    if (level_reached > stats[i].best_level)
      stats[i].best_level = level_reached;
  }

/* updatewinliststats(name, status, level_reached) - Same, on the GLOBAL */
/*   stats (persisted immediately). Called alongside updatewinlist() at */
/*   the exact same call sites -- never instead of it. */
void updatewinliststats(char *name, char status, int level_reached)
  {
    updatewinliststats_in(winliststats, name, status, level_reached);
    writewinliststats();
  }

/* writewinlisttxt_to(csvname, wl, stats) - Exports the given winlist */
/*   (+ its extended metrics where available) to a plain-text CSV file, */
/*   e.g. for a webpage to display. Gated by game.winlist_export_txt */
/*   (game.conf). Used for the global winlist AND for every channel's own */
/*   winlist (each channel exports to game.winlist.<name>.csv with the */
/*   exact same columns). NOTE: the in-game winlist itself (struct */
/*   winlist_t, /winlist command, TetriNET client display) is completely */
/*   unaffected -- this only ever produces additional, separate files. */
void writewinlisttxt_to(char *csvname, struct winlist_t *wl, struct winliststats_t *stats)
  {
    FILE *file_out;
    int i, j, rank;
    char name_clean[NICKLEN+1];
    char when_str[32];
    struct tm *tm_info;

    if (!game.winlist_export_txt) return;

    file_out = fopen(csvname, "w");
    if (file_out == NULL) return;

    fprintf(file_out, "rank,type,name,score,wins,last_win,best_level,avg_level\n");

    rank = 1;
    for (i=0; i<MAXWINLIST; i++)
      {
        if (wl[i].inuse)
          {
            strip_colour_codes(wl[i].name, name_clean);
            j = find_winliststats_in(stats, name_clean, wl[i].status);

            if ( (j>=0) && (stats[j].wins>0) )
              {
                tm_info = localtime(&stats[j].last_win);
                strftime(when_str, sizeof(when_str), "%Y-%m-%d %H:%M:%S", tm_info);
                fprintf(file_out, "%d,%s,\"%s\",%lu,%lu,%s,%d,%.2f\n",
                        rank,
                        (wl[i].status=='t' ? "team" : "player"),
                        name_clean,
                        wl[i].score,
                        stats[j].wins,
                        when_str,
                        stats[j].best_level,
                        (double)stats[j].level_sum / (double)stats[j].wins);
              }
            else
              {
                /* No extended stats yet for this entry (e.g. it predates this
                   feature, or hasn't won again since upgrading) -- leave the
                   extra columns blank rather than guessing. */
                fprintf(file_out, "%d,%s,\"%s\",%lu,,,,\n",
                        rank, (wl[i].status=='t' ? "team" : "player"), name_clean, wl[i].score);
              }
            rank++;
          }
      }

    fclose(file_out);
  }

/* writewinlisttxt() - Same, for the GLOBAL winlist (game.winlist.csv) */
void writewinlisttxt(void)
  {
    writewinlisttxt_to(FILE_WINLIST_TXT, winlist, winliststats);
  }
  
/* updatewinlist(team/player name, team or player?, score to add) */
/*   Scans the winlist for this team or players name (second parameter hold */
/*   either a p or t for player or team respecively). If found, adds the score */
/*   to that entry's score, otherwise creates a new entry with the score */
/*   Then, does a quick sort on the winlist to ensure that it's in decending order */

/* updatewinlist_in(wl, name, status, score) - Adds score to name's entry
   in the given winlist array (the global one, or a channel's own),
   creating/bubbling the entry as needed. */
void updatewinlist_in(struct winlist_t *wl, char *name, char status, int score)
  { /* This would have been SOOO much easier with linked list array. Oh well */
    int i,j,found;
    struct winlist_t rec_winlist;

    /* First, try find if the name already exists */
    i=0;found=0;
    while ( (i<MAXWINLIST) && (wl[i].inuse) && (!found))
      {
        if ( (!strcasecmp(wl[i].name,name)) && (wl[i].status==status))
          found=1;
        else i++;
      }
    if (found)
      {
        wl[i].score+=score; /* Just add the score */
      }
    else
      { /* New entry. Bit harder. Have to determine where it should be.*/

        /* i contains location of last winlist entry */
        /* so put record here, and bubble it up to its location */
        if (i == MAXWINLIST )
          { /* Heck, winlist is full, check see if we can even add to list */
            i=MAXWINLIST-1;
          }
        if (wl[i].score < score)
          {
            wl[i].score = score;
            name[NICKLEN]=0;
            strcpy(wl[i].name, name);
            wl[i].status = status;
            wl[i].inuse = 1;
          }
      }

    /* And bubble it up */
    j=i;
    while( (j>0) && ( (!wl[j-1].inuse) || (wl[j-1].score < wl[i].score) ))
      { /* Yep, it's higher than its parent so swap em */
        j--;
        rec_winlist = wl[j];
        wl[j]=wl[i];
        wl[i]=rec_winlist;
        i=j;
      }


  }

/* updatewinlist(name, status, score) - Same, on the GLOBAL winlist */
void updatewinlist(char *name, char status, int score)
  {
    updatewinlist_in(winlist, name, status, score);
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

/* Send winlist top10 to playernum (-1 = all). Uses the channel's
   EFFECTIVE winlist: its own one when own_winlist=WINLIST_OWN, none at
   all (an empty winlist packet, clearing the client's display) for
   WINLIST_NONE, the global one otherwise. */
void sendwinlist(struct channel_t *chan,struct net_t *n)
  {
    int j;
    char name[NICKLEN+1];
    struct net_t *nsock;
    struct winlist_t *wl;

    if (chan->own_winlist==WINLIST_OWN)
      wl = chan->winlist;
    else if (chan->own_winlist==WINLIST_NONE)
      wl = NULL;
    else
      wl = winlist;

    if (n==NULL)
      nsock=chan->net;
    else
      nsock=n;

    do
      {
        if ( ( (nsock->type == NET_CONNECTED)|| (nsock->type == NET_WAITINGFORTEAM)) )
          {
            tprintf(nsock->sock,"winlist");
            for(j=0; (wl!=NULL) && (j<10); j++)
              {
                if (wl[j].inuse)
                  {
                    /* TetriNET client does NOT like colour in the beginning of the name */
                    strip_colour_codes(wl[j].name, name);
                    tprintf(nsock->sock, " %c%s;%lu", wl[j].status, name, wl[j].score);
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


