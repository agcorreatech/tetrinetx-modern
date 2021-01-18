/*
  game.c
  
*/

/* securitywrite() */
/*   Writes out the security structure into a text format game.secure file */
int securitywrite()
  {
    FILE *file_out;
    
    file_out = fopen(FILE_SECURE, "w");
    if (file_out == NULL)
      return(-1);
      
    fprintf(file_out,"# TetriNET (linux) Security configuration file\n");
    fprintf(file_out,"\n");
    fprintf(file_out,"# This file contains security configuration for TetriNET, and by default will\n");
    fprintf(file_out,"# be created with all values commented out. \n");
    fprintf(file_out,"# Each configuration value consists of a TAG name, followed by an equal sign\n");
    fprintf(file_out,"# and a value. IE:\n");
    fprintf(file_out,"#      op_password=mypass\n");
    fprintf(file_out,"# Any text after a # is ignored, and can be used as comments.\n");
    fprintf(file_out,"\n");
    fprintf(file_out,"# op_password [] - Typing /op <thispassword> will give player op status\n");
    fprintf(file_out,"#op_password=pass4word\n");
    fprintf(file_out,"\n\n");
    fprintf(file_out,"# End of File\n");   
    
    fclose(file_out);
    
    lvprintf(3,"Wrote new security configuration to %s\n", FILE_SECURE);
    return(0);
  }

/* securityread() */
/*   Reads from a text format game.secure file, data into the security structure */
int securityread(void)
  { 
  
    FILE *file_in;
    char buf[513];
    char id_tag[81];
    char id_value[81];
    int i,j,error;
    
    security.op_password[0]=0;
    file_in = fopen(FILE_SECURE, "r");
    if (file_in == NULL)
      return(-1);
      
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
        while(buf[j]==' ')
          {
            buf[j]='\0';       /* Strip trailing spaces */
            j--;
          }
          
        j=strlen(buf);
        if (j != 0)
          {
            error = 1;
            sscanf(buf,"%80[^= ] = %80s", id_tag, id_value);
            /* Yuk bit */
            if (!strcasecmp(id_tag,"op_password"))
              {
                strncpy(security.op_password, id_value, PASSLEN-1); security.op_password[PASSLEN-1]=0;
                error=0;
              }
            
            if (error==1)
              {

                lvprintf(2,"%s: Unknown Identifier: %s\n", FILE_SECURE, buf);
              }
          }
      }
    fclose(file_in);
    lvprintf(3,"Read security configuration from %s\n", FILE_SECURE);
    return(0);
  }


/* init_security() */
/*   Initialises the security structure */
void init_security(void)
  {
    security.op_password[0]=0; /* Clear password, which disables the /op command */
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
    fprintf(file_out,"#           3 = Enable command ONLY for authenticated ops (/op)\n");
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
    fprintf(file_out,"command_topic=%d\n", game.command_topic);
    fprintf(file_out,"command_priority=%d\n", game.command_priority);
    fprintf(file_out,"command_move=%d\n", game.command_move);
    fprintf(file_out,"command_winlist=%d\n", game.command_winlist);
    fprintf(file_out,"command_set=%d\n", game.command_set);
    fprintf(file_out,"command_persistant=%d\n", game.command_persistant);
    fprintf(file_out,"command_save=%d\n", game.command_save);
    fprintf(file_out,"command_reset=%d\n", game.command_reset);
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
    game.maxchannels=1;
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
    game.command_topic=2;
    game.command_priority=2;
    game.command_move=2;
    game.command_winlist=1;
    game.command_help=1;
    game.command_set=4;
    game.command_persistant=3;
    game.command_save=3;
    game.command_reset=3;
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
  
  
/* Send winlist top10 to playernum (-1 = all) */
void sendwinlist(struct channel_t *chan,struct net_t *n)
  {
    int j,k;
    char name[NICKLEN];
    char *P;
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
                    P=winlist[j].name;
                    k=0;
                    name[0]=0;
                    while( *P != 0)
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
                              name[k]=*P;
                              k++;
                            }
                        P++;
                      }
                    name[k]=0;
                            
                  
                    tprintf(nsock->sock, " %c%s;%lu", winlist[j].status, name, winlist[j].score);
                  }
              } 
            tprintf(nsock->sock,"\xff");
          }
        nsock=nsock->next;
      } while ( (n!=NULL) && (nsock!=NULL) );
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


