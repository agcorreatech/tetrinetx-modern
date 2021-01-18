
/* 
   TetriNET Server for Linux                                     V1.13.XX
*************************************************************************
 What is TetriNET?                                                      
   TetriNET is an addictive 6 player tetris game. Would someone please  
   fill this question in with something more interesting ;)


 What does this program do?
   I created this program originally because I wanted to run a TetriNET
   server off my gateway machine, which ran RedHat Linux, and masqueraded
   packets between my internal network and the Internet. And thats where
   my total interest lies. If a new TetriNET comes out, I _may_ try to
   update the linux server.
   What this program does is set up a TetriNET server that ordinary
   TetriNET clients can connect to. It attempts to fix some of the
   "glaring" holes in the TetriNET protocol that I discovered, and which
   I'm sure some people use as cheats, but I now see why it is nearly
   impossible to fix ;), without a modification to the client.
   I've kept the server as close to the same as the original TetriNET
   server, but I've added some extras that I've often wanted, such as
   the "/kick" and "/ban" keywords.
   Please note, this server in no way encompasses the whole game. The clients
   are the ones that do most of the work, with the server just passing suitable
   packets between each client, and of course adding some of it's own.
  
 Acknowledgements
   Naturally I acknowledge the author of the brilliant game TetriNET, 
   St0rmcat. Let me just say that trying to find your homepage was definately
   an experience (one of the three most hardest things on the internet)... and 
   so was reading it ;) I guess it must be interesting to be defining a "protocol" 
   
   I also acknowledge the author(s) of the "EggDrop" irc program, (see net.h).
   The reason is that I borrowed some of their code (really, with minor 
   modifications, net.c is completely theirs). I didn't feel like recreating
   the wheel, and I'd previously gone through that program line by line 
   anyway, and thought it to be one of the most brilliantly neatly written 
   programs I've seen yet.

   Thanks to BlueCoder (see crack.h). His goals follow the same lines as mine,
   and thus was a good source of information. Good luck on your program man,
   and keep in touch with any of your additions to the protocol  ;)
   
   Thanks to those who quite unknowingly tested the Server when it went on live
   trial, on the 14/9/98 and 15/9/98. I just wish I could remember your nicks ;)
   One person I DO remember ;), is ^k1mc1^, good fun to play against, and has
   an excellent team name :)

  Who Am I
    My name is Brendan Grieve, and I tend to program now and then on something
    that interests me. I don't program for a living, but rather live for my
    programming, if you get my meaning. Most of my code is generally open, since
    if even one person learns something from it, GREAT. The best way to learn is
    to look at other code, and to modify it even, to add something in.
    Also, of course, any bugs can be traced right to the exact line, and patched
    easily ;)
    
    email: brg@cheerful.com
    
  What can I do with your Source Code?
    Learn from it if you want. Use it. If you want to improve it, go ahead, and
    if you think your changes will be of use to others, then please do
    email me the changes, and I'll add them to the main distribution.
    Essentially all code, apart from bits (mentiotioned in their respective
    header files), are completely public domain.
    
    What I want at the moment is for this code to be portable to other
    unices than just Linux. And for that, naturally I need someone who
    has other types of unixes to try compile the program.
    
    I now have a WISHLIST text file with all the excellent suggestions
    sent in by various people. They all might one day be added, and if
    you think of a good way, go right ahead ;), and if it's good, I'll add
    it to the main distribution.
    
   
*/
   
   
 
   

/*undef TETDEBUG*/		/* Every time I find I need a printf, I'll encase it as a debug */

#include "config.h"			/* Generally about the only file */
                                        /* that the user can safely change */
#include <signal.h>
#include <unistd.h>
#include "main.h"
#include "net.h"
#include "utils.h"
#include "crack.h"
#include "game.h"

#include "utils.c"
#include "net.c"
#include "crack.c"
#include "game.c"


/* remnet (channel, n) - Removes n from the channel list */
void remnet(struct channel_t *chan, struct net_t *n)
  {
    struct net_t *nsock,*osock;
    
    nsock=chan->net;
    osock=NULL;
    while( (nsock!=NULL) && (nsock!= n))
      {
        osock=nsock;
        nsock=nsock->next;
      }
    if (nsock!=NULL)
      {
        if (osock==NULL)
          chan->net=nsock->next;
        else
          osock->next=nsock->next;
        nsock->channel=NULL;
      }
  }

/* addnet (channel, n) - Adds n to the channel list */
void addnet(struct channel_t *chan, struct net_t *n)
  {
    n->next=chan->net;
    chan->net=n;
    n->channel=chan;
  }

/* writepid() - Writes the current pid to game.pidfile */
void writepid(void)
  {
    FILE *file_out;
    
    file_out = fopen(game.pidfile,"w");
    if (file_out == NULL)
      {
        lvprintf(0,"ERROR: Could not write to PID file %s\n", game.pidfile);
      }
    else
      {
        fprintf(file_out,"%d", getpid());
        fclose(file_out);
        lvprintf(9,"Wrote PID to file %s\n", game.pidfile);
      }
  }

/* delpid() - Delete the pid file */
void delpid(void)
  {
    remove(game.pidfile);
    lvprintf(9,"Removed PID file %s\n", game.pidfile);
  }

/* int numallplayers(chan) - Returns ANY player that is connected in a channel */
int numallplayers(struct channel_t *chan)
  {
    int count;
    struct net_t *n;
    
    count=0;
    n=chan->net;
    while (n!=NULL)
      {
        count++;
        n=n->next;
      }
    return(count);
  }
/* int numplayers(chan)  - Returns number of CONNECTED Players in a channel */
int numplayers(struct channel_t *chan)
  {
    int count;
    struct net_t *n;
    
    count=0;
    n=chan->net;
    while (n!=NULL)
      {
        if( ((n->type==NET_CONNECTED) || (n->type==NET_WAITINGFORTEAM))) count++;
        n=n->next;
      }
    return(count);
  }

/* int numchannels() - Returns number of channels  */
int numchannels(void)
  {
    int count;
    struct channel_t *chan;
    
    chan=chanlist;
    count=0;
    while (chan!= NULL)
      {
        count++;
        chan=chan->next;
      }
    return(count);
    
  }



/* is_banned( net ) - Returns 1 if the IP of the player is banned */
char is_banned(struct net_t *n)
  { /* I should use regex, but I've not used it before, and it was late. Easier to write a quick one of my own */
    FILE *file_in;
    char n1[4], n2[4], n3[4], n4[4];
    char ip1[4], ip2[4], ip3[4], ip4[4];
    int i; int found;
    
    file_in = fopen(FILE_BAN,"r");
    if (file_in == NULL)
      return(0);
      
    sprintf(n1,"%lu", (unsigned long)(n->addr&0xff000000)/(unsigned long)0x1000000);
    sprintf(n2,"%lu", (unsigned long)(n->addr&0x00ff0000)/(unsigned long)0x10000);
    sprintf(n3,"%lu", (unsigned long)(n->addr&0x0000ff00)/(unsigned long)0x100);
    sprintf(n4,"%lu", (unsigned long)n->addr&0x000000ff);
    
    found = 0;
    while (!found && !feof(file_in))
      {  
        /* Read in line by line, and match regular expressions with the IP of sock_index */    
        i = fscanf(file_in," %4[0-9*] . %4[0-9*] . %4[0-9*] . %4[0-9*] \n", ip1, ip2, ip3, ip4);
        if (i == 4)
          {
            if (     ((!strcmp(n1,ip1)) || (!strcmp(ip1,"*")))
                  && ((!strcmp(n2,ip2)) || (!strcmp(ip2,"*")))
                  && ((!strcmp(n3,ip3)) || (!strcmp(ip3,"*")))
                  && ((!strcmp(n4,ip4)) || (!strcmp(ip4,"*")))
               )
              {/* Matches! */
                found=1;
              }                
          }
      }
    fclose(file_in);
    return(found);
  }

/* is_op( net ) - Returns 1 if this player is the lowest gameslot, thus the chanop */
char is_op(struct net_t *n)
  {
    int found;
    struct net_t *nt;
    found = 0;

    nt=n->channel->net;
    while( nt!=NULL)
      {
        if ( ( (nt->type == NET_CONNECTED) || (nt->type == NET_WAITINGFORTEAM) ) && (nt->gameslot < n->gameslot))
          found = 1;
        nt=nt->next;
      }
    return(!found);
  }

/* passed_level(net, level required to pass) - Returns 1 if the player's security */
/*            level is equal to or higher than the level required to pass */
/*            Passing is_op gives player a security level of at least 2*/
char passed_level(struct net_t *n, int passlevel)
  {
    int currlevel;

    currlevel=LEVEL_NORMAL; /* Normal Player */

    if (is_op(n)) currlevel=LEVEL_OP;  /* Op by position */
    
    /* Player might already have a higher level... say they did a /op */
    if (n->securitylevel > currlevel) 
      currlevel=n->securitylevel;
    
    return( currlevel >= passlevel);
  }

/* kick(net from, player to be kicked) - Disconnects player, and informs all others that they were kicked */
void kick(struct net_t *n_from, int kick_gameslot)
  {
    struct net_t *n;
    struct net_t *n_to;
    
    n=n_from->channel->net;
    n_to=NULL;
    while (n!=NULL)
      {
        if ( (n->gameslot == kick_gameslot) && ((n->type == NET_CONNECTED) || (n->type == NET_WAITINGFORTEAM))) 
          n_to = n;
        n=n->next;
      }
    if ((n_to!=NULL) && (kick_gameslot>=1) && (kick_gameslot<=6))
      {
        lvprintf(4,"#%s-%s: Kicked %s from server\n",n_from->channel->name,n_from->nick,n_to->nick);
        n=n_from->channel->net;
        while (n!=NULL)
          {
            if ( ((n->type == NET_CONNECTED) || (n->type == NET_WAITINGFORTEAM))) 
              {
                tprintf(n->sock,"kick %d\xff", kick_gameslot);
              }
            n=n->next;
          }
        killsock(n_to->sock); lostnet(n_to);
      }
  }

/* tet_checkversion( client version ) - Returns 0 if the server supports this */
/*   client version. At the moment, ONLY support 1.13 client*/ 
int tet_checkversion(char *buf)
  { /* Returns -1 if versioncheck fails */
    if (!strcmp(TETVERSION,buf))
      return(0);
    else
      return(-1);
  }

/* lvprintf( priority, same as printf ) - Logs to the log IF priority is smaller */
/*                                        or equal to game.verbose */
// Example: lvprintf(9,"%s: Disconnected due to failed decryption: %s\n",n->host,buf);

void lvprintf(int priority, char *format,...)
{ /* No bounds checking. Be very careful what you log */
  va_list va; static char SBUF2[768];
  va_start(va,format);
  vsprintf(SBUF2,format,va);
  if (strlen(SBUF2)>512)
    {
      SBUF2[512]=0;   /* truncate string. Of course, by now we have buffer overrun ;) */
    }
  
  if (priority <= game.verbose)
    lprintf(SBUF2);

  va_end(va);
}

/* lprintf( same as printf ) - Logs to the log. */
void lprintf(char *format,...)
{ /* No bounds checking. Be very careful what you log */
  va_list va; static char SBUF2[768];
  FILE *file_out;
  char *mytime;
  char *P;
  time_t cur_time;
  va_start(va,format);
  vsprintf(SBUF2,format,va);
  if (strlen(SBUF2)>512)
    {
      SBUF2[512]=0;   /* truncate string. Of course, by now we have buffer overrun ;) */
    }
  
  file_out = fopen(FILE_LOG,"a");
  if (file_out != NULL)
    {
      cur_time = time(NULL);
      mytime=ctime(&cur_time);
      P=mytime;
      P+=4;
      mytime[strlen(mytime)-6]=0;
      fprintf(file_out,"%s %s", P, SBUF2);
      fclose(file_out);
    }
  va_end(va);
}

/* Open a Listening Socket on the TetriNET port */
void init_telnet_port()
{
  int i,j;
  struct net_t *n;
  
  /* find old entry if it exists */
  n=gnet;
  while ( (n!=NULL) && (n->type!=NET_TELNET))
    n=n->next;
    
  if (n==NULL) {
    /* no existing entry */
    n=gnet;
    gnet=malloc(sizeof(struct net_t));
    gnet->next=n;
    n=gnet;
    n->addr=getmyip();
    n->type=NET_TELNET;
    strcpy(n->nick,"(telnet)");
    getmyhostname(n->host);
  }
  else {
    /* already an entry */
    killsock(n->sock);
  }
  j=TELNET_PORT;
  i=open_listen_socket(&j,game.bindip);

  if (i>=0) {
    n->port=j;
    n->sock=i;

    lvprintf(3,"Listening at telnet port %d, on socket %d, bound to %s\n", j,i,game.bindip);
    return;
  }
  printf("Couldn't find telnet port %d. (TetriNET already running?)\n",j);
  lvprintf(0,"Couldn't find telnet port %d.\n", j);
  exit(1);
}

/* Open a Listening Socket on the TetriNET query port */
void init_query_port()
{
  int i,j;
  struct net_t *n;
  
  /* find old entry if it exists */
  n=gnet;
  while ( (n!=NULL) && (n->type!=NET_QUERY))
    n=n->next;
    
  if (n==NULL) {
    /* no existing entry */
    n=gnet;
    gnet=malloc(sizeof(struct net_t));
    gnet->next=n;
    n=gnet;
    n->addr=getmyip();
    n->type=NET_QUERY;
    strcpy(n->nick,"(telnet)");
    getmyhostname(n->host);
  }
  else {
    /* already an entry */
    killsock(n->sock);
  }
  j=QUERY_PORT;
  i=open_listen_socket(&j,game.bindip);

  if (i>=0) {
    n->port=j;
    n->sock=i;

    lvprintf(3,"Listening at query port %d, on socket %d, bound to %s\n", j,i,game.bindip);
    return;
  }
  printf("Couldn't find telnet port %d. (TetriNET already running?)\n",j);
  lvprintf(0,"Couldn't find telnet port %d.\n", j);
  exit(1);
}




/* Main loop calls here when activity found on a net socket */
void net_activity(int z, char *buf, int len)
  {
    struct net_t *n;
    struct channel_t *chan;
    
    chan=chanlist;
    n=NULL;
    while ( (chan!=NULL) && (n==NULL) )
      {
        n=chan->net;
        while( (n!=NULL) && (n->sock!=z) )
          n=n->next;
        chan=chan->next;
      }

    if (n==NULL) 
      { /* Try global list */
        n=gnet;
        while( (n!=NULL) && (n->sock!=z) )
          n=n->next;
      }
    
      
    if (n==NULL) return;
    
    /* Reset timeout on socket */
    if (n->status == STAT_PLAYING)
      n->timeout = game.timeout_ingame;
    else
      n->timeout = game.timeout_outgame;
    
    switch (n->type)
      {
        case NET_TELNET: 		/* Recieved connection */
          {
            net_telnet(n,buf);
            break;
          }
        case NET_QUERY:			/* Recieved connection */
          {
            net_query(n,buf);
            break;
          }
        case NET_TELNET_INIT:		/* Received clients init sequence */
          {
            net_telnet_init(n,buf);
            break;
          }
        case NET_QUERY_INIT:		/* Received clients init sequence */
          {
            net_query_init(n,buf);
            break;
          }
        case NET_WAITINGFORTEAM:	/* Waiting for inital team */
          {
            net_waitingforteam(n,buf);
            break;
          }
        case NET_CONNECTED:		/* Recieved command from client */
          {
            net_connected(n,buf);
            break;
          }
        default:
          {
            lvprintf(0,"Internal Error - Untrapped Network activity. (BIG ERROR)\n");
          }
      }
  }
  
/* String length (minus colours) */
int strclen(char *S)
  {
    int i;int count;
    if (S==NULL)
      return 0;
      
    i=0;count=0;
    while(S[i]!=0)
      {
        if(              (S[i] != BOLD)
                      && (S[i] != CYAN)
                      && (S[i] != BLACK)
                      && (S[i] != BLUE)
                      && (S[i] != DARKGRAY)
                      && (S[i] != MAGENTA)
                      && (S[i] != GREEN)
                      && (S[i] != NEON)
                      && (S[i] != SILVER)
                      && (S[i] != BROWN)
                      && (S[i] != NAVY)
                      && (S[i] != VIOLET)
                      && (S[i] != RED)
                      && (S[i] != ITALIC)
                      && (S[i] != TEAL)
                      && (S[i] != WHITE)
                      && (S[i] != YELLOW)
                      && (S[i] != UNDERLINE) )
                count++;
        i++;  
      }
    return(i);
  }

/* Connected, recieving commands */
void net_connected(struct net_t *n, char *buf)
  {
    char COMMAND[101]; char PARAM[601]; char MSG[601];
    char STRG[513], STRG2[513], STRG3[80];
    int num; int num1; int num2; int num3; int num4; int s; int i,j,k,l,x,y;
    int nn1,nn2,nn3,nn4,nn5,nn6,nn7,nn8,nn9;
    int valid_param;
    char *P=NULL;
    struct channel_t *chan,*ochan,*c,*oc;
    struct net_t *nsock=NULL;
    struct net_t *ns1,*ns2;
    
    COMMAND[0]=0;PARAM[0]=0;MSG[0]=0;
    /* Ensure command is proper before passing it to other players */
    sscanf(buf, "%100s %600[^\n\r]", COMMAND, PARAM);
    valid_param = 0; /* 0 = invalid    */
                     /* 1 = valid      */
                     /* 2 = valid command, but failed some test */

    
    /* Party Line Message - pline <playernumber> <message> */
    if ( !strcasecmp(COMMAND, "pline") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %600[^\n\r]", &num, MSG);
        if ( (s >= 2) && (num==n->gameslot))
          {
            if (MSG[0]=='/')
              {
                valid_param=1;
                /* First parse to see if it's a server command */
                if ( !strncasecmp(MSG, "/kick", 5) && (game.command_kick > 0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_kick) )
                      {
                        P=MSG+6;
                        while( (((*P)-'0') >= 1) && (((*P)-'0') <= 6) )
                          {
                            
                            kick(n, (*P)-'0');
                            P++;
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                   
                  }

                /* Set channel settings */
                if ( !strncasecmp(MSG, "/set", 4) && (game.command_set>0))
                  {
                    valid_param=2;
                    if (passed_level(n,game.command_set) 
                      || ((passed_level(n,LEVEL_AUTHOP)||(passed_level(n,LEVEL_OP)&&!n->channel->persistant))
                           && (game.command_set==4)
                         )
                       )
                      {
                        PARAM[0]=0;
                        STRG2[0]=0;
                        STRG[0]=0;
                        sscanf(MSG,"%s %s %1024[^\n]", STRG2,STRG,PARAM);

                                
                        if ( !strcasecmp(STRG,"BLOCK") )
                          {
                            k=0;
                            k=sscanf(PARAM,"%d %d %d %d %d %d %d",&nn1,&nn2,&nn3,&nn4,&nn5,&nn6,&nn7);
                            if (k==7)
                              {
                                if ((abs(nn1)+abs(nn2)+abs(nn3)+abs(nn4)+abs(nn5)+abs(nn6)+abs(nn7)) == 100)
                                  {
                                    n->channel->block_leftl=abs(nn1);
                                    n->channel->block_leftz=abs(nn2);
                                    n->channel->block_square=abs(nn3);
                                    n->channel->block_rightl=abs(nn4);
                                    n->channel->block_rightz=abs(nn5);
                                    n->channel->block_halfcross=abs(nn6);
                                    n->channel->block_line=abs(nn7);
                                    tprintf(n->sock,"pline 0 %cSettings Updated!\xff",NAVY);
                                  }
                                else
                                  {
                                    tprintf(n->sock,"pline 0 %cAll Parameters must add up to 100%%\xff",NAVY);
                                  }
                              }
                            else if (strlen(PARAM)==0)
                              {
                                tprintf(n->sock,"pline 0 %cBLOCK\xff",RED);
                                tprintf(n->sock,"pline 0 %cSet the percentage occurance of blocks in a game\xff",NAVY);
                                tprintf(n->sock,"pline 0 %cll\t%cPercent Left L shaped block\xff", BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %clz\t%cPercent Left Z shaped block\xff", BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %csq\t%cPercent Square shaped block\xff", BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %crl\t%cPercent Right L shaped block\xff", BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %crz\t%cPercent Right Z shaped block\xff", BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %chc\t%cPercent HalfCross shaped block\xff", BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cln\t%cPercent Line shaped block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cAll percentages add up to a total of 100%%\xff",NAVY);
                                tprintf(n->sock,"pline 0 %cIE: %c/set %cBLOCK %c14 14 15 14 14 14 15\xff",NAVY,BLUE,RED,BLUE);
                                tprintf(n->sock,"pline 0 Current Setting: %d %d %d %d %d %d %d\xff",
                                                  n->channel->block_leftl,
                                                  n->channel->block_leftz,
                                                  n->channel->block_square,
                                                  n->channel->block_rightl,
                                                  n->channel->block_rightz,
                                                  n->channel->block_halfcross,
                                                  n->channel->block_line);
                              } 
                            else
                              tprintf(n->sock,"pline 0 %cNot enough parameters. Try %c/set %cBLOCK\xff",NAVY,BLUE,RED);
                          }
                        else if ( !strcasecmp(STRG,"SPECIAL") )
                          {
                            k=0;
                            k=sscanf(PARAM,"%d %d %d %d %d %d %d %d %d",&nn1,&nn2,&nn3,&nn4,&nn5,&nn6,&nn7,&nn8,&nn9);
                            if (k==9)
                              {
                                if (   ((abs(nn1)+abs(nn2)+abs(nn3)+abs(nn4)+abs(nn5)+abs(nn6)+abs(nn7)+abs(nn8)+abs(nn9)) == 100)
                                   )
                                  {
                                    n->channel->special_addline=abs(nn1);
                                    n->channel->special_clearline=abs(nn2);
                                    n->channel->special_nukefield=abs(nn3);
                                    n->channel->special_randomclear=abs(nn4);
                                    n->channel->special_switchfield=abs(nn5);
                                    n->channel->special_clearspecial=abs(nn6);
                                    n->channel->special_gravity=abs(nn7);
                                    n->channel->special_quakefield=abs(nn8);
                                    n->channel->special_blockbomb=abs(nn9);
                                    tprintf(n->sock,"pline 0 %cSettings Updated!\xff",NAVY);
                                  }
                                else
                                  tprintf(n->sock,"pline 0 %cAll Parameters must add up to 100%%\xff",NAVY);
                              }
                            else if (strlen(PARAM)==0)
                              {
                                tprintf(n->sock,"pline 0 %cSPECIAL\xff",RED);
                                tprintf(n->sock,"pline 0 %cSet the percentage occurance of special blocks in a game\xff",NAVY);
                                tprintf(n->sock,"pline 0 %cA\t%cPercent ADDLINE block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cC\t%cPercent CLEARLINE block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cN\t%cPercent NUKEFIELD block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cR\t%cPercent RANDOMCLEAR block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cS\t%cPercent SWITCHFIELD block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cB\t%cPercent CLEARSPECIAL block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cG\t%cPercent GRAVITY block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cQ\t%cPercent QUAKEFIELD block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cO\t%cPercent BLOCKBOMB block\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cAll percentages add up to a total of 100%%\xff",NAVY);
                                tprintf(n->sock,"pline 0 %cIE: %c/set %cSPECIAL %c32 18 1 11 3 14 1 6 14\xff",NAVY,BLUE,RED,BLUE);
                                tprintf(n->sock,"pline 0 Current Setting: %d %d %d %d %d %d %d %d %d\xff",
                                              n->channel->special_addline,
                                              n->channel->special_clearline,
                                              n->channel->special_nukefield,
                                              n->channel->special_randomclear,
                                              n->channel->special_switchfield,
                                              n->channel->special_clearspecial,
                                              n->channel->special_gravity,
                                              n->channel->special_quakefield,
                                              n->channel->special_blockbomb);
                              }
                            else
                              tprintf(n->sock,"pline 0 %cNot enough parameters. Try %c/set %cSPECIAL\xff",NAVY,BLUE,RED);
                          }
                        else if ( !strcasecmp(STRG,"SUDDENDEATH") )
                          {
                            k=0;
                            k=sscanf(PARAM,"%d %d %d",&nn1,&nn2,&nn3);
                            if (k==3)
                              {
                                if ( (nn1 >= 0) && (nn2 >= 0) && (nn3 > 0) )
                                  {
                                    n->channel->sd_timeout=nn1;
                                    n->channel->sd_lines_per_add=nn2;
                                    n->channel->sd_secs_between_lines=nn3;
                                    tprintf(n->sock,"pline 0 %cSettings Updated!\xff",NAVY);
                                  }
                                else
                                  tprintf(n->sock,"pline 0 %cInvalid Parameters\xff",NAVY);
                              }
                            else if (strlen(PARAM)==0)
                              {
                                tprintf(n->sock,"pline 0 %cSUDDENDEATH\xff",RED);
                                tprintf(n->sock,"pline 0 %cSet the configuration for SUDDENDEATH mode\xff",NAVY);
                                tprintf(n->sock,"pline 0 %cto\t%cNumber of Seconds till SuddenDeath Mode\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cla\t%cNumber of Lines to add each time\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %csl\t%cNumber of Seconds between each add line\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cIE: %c/set %cSUDDENDEATH %c180 1 30\xff",NAVY,BLUE,RED,BLUE);
                                tprintf(n->sock,"pline 0 Current Setting: %d %d %d\xff",
                                        n->channel->sd_timeout,
                                        n->channel->sd_lines_per_add,
                                        n->channel->sd_secs_between_lines);
                              }
                            else
                              tprintf(n->sock,"pline 0 %cNot enough parameters. Try %c/set %cSUDDENDEATH\xff",NAVY,BLUE,RED);
                          }
                        else if ( !strcasecmp(STRG,"SUDDENDEATHMSG") )
                          {
                            k=0;
                            k=sscanf(PARAM,"%512[^\n]",STRG3);
                            if (k==1)
                              {
                                strncpy(n->channel->sd_message,STRG3,SDMSGLEN);n->channel->sd_message[SDMSGLEN-1]=0;
                                tprintf(n->sock,"pline 0 %cSettings Updated!\xff",NAVY);
                              }
                            else if (strlen(PARAM)==0)
                              {
                                tprintf(n->sock,"pline 0 %cSUDDENDEATHMSG\xff",RED);
                                tprintf(n->sock,"pline 0 %cSet the Message sent to players when SuddenDeath mode arrives\xff",NAVY);
                                tprintf(n->sock,"pline 0 %cmsg\t%cMessage to send to players\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cIE: %c/set %cSUDDENDEATHMSG %cTime's up! It's SUDDEN DEATH MODE!\xff",NAVY,BLUE,RED,BLUE);
                                tprintf(n->sock,"pline 0 Current Setting: %s\xff",n->channel->sd_message);
                              }
                            else
                              tprintf(n->sock,"pline 0 %cNot enough parameters. Try %c/set %cSUDDENDEATHMSG\xff",NAVY,BLUE,RED);
                          }
                        else if ( !strcasecmp(STRG,"RULES") )
                          {
                            k=0;
                            k=sscanf(PARAM,"%d %d %d %d %d %d %d %d",&nn1,&nn2,&nn3,&nn4,&nn5,&nn6,&nn7,&nn8);
                            if (k==8)
                              {
                                if ( (nn1>=0)&&(nn1<100)&&(nn2>0)&&(nn3>0)&&(nn4>0)&&(nn5>=0)&&(nn6>0)&&(nn6<=18)&&(nn7>=0)&&(nn7<=1)&&(nn8>=0)&&(nn8<=1))
                                  {
                                    n->channel->starting_level=nn1;
                                    n->channel->lines_per_level=nn2;
                                    n->channel->level_increase=nn3;
                                    n->channel->lines_per_special=nn4;
                                    n->channel->special_added=nn5;
                                    n->channel->special_capacity=nn6;
                                    n->channel->classic_rules=nn7;
                                    n->channel->average_levels=nn8;
                                    tprintf(n->sock,"pline 0 %cSettings Updated!\xff",NAVY);
                                  }
                                else
                                  tprintf(n->sock,"pline 0 %cInvalid parameters\xff",NAVY);                          
                              }
                            else if (strlen(PARAM)==0)
                              {
                                tprintf(n->sock,"pline 0 %cRULES\xff",RED);
                                tprintf(n->sock,"pline 0 %cSet the Channel Rules in the game\xff",NAVY);
                                tprintf(n->sock,"pline 0 %csl\t%cStarting Level of all players\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cll\t%cLines per level\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cli\t%cAmount level increases each time\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cls\t%cLines per special\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %csa\t%cNumber of specials added each time\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %csc\t%cSpecial Capacity of players\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %ccr\t%cPlay with classic rules?\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cal\t%cAverage all players levels?\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cIE: %c/set %cRULES %c1 2 1 1 1 18 1 1\xff",NAVY,BLUE,RED,BLUE);
                                tprintf(n->sock,"pline 0 Current Setting: %d %d %d %d %d %d %d %d\xff",
                                      n->channel->starting_level,
                                      n->channel->lines_per_level,
                                      n->channel->level_increase,
                                      n->channel->lines_per_special,
                                      n->channel->special_added,
                                      n->channel->special_capacity,
                                      n->channel->classic_rules,
                                      n->channel->average_levels);
                              }
                            else
                              tprintf(n->sock,"pline 0 %cNot enough parameters. Try %c/set %cRULES\xff",NAVY,BLUE,RED);
                          }
                        else if ( !strcasecmp(STRG,"MISC") )
                          {
                            k=0;
                            k=sscanf(PARAM,"%d %d %d",&nn1,&nn2,&nn3);
                            if (k==3)
                              {
                                if ( (nn1>=0)&&(nn1<=1)&&(nn2>=0)&&(nn2<=1)&&(nn3>=0)&&(nn3<=1))
                                  {
                                    n->channel->stripcolour=nn1;
                                    n->channel->serverannounce=nn2;
                                    n->channel->pingintercept=nn3;
                                    tprintf(n->sock,"pline 0 %cSettings Updated!\xff",NAVY);
                                  }
                                else
                                  tprintf(n->sock,"pline 0 %cInvalid parameters\xff",NAVY);
                              }
                            else if (strlen(PARAM)==0)
                              {
                                tprintf(n->sock,"pline 0 %cMISC\xff",RED);
                                tprintf(n->sock,"pline 0 %cSet Misc options\xff",NAVY);
                                tprintf(n->sock,"pline 0 %csc\t%cStrip Colour from GameMessages?\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %csa\t%cServer Announces winner?\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cpi\t%cIntercept \"t\" in a Gamemessage as PING?\xff",BLUE,NAVY);
                                tprintf(n->sock,"pline 0 %cIE: %c/set %cMISC %c 1 1 1\xff",NAVY,BLUE,RED,BLUE);
                                tprintf(n->sock,"pline 0 Current Setting: %d %d %d\xff",
                                   n->channel->stripcolour,
                                   n->channel->serverannounce,
                                   n->channel->pingintercept);
                              }
                            else
                              tprintf(n->sock,"pline 0 %cNot enough parameters. Try %c/set %cMISC\xff",NAVY,BLUE,RED);
                          }
                        else
                          {
                            tprintf(n->sock,"pline 0 %cSET %ccommands - Set channel attributes\xff",TEAL, NAVY);
                            tprintf(n->sock,"pline 0 %cCommands are of the general form: %c/set %c<command>%c[%cvalue [..value]%c]\xff",NAVY,BLUE,RED,BLACK,BLUE,BLACK);
                            tprintf(n->sock,"pline 0 %cWhen given without a value, the current value is returned.\xff",NAVY);
                            tprintf(n->sock,"pline 0 %c/set %cBLOCK %cll lz sq rl rz hc ln   %cSet Block Configuration\xff", BLUE,RED,BLUE,NAVY);
                            tprintf(n->sock,"pline 0 %c/set %cSPECIAL %cA C N R S B G Q O   %cSet Special Configuration\xff",BLUE,RED,BLUE,NAVY);
                            tprintf(n->sock,"pline 0 %c/set %cSUDDENDEATH %cto la sl   %cSet SuddenDeath Configuration\xff",BLUE,RED,BLUE,NAVY);
                            tprintf(n->sock,"pline 0 %c/set %cSUDDENDEATHMSG %cmsg   %cSet SuddenDeath Message\xff",BLUE,RED,BLUE,NAVY);
                            tprintf(n->sock,"pline 0 %c/set %cRULES %csl ll li ls sa sc cr al   %cSet Rules Configuration\xff",BLUE,RED,BLUE,NAVY);
                            tprintf(n->sock,"pline 0 %c/set %cMISC %csc sa pi   %cSet Misc Configuration\xff",BLUE,RED,BLUE,NAVY);
                            /*tprintf(n->sock,"pline 0 %c/set %cCONFIG %cconfignum   %cSet ALL config options in one shot\xff",BLUE,RED,BLUE,NAVY);*/
                            tprintf(n->sock,"pline 0 %cType %c/set %ccommandname %cto find out what the parameters mean.\xff",NAVY,BLUE,RED,NAVY);
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  } 
            
                /* Set channel topic */
                if ( !strncasecmp(MSG, "/topic", 6) && (game.command_topic>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_topic) )
                      {
                        P=MSG+7;
                        if (strlen(MSG)>=7)
                          {
                            strncpy(n->channel->description, P, DESCRIPTIONLEN-1); n->channel->description[DESCRIPTIONLEN-1]=0;
                            lvprintf(4,"#%s-%s changed channel topic to %s\n",n->channel->name,n->nick,n->channel->description);
                            nsock=n->channel->net;
                            while (nsock!=NULL)
                              {
                                if ((nsock->type==NET_CONNECTED))
                                  {
                                    tprintf(nsock->sock,"pline 0 %c%s%c%c changes the channel topic to %s\xff", GREEN,n->nick,BLACK,GREEN,n->channel->description);
                                  }
                                nsock=nsock->next;
                              }
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
                  
                /* Who is online */
                if ( !strncasecmp(MSG, "/who", 4) && (game.command_who>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_who) )
                      {
                        if (passed_level(n,LEVEL_AUTHOP))
                          tprintf(n->sock,"pline 0 %cNickname\tTeam            \tChannel\tIP\xff", NAVY);
                        else
                          tprintf(n->sock,"pline 0 %cNickname\tTeam            \tChannel\xff", NAVY);
                        l=1;
                        chan=chanlist;
                        while(chan!=NULL)
                          {
                            nsock=chan->net;
                            while (nsock!=NULL)
                              {
                                if (nsock->type==NET_CONNECTED)
                                  {
                                    STRG[0]=0;
                                    STRG2[0]=0;
                                    strcpy(STRG,nsock->nick);
                                    j=strclen(STRG);
                                    k=strlen(STRG);
                                    while(j<15)
                                      {
                                        STRG[k]=' ';
                                        k++;
                                        j++;
                                      }
                                    STRG[k]=0;
                              
                                    strcpy(STRG2,nsock->team);
                                    j=strclen(STRG2);
                                    k=strlen(STRG2);
                                    while(j<17)
                                      {
                                        STRG2[k]=' ';
                                        k++;
                                        j++;
                                      }
                                    STRG2[k]=0;
                            
                                    if (nsock==n)
                                      j=RED;
                                    else
                                      j=DARKGRAY;
                                    
                                    if (passed_level(n,LEVEL_AUTHOP))
                                      tprintf(n->sock,"pline 0 %c(%c%d%c) %c%s\t%c%s\t%c#%s\t%s\xff", NAVY,j,l,NAVY, j, STRG,TEAL,STRG2,BLUE,nsock->channel->name,nsock->host);
                                    else
                                      tprintf(n->sock,"pline 0 %c(%c%d%c) %c%s\t%c%s\t%c#%s\xff", NAVY,j,l,NAVY, j, STRG,TEAL,STRG2,BLUE,nsock->channel->name);
                                    l++;
                                  }
                                nsock=nsock->next;
                              }
                            chan=chan->next;
                          }
                      } 
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
              
                /* Set channel priority */
                if ( !strncasecmp(MSG, "/priority", 9) && (game.command_priority>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_priority) )
                      {
                        P=MSG+10;
                        if ( (atoi(P) >= 0) && (atoi(P) < 100) )
                          {
                            n->channel->priority=atoi(P);
                            lvprintf(4,"#%s-%s changed channel priority to %d\n",n->channel->name,n->nick,n->channel->priority);
                            nsock=n->channel->net;
                            while (nsock!=NULL)
                              {
                                if ((nsock->type==NET_CONNECTED) )
                                  {
                                    tprintf(nsock->sock,"pline 0 %c%s%c%c changes the channel priority to %d\xff", GREEN,n->nick,BLACK,GREEN,n->channel->priority);
                                  }
                                nsock=nsock->next;
                              }
                          }
                        else
                          {
                            tprintf(n->sock,"pline 0 %cChannel priority should lie in the range of 0 to 99. 0 prevents people from automatically joining.\xff",NAVY);
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }

                /* List Channels */
                if ( !strncasecmp(MSG, "/list", 5) && (game.command_list>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_list) )
                      {
                        if (chanlist != NULL)
                          {
                            tprintf(n->sock,"pline 0 %cTetriNET Channel Lister - (Type %c/join %c#channelname%c)\xff",NAVY,RED,BLUE,NAVY);
                            chan=chanlist;
                            i=1;
                            while (chan != NULL)
                              {
                                if (n->channel==chan)
                                  j=RED;
                                else
                                  j=BLUE;
                              
                                if (chan->status==STATE_INGAME)
                                  {
                                    sprintf(STRG,"%c{INGAME}%c", DARKGRAY,NAVY);
                                  }
                                else
                                  {
                                    sprintf(STRG,"                ");
                                  }
                                if (numplayers(chan) >= chan->maxplayers)
                                  tprintf(n->sock,"pline 0 %c(%c%d%c) %c#%-6s\t%c[%cFULL%c] %s       (%d)   %c%s\xff", NAVY, j, i, NAVY, BLUE, chan->name, NAVY,RED,NAVY,STRG, chan->priority,BLACK,chan->description);
                                else
                                  {
                                    tprintf(n->sock,"pline 0 %c(%c%d%c) %c#%-6s\t%c[%cOPEN%c-%d/%d%c] %s (%d)   %c%s\xff", NAVY, j, i, NAVY, BLUE, chan->name, NAVY,TEAL,BLUE,numplayers(chan),chan->maxplayers,NAVY,STRG, chan->priority,BLACK,chan->description);
                                  }
                                i++;
                                chan=chan->next;
                              }
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
              
                /* Join Channel */
                if ( !strncasecmp(MSG, "/j", 2) && (game.command_join>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_join) || (game.command_join==4))
                      {
                        if ( !strncasecmp(MSG,"/join", 5) )
                          P=MSG+6;
                        else
                          P=MSG+3;
                        STRG[0]=0;
                        k = sscanf(P,"%512s", STRG);
                        if ((k != 0) )
                          {
                            /* First, is it a #channel or channel number? */
                            j=atoi(STRG);
                            chan=NULL;
                            ochan=NULL;
                            if (j>0)
                              { /* Position */
                                chan=chanlist;
                                j--;
                                while( (j>0) && (chan!=NULL) ) 
                                  {
                                    chan=chan->next;
                                    j--;
                                  }
                              }
                            else if (STRG[0]=='#')
                              { /* Name */
                                sscanf(P,"#%512s", STRG);
                            
                                chan = chanlist;
                                while ( (chan != NULL) && (strcasecmp(STRG,chan->name)) )
                                  chan=chan->next;
                                j=0;
                              }
                            else
                              {
                                j=-1;
                                tprintf(n->sock,"pline 0 %cFormat: %c/join %c<#channel|channel number>\xff", NAVY,RED,BLUE);
                              }
                            ochan=n->channel;
                            if ( (chan!=NULL) && (numplayers(chan)>=chan->maxplayers))
                              { /* A Full channel */
                                tprintf(n->sock,"pline 0 %cThat channel is %cFULL%c!\xff", NAVY, RED,BLACK); 
                              }
                            else if ( (chan==NULL) && (j>=0) && (numchannels() >= game.maxchannels))
                              { /* Too many channels */
                                tprintf(n->sock,"pline 0 %cCannot create any more channels!\xff",RED);
                              }
                            else if ( (chan==NULL) && (j>=0) && (game.command_join==4) && (n->securitylevel!=LEVEL_AUTHOP))
                              { /* Can't create a NEW channel */
                                tprintf(n->sock,"pline 0 %cCannot create new channel\xff",RED);
                              }
                            else if (j>=0)
                              {
                                if (chan == NULL)
                                  { /* New channel */
                                    chan=chanlist;
                                    while ((chan!= NULL) && (chan->next!=NULL)) chan=chan->next;
                                
                                    if (chan==NULL)
                                      {
                                        chanlist=malloc(sizeof(struct channel_t));
                                        chan=chanlist;
                                      }
                                    else
                                      {
                                        chan->next=malloc(sizeof(struct channel_t));
                                        chan=chan->next;
                                      }
                                
                                    chan->next=NULL;
                                    chan->net=NULL;
                                    strncpy(chan->name, STRG, CHANLEN-1); chan->name[CHANLEN-1]=0;
                                    chan->maxplayers=DEFAULTMAXPLAYERS;
                                    chan->status=STATE_ONLINE;
                                    chan->description[0]=0;
                                    chan->priority=DEFAULTPRIORITY;
                                    chan->sd_mode=SD_NONE;
                                    chan->persistant=0;
                                
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
                                
                                    /* Remove ourselves from old channel list */
                                    n->channel=chan;
                                
                                    lvprintf(4,"#%s-%s created new channel\n", n->channel->name,n->nick);
                                    tprintf(n->sock,"pline 0 %cCreated new Channel - %c#%s%c\xff", GREEN, BLUE, chan->name, BLACK);
                                  }
                        
                                else
                                  { /* An already existing channel */
                                    n->channel = chan;
                                    lvprintf(4,"#%s-%s joined channel\n", n->channel->name,n->nick);
 
                                    tprintf(n->sock,"pline 0 %cJoined existing Channel - %c#%s%c\xff", GREEN, BLUE, chan->name, BLACK);
                                  }
                                remnet(ochan,n);
                                addnet(chan,n);
                            
                                /* Send to old channel, this player join message */
                                nsock=ochan->net;
                                while (nsock!=NULL)
                                  {
                                    if ( (nsock->type==NET_CONNECTED) )
                                      {
                                        /* Tell them we've joined this channel */
                                        tprintf(nsock->sock,"pline 0 %c%s%c %chas joined channel #%s\xff", DARKGRAY,n->nick,BLACK,DARKGRAY,n->channel->name);
                                      }
                                    nsock=nsock->next;
                                  }
                            
                                if ((ochan->status == STATE_INGAME) || (ochan->status ==STATE_PAUSED))
                                  {
                                    n->status = STAT_NOTPLAYING;
                                    tprintf(n->sock,"endgame\xff");
                                  }
                                  
                                /* Cleartheir Field */
                                for(y=0;y<FIELD_MAXY;y++)
                                  for(x=0;x<FIELD_MAXX;x++)
                                    n->field[x][y]=0; /* Nothing */
                        
                                /* Work out gameslot */
                                num1=n->gameslot;
                                num2=0; num3=1;
                                while ( (num2 < n->channel->maxplayers) && (num3) )
                                  {
                                    num2++;
                                    num3=0;
                                    nsock=n->channel->net;
                                    while (nsock!=NULL)
                                      {
                                        if( (nsock!=n) && ( (nsock->type==NET_CONNECTED) || (nsock->type==NET_WAITINGFORTEAM) )&&(nsock->gameslot==num2) ) num3=1;
                                        nsock=nsock->next;
                                      }
                                  }
                                if (num3==1)
                                  {
                                    lvprintf(0,"#%s-%s Ran out of places in channel (FATAL)\n",n->channel->name,n->nick);
                                    killsock(n->sock);lostnet(n);
                                    return;
                                  }
                              
                                /* Clear our spot */
                                tprintf(n->sock,"playerleave %d\xff",n->gameslot);
                                n->gameslot=num2;
                            
                                /* Now, send playerleft to all on old channel AND to player */
                                num3=0;
                                num4=0;
                                STRG2[0]=0;
                                nsock=ochan->net;
                                while (nsock!=NULL)
                                  {
                                    if ( (nsock!=n) && (nsock->type == NET_CONNECTED))
                                      {
                                        tprintf(nsock->sock,"playerleave %d\xff", num1);
                                        tprintf(n->sock,"playerleave %d\xff", nsock->gameslot);
                                        if (strcasecmp(STRG2,nsock->team))
                                          {/* Different team, so add player */
                                            num3++;
                                            if (nsock->status==STAT_PLAYING) num4++;
                                            strcpy(STRG2,nsock->team);
                                          }
                                        else if (STRG2[0]==0) 
                                          {
                                            num3++;
                                            if (nsock->status==STAT_PLAYING) num4++;
                                          }
                                      }
                                    nsock=nsock->next;
                                  }
                            
                                /* Now send to new channel, this player joined, AND to player */
                                nsock=n->channel->net;
                                while (nsock!=NULL)
                                  {
                                    if ( (nsock!=n) && (nsock->type==NET_CONNECTED))
                                      {
                                        /* Send each other player and their team anme */
                                        tprintf(n->sock, "playerjoin %d %s\xffteam %d %s\xff",nsock->gameslot, nsock->nick, nsock->gameslot, nsock->team);
                                        /* Send to the other player, this player and their team */
                                        tprintf(nsock->sock, "playerjoin %d %s\xffteam %d %s\xff", n->gameslot, n->nick, n->gameslot, n->team);
                                        /* Tell them we've joined this channel */
                                        tprintf(nsock->sock,"pline 0 %c%s%c %chas joined channel #%s\xff", GREEN,n->nick,BLACK,GREEN,n->channel->name);
                                      }
                                    nsock=nsock->next;
                                  }
                        
                                /* Set our playernumber */
                                tprintf(n->sock, "playernum %d\xff",n->gameslot); 
                      
                                /* If game is in progress, send all other players fields */
                                if( (n->channel->status == STATE_INGAME) || (n->channel->status == STATE_PAUSED) )  
                                  {
                                    nsock=n->channel->net;
                                    while (nsock!=NULL)
                                      {
                                        if( (nsock->type==NET_CONNECTED) &&(nsock!=n))  
                                          {
                                            /* Each player who has lost, send player_lost */
                                            if (nsock->status != STAT_PLAYING)
                                              {
                                                tprintf(n->sock,"playerlost %d\xff", nsock->gameslot);
                                              }
                                            sendfield(n, nsock); /* Send to player, this players field */
                                          }
                                        nsock=nsock->next;
                                      }
                                  }
                          
                                /* If we are ingame, then tell this person that we are! */
                                if ( (n->channel->status == STATE_INGAME) || (n->channel->status == STATE_PAUSED) )
                                  tprintf(n->sock,"ingame\xff");
                          
                                /* If we are currently paused, kindly let them know that fact to */ 
                                if ( n->channel->status == STATE_PAUSED )
                                  tprintf(n->sock,"pause 1\xff");
                                                                               
                                /* If 1 or less players/teams now are playing, AND the player that quit WAS playing, STOPIT */
                                if ( (num4 <= 1) && (ochan->status == STATE_INGAME) && (n->status == STAT_PLAYING) )
                                  {
                                    nsock=ochan->net;
                                    while (nsock!=NULL)
                                      {
                                        if ( (nsock=n) && (nsock->type == NET_CONNECTED))
                                          {
                                            tprintf(nsock->sock,"endgame\xff");
                                            nsock->status=STAT_NOTPLAYING;
                                            nsock->timeout=game.timeout_outgame;
                                          }
                                        nsock=nsock->next;
                                      }
                                    ochan->status = STATE_ONLINE;
                                  }
          
                                /* If no players, then we delete the channel IF it's not persistant*/
                                if ( (numallplayers(ochan) == 0) && (ochan!=n->channel) && (!ochan->persistant) )
                                  {
                                    c=chanlist;
                                    oc=NULL;
                                    while ( (c != ochan) && (c != NULL) ) 
                                      {
                                        oc=c;
                                        c=c->next;
                                      }
                                    if (c != NULL)
                                      {
                                        if (oc != NULL)
                                          oc->next=c->next;
                                        else
                                          chanlist=c->next;
                                        free(c); 
                                      } 
                                  }
                        
                                n->timeout=game.timeout_outgame;
                                if (n->channel->status == STATE_ONLINE)
                                  {
                                    n->status=STAT_NOTPLAYING;
                                  }
                                else
                                  {
                                    n->status=STAT_LOST;
                                  }
                              }
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
              
                /* Clear Winlist - Suggestion by n|ck */
                if ( !strncasecmp(MSG, "/clear", 6) && (game.command_clear>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_clear) )
                      {
                        lvprintf(4,"#%s-%s cleared the winlist\n",n->channel->name,n->nick);
                        init_winlist();
                        writewinlist();
                        sendwinlist(n->channel,NULL);
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
            
                /* Persistant Channel */
                if ( !strncasecmp(MSG, "/persistant", 11) && (game.command_persistant>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_persistant) )
                      {
                        P=MSG+12;
                        if (*P=='0')
                          {
                            n->channel->persistant=0;
                            tprintf(n->sock,"pline 0 %cChannel will be deleted when last player leaves\xff",NAVY);
                          }
                        else
                          {
                            n->channel->persistant=1;
                            tprintf(n->sock,"pline 0 %cChannel is now persistant, and will not be deleted\xff",NAVY);
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
        
                /* Save Config */
                if ( !strncasecmp(MSG, "/save", 5) && (game.command_save>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_save) )
                      {
                        gamewrite();
                        tprintf(n->sock,"pline 0 %cGame configuration and Persistant Channel info saved\xff",NAVY);
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
            
                /* Reset/Load Config */
                if ( !strncasecmp(MSG, "/reset", 6) && (game.command_reset>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_reset) )
                      {
                        gameread();
                        tprintf(n->sock,"pline 0 %cGame configuration and Persistant Channel info loaded\xff",NAVY);
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
            
                /* Private msg to a person - Suggestion by crazor */
                if ( !strncasecmp(MSG, "/msg", 4) && (game.command_msg>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_msg) )
                      {
                        P=MSG+5;
                        sscanf(P,"%512s %512[^\n\r ]", STRG, STRG2);
                
                        P=STRG;
                        while( (((*P)-'0') >= 1) && (((*P)-'0') <= 6) )
                          {
                            j=(*P)-'0';
                            nsock=n->channel->net;
                            while (nsock!=NULL)
                              {
                                if ( (nsock->type == NET_CONNECTED) && (nsock->gameslot == j))
                                  {
                                    lvprintf(5,"#%s-(%s to %s): %s\n", n->channel->name,n->nick,nsock->nick,STRG2);
                                    tprintf(nsock->sock,"pline %d %c(msg)%c %s\xff", n->gameslot, NAVY, BLACK, STRG2); 
                                  }
                                nsock=nsock->next;
                              }
                            P++;
                          }
                      }  
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
              
                /* Move a player to another gamespot */
                if ( !strncasecmp(MSG, "/move", 5) && (game.command_move>0))
                  {
                    valid_param=2;
                    if ( passed_level(n,game.command_move) && (n->channel->status == STATE_ONLINE) )
                      {
                        P=MSG+6;
                        k=0;l=0;
                        sscanf(P," %d %d", &k, &l);
                    
                        if ( (k>0) && (k<=6) && (l>0) && (l<=6) && (k!=l) )
                          {
                            /* Ok, this is NOT supposed to work, but it does ;) */
                     
                            /* Give player l, player k's number */
                            /* Give player k, player l's number */
                            /* Send to all others, as if they've rejoined */
                        
                            /* Find sock_index... for k */
                            ns1=n->channel->net;
                            while ( (ns1!=NULL) && !( ((ns1->type == NET_CONNECTED) && (ns1->gameslot==k) )))
                              ns1=ns1->next;
                        
                            ns2=n->channel->net;
                            while( (ns2!=NULL) && !((ns2->type == NET_CONNECTED) && (ns2->gameslot==l) ))
                              ns2=ns2->next;
                        
                            if ( (ns1!=NULL) )
                              {
                                tprintf(ns1->sock,"playernum %d\xff", l);
                                ns1->gameslot = l;
                            
                                if (ns2!=NULL)
                                  { /* This player existed... */
                                    lvprintf(4,"#%s-%s swaps player %s(%d) with %s(%d)\n", n->channel->name,n->nick,ns1->nick,k,ns2->nick,l); 
                                    tprintf(ns2->sock,"playernum %d\xff", k);
                                    ns2->gameslot=k;
                                    tprintf(ns1->sock,"playerjoin %d %s\xff", ns2->gameslot, ns2->nick);
                                    tprintf(ns2->sock,"playerjoin %d %s\xff", ns1->gameslot, ns1->nick);
                                  } 
                                else
                                  {
                                    lvprintf(4,"#%s-%s moves player %s(%d) to %s(%d)\n", n->channel->name,n->nick,ns1->nick,k,ns1->nick,l); 
                                    tprintf(ns1->sock,"playerleave %d\xff", k);
                                  }
                    
                                /* Now tell everyone else */
                                nsock=n->channel->net;
                                while (nsock!=NULL)
                                  {
                                    if ( (nsock->type == NET_CONNECTED) && (nsock!=ns1) && (nsock!=ns2) )
                                      {
                                        tprintf(nsock->sock,"playerjoin %d %s\xff", ns1->gameslot, ns1->nick);
                                        if (ns2!=NULL)
                                          { /* Player existed */
                                            tprintf(nsock->sock,"playerjoin %d %s\xff", ns2->gameslot,ns2->nick);
                                          }
                                        else
                                          {
                                            tprintf(nsock->sock,"playerleave %d\xff", k);
                                          }  
                                      }
                                    nsock=nsock->next;
                                  }
                              }
                          }
                        else
                          tprintf(n->sock,"pline 0 %c/move %c<playernumber> <newplayernumber>\xff", RED, BLUE);

                      }  
                    else
                      {
                        if (n->channel->status != STATE_ONLINE)
                          tprintf(n->sock,"pline 0 %cCommand unavailable while a game is in progress. Stop it first.\xff",RED);
                        else
                          tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                      }
                  }
                
            
                /* Take "ops" - Suggestion by (jawfx@hotmail.com 21/9/98) */
                if ( !strncasecmp(MSG, "/op", 3) && (game.command_op>0))
                  {
                    valid_param=2;
                    P=MSG+4;
                    if (securityread() < 0)
                      securitywrite();
                        
                    if ( (strlen(security.op_password) > 0) && !strcmp(P, security.op_password) )
                      { /* Passed, this player is OP */
                        n->securitylevel =LEVEL_AUTHOP;
                        tprintf(n->sock,"pline 0 %cYour security level is now: %cAUTHENTICATED OP\xff", GREEN, RED);
                        lvprintf(4,"#%s-%s authenticated successfully for op status\n", n->channel->name,n->nick);
                      }
                    else
                      {
                        tprintf(n->sock,"pline 0 Invalid Password! (Attempt logged)\xff");
                        lvprintf(1,"#%s-%s Failed attempt to gain OP status\n",  n->channel->name, n->nick);
                      }
                  }
            
                /* Winlist. Display the top X people - Suggestion by crazor */
                if ( !strncasecmp(MSG, "/winlist", 8) && (game.command_winlist>0) )
                  {
                    valid_param=2;
                    if (passed_level(n,game.command_winlist))
                      {
                        P=MSG+9;
                        if (strlen(MSG) == 8)
                          j=10;
                        else
                          j=atoi(P);
                        if ( (j >= 1) && (j <= MAXWINLIST) )
                          {
                            i=0;
                            tprintf(n->sock,"pline 0 %cTop %d Winlist\xff", BLUE, j);
                            while ( (i < j) && (winlist[i].inuse) )
                              {
                                if (winlist[i].status == 't')
                                  {
                                    if (!strcmp(n->team, winlist[i].name))
                                      k = RED;
                                    else
                                      k = BLACK;
                                    tprintf(n->sock,"pline 0 %c%d. %4d - Team %s\xff", k, i+1, winlist[i].score, winlist[i].name); 
                                  }
                                else
                                  {
                                    if (!strcmp(n->nick, winlist[i].name))
                                      k = RED;
                                    else
                                      k = BLACK;
                                    tprintf(n->sock,"pline 0 %c%d. %4d - Player %s\xff", k, i+1, winlist[i].score, winlist[i].name); 
                                  }
                                i++;
                              }
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
              
                if ( !strncasecmp(MSG, "/help", 5) && (game.command_help>0) )
                  {
                    valid_param=2;
                    if (passed_level(n,game.command_help))
                      {
                        tprintf(n->sock,"pline 0 V%s.%s - Built in server commands %c(*) %crequires 'op', %c(!) %crequires '/op'\xff", TETVERSION, SERVERBUILD, TEAL, BLACK, RED, BLACK);
                        if (game.command_clear>0)
                          {
                            STRG[0]=0;
                            switch(game.command_clear)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/clear\xff", RED);
                            tprintf(n->sock,"pline 0       %-4s %cClears the winlist\xff", STRG, BLACK);
                          }
                        if (game.command_join)
                          {
                            STRG[0]=0;
                            switch(game.command_join)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/join %c<#channel|channel number>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cJoins or creates a virtual tetrinet channel\xff", STRG, BLACK);
                          }
                        if (game.command_kick)
                          {
                            STRG[0]=0;
                            switch(game.command_kick)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/kick %c<playernumber(s)>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cKicks player(s) from the server\xff", STRG, BLACK);
                          }
                        if (game.command_list)
                          {
                            STRG[0]=0;
                            switch(game.command_list)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/list\xff", RED);
                            tprintf(n->sock,"pline 0       %-4s %cLists available virtual TetriNET channels\xff", STRG, BLACK);
                          }
                        tprintf(n->sock,"pline 0   %c/me %c<action>\xff", RED, BLUE);
                        tprintf(n->sock,"pline 0            Performs an action\xff");
                        if (game.command_move)
                          {
                            STRG[0]=0;
                            switch(game.command_move)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/move %c<playernumber> <new playernumber>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cMoves a player to a new playernumber\xff", STRG,BLACK);
                          }
                    
                        if (game.command_msg)
                          {
                            STRG[0]=0;
                            switch(game.command_msg)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/msg %c<playernumber(s)> <msg>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cPrivately messages player(s)\xff", STRG,BLACK);
                          }
                        if (game.command_op)
                          {
                            tprintf(n->sock,"pline 0   %c/op %c<op_password>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0            Gain AUTHENTICATED OP status\xff");
                          }
                        if (game.command_persistant)
                          {
                            STRG[0]=0;
                            switch(game.command_persistant)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/persistant %c<0/1>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cPersistant channels are not deleted when the last person leaves\xff", STRG, BLACK);
                          }
                        if (game.command_priority)
                          {
                            STRG[0]=0;
                            switch(game.command_priority)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/priority %c<channel priority(1-99)>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cNew players join the highest priority channel\xff", STRG, BLACK);
                          }
                        if (game.command_reset)
                          {
                            STRG[0]=0;
                            switch(game.command_reset)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/reset\xff", RED);
                            tprintf(n->sock,"pline 0       %-4s %cReload saved game configuration AND persistant channel config\xff", STRG, BLACK);
                          }
                        if (game.command_save)
                          {
                            STRG[0]=0;
                            switch(game.command_save)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/save\xff", RED);
                            tprintf(n->sock,"pline 0       %-4s %cAll game configuration AND persistant channel info is saved\xff", STRG, BLACK);
                          }
                        if (game.command_set)
                          {
                            STRG[0]=0;
                            switch(game.command_set)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/set\xff", RED);
                            tprintf(n->sock,"pline 0       %-4s %cSet Channel Config. Type %c/set %cHELP\xff", STRG, BLACK,BLUE,RED);
                          }
                        if (game.command_topic)
                          {
                            STRG[0]=0;
                            switch(game.command_topic)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/topic %c<channel topic>\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cChanges the topic of the virtual TetriNET channel\xff", STRG, BLACK);
                          }
                        if (game.command_who)
                          {
                            STRG[0]=0;
                            switch(game.command_who)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/who\xff", RED);
                            tprintf(n->sock,"pline 0       %-4s %cLists all logged in players, and what channel they're on\xff", STRG, BLACK);
                          }
                    
                        if (game.command_winlist)
                          {
                            STRG[0]=0;
                            switch(game.command_winlist)
                              {
                                case 2: /* Requires OP */
                                  {
                                    sprintf(STRG,"%c(*)", TEAL);
                                    break;
                                  }
                                case 3: /* Requires /OP */
                                  {
                                    sprintf(STRG,"%c(!)", RED);
                                    break;
                                  }
                              }
                            tprintf(n->sock,"pline 0   %c/winlist %c[n]\xff", RED, BLUE);
                            tprintf(n->sock,"pline 0       %-4s %cDisplays the top n players\xff",STRG,BLACK);
                          }
                      }
                    else
                      tprintf(n->sock,"pline 0 %cYou do NOT have access to that command!\xff",RED);
                  }
                if (valid_param==1)
                  {
                    tprintf(n->sock,"pline 0 %cInvalid /COMMAND!\xff",RED);
                  }
              }
            else
              {
                lvprintf(5,"#%s-%s: %s\n", n->channel->name,n->nick,MSG);
                nsock=n->channel->net;
                while (nsock!=NULL)
                  {
                    if ( (nsock!=n) && (nsock->type == NET_CONNECTED) )
                      {  /* Write line, after first resetting it to black (incase colour still exists from nick) */
                        tprintf(nsock->sock,"pline %d %c%s\xff", n->gameslot, BLACK, MSG); 
                      }
                    nsock=nsock->next;
                  }
              }
            valid_param=1;
          }
      }
      
    /* Party Line Act - plineact <playernumber> <message> */
    if ( !strcasecmp(COMMAND, "plineact") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %600[^\n\r]", &num, MSG);
        if ( (s >= 2) && (num==n->gameslot))
          {
            valid_param=1;
            lvprintf(5,"#%s-%s acts: %s\n",n->channel->name,n->nick,MSG);
            nsock=n->channel->net;
            while (nsock!=NULL)
              {
                if ( (nsock!=n) && (nsock->type == NET_CONNECTED))
                  {  /* Write out action, after first resetting colors */
                    tprintf(nsock->sock,"plineact %d %s\xff", n->gameslot, MSG); 
                  }
                nsock=nsock->next;
              }
          }
      }
      
    
    /* Change Team - team <playernumber> <teamname> */
    if ( !strcasecmp(COMMAND, "team") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %600[^\n\r]", &num, MSG);
        if ( (s >= 1) && (num==n->gameslot) && (n->status == STAT_NOTPLAYING))
          {
            valid_param=1;
            strncpy(n->team, MSG, TEAMLEN); n->team[TEAMLEN]=0;
            lvprintf(5,"#%s-%s changes team to %s\n",n->channel->name,n->nick,n->team);
            nsock=n->channel->net;
            while (nsock!=NULL)
              {
                if ( (n!=nsock) && (nsock->type == NET_CONNECTED) )
                  tprintf(nsock->sock,"team %d %s\xff", n->gameslot, MSG); 
                nsock=nsock->next;
              }
          }
      }
      
    /* Pause Game - pause <0|1> <playernumber> */
    if ( !strcasecmp(COMMAND, "pause") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %d", &num, &num2);
        if ( (s >= 2) && is_op(n) && (n->channel->status == STATE_INGAME) && (num2==n->gameslot) && ( (num==0)||(num==1)) )
          {
            if (num==1)
              lvprintf(4,"#%s-%s pauses game\n",n->channel->name,n->nick);
            else
              lvprintf(5,"#%2-%s unpauses game\n",n->channel->name,n->nick);
              
            
            valid_param=1;
            nsock=n->channel->net;
            while(nsock!=NULL)
              {
                if ( (nsock->type == NET_CONNECTED) && (nsock->channel==n->channel))
                  tprintf(nsock->sock,"pause %d\xff", num);
                nsock=nsock->next;
              }
            if (num==0)
              n->channel->status=STATE_PAUSED;
            else
              n->channel->status=STATE_INGAME;
          }
      }
      
    /* Game message - gmsg <message> */
    if ( !strcasecmp(COMMAND, "gmsg") )
      {
        valid_param=1;
        /* If it's just "t", then reply PONG to player only*/
        sprintf(MSG,"<%s> t", n->nick);
        if ( !strcasecmp(MSG, PARAM) && n->channel->pingintercept)
          {
            tprintf(n->sock,"gmsg * PONG\xff");
          }
        else
          {
            /* FIRST, get the NICK, and strip out color codes. Else it looks horrible */
            sprintf(MSG,"<%s>", n->nick);
            j=0;
            if ( PARAM[0] == '<' )
              { /* Yep, its a message. So save nick stripped of colour codes in MSG */
                j=strlen(MSG);
                MSG[0]=0;
                P=MSG;
                for(i=0;i<j;i++)
                  {
                    if ( n->channel->stripcolour
                      && (PARAM[i] != BOLD)
                      && (PARAM[i] != CYAN)
                      && (PARAM[i] != BLACK)
                      && (PARAM[i] != BLUE)
                      && (PARAM[i] != DARKGRAY)
                      && (PARAM[i] != MAGENTA)
                      && (PARAM[i] != GREEN)
                      && (PARAM[i] != NEON)
                      && (PARAM[i] != SILVER)
                      && (PARAM[i] != BROWN)
                      && (PARAM[i] != NAVY)
                      && (PARAM[i] != VIOLET)
                      && (PARAM[i] != RED)
                      && (PARAM[i] != ITALIC)
                      && (PARAM[i] != TEAL)
                      && (PARAM[i] != WHITE)
                      && (PARAM[i] != YELLOW)
                      && (PARAM[i] != UNDERLINE) )
                      {
                        *P = PARAM[i];
                        P++;
                      }
                  }
                *P='\0';
                P=PARAM+j+1;
                j=1;
              }
            lvprintf(5,"#%s-%s: %s\n", n->channel->name,n->nick,PARAM);
            nsock=n->channel->net;
            while (nsock!=NULL)
              {
                if ( (nsock->type == NET_CONNECTED) )
                  {
                    if (j)
                      tprintf(nsock->sock,"gmsg %s %s\xff", MSG, P);
                    else
                      tprintf(nsock->sock,"gmsg %s\xff", PARAM);
                  }
                nsock=nsock->next;
              }
          }
      }
      
    /* Player Lost - playerlost <playernumber> */
    if ( !strcasecmp(COMMAND, "playerlost") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %600[^\n\r]", &num, MSG);
        if ( (s >= 1) && ( (n->channel->status == STATE_INGAME) || (n->channel->status == STATE_PAUSED) ) && (num==n->gameslot) )
          {
            valid_param=1;
            /* Now, is this player actually playing? If not, we just ignore them */
            if (n->status == STAT_PLAYING)
              {
                /* Set player to be "Lost" */
                n->status = STAT_LOST;
                n->timeout = game.timeout_outgame;
                lvprintf(6,"#%s-%s lost\n",n->channel->name,n->nick);
                
                num2=0;    /* Assume no-one left playing */
                num3=0;	/* Player who is still in */
                MSG[0]=0;  /* Store playing team name here */
                /* Check if game is finished, and tell each person this player has lost */
                nsock=n->channel->net;
                ns1=NULL;
                while (nsock!=NULL)
                  {
                    if ( (nsock!=n) && (nsock->type == NET_CONNECTED) && (nsock->status == STAT_PLAYING))
                      {
                        tprintf(nsock->sock,"playerlost %d\xff",num);
                        if (strcasecmp(MSG,nsock->team))
                          { /* Different team, so add player */
                            num2++;
                            ns1=nsock;
                            strcpy(MSG,nsock->team);
                          }
                        else if (MSG[0]==0) 
                          {
                            num2++;
                            ns1=nsock;
                          }
                      }
                    nsock=nsock->next;
                  }
                if ( (num2 <= 1) && (ns1!=NULL) )
                  { /* 1 or less different teams playing. Stop the game, and take score */
                    if (ns1->type == NET_CONNECTED)
                      {
                        if (strlen(ns1->team) > 0)
                          { /* Team won, so add score to team */
                            updatewinlist(ns1->team,'t',3);
                          }
                        else
                          { /* Player won, so add score to player name */
                            updatewinlist(ns1->nick,'p',3);
                          }
                      }
                    n->channel->status=STATE_ONLINE;
                    nsock=n->channel->net;
                    while (nsock!=NULL)
                      {
                        if ( (nsock->type == NET_CONNECTED))
                          {
                            tprintf(nsock->sock,"endgame\xff");
                            tprintf(nsock->sock,"playerwon %d\xff", ns1->gameslot);
                            nsock->status=STAT_NOTPLAYING;
                            nsock->timeout=game.timeout_outgame;
                            /* Send every playing field */
                            
                            ns2=n->channel->net;
                            while (ns2!=NULL)
                              {
                                if ( (ns2!=nsock) && (ns2->type == NET_CONNECTED))
                                  sendfield(nsock,ns2);
                                ns2=ns2->next;
                              }
                        
                            /*!!! ADD-IN: Server anounces winner */
                            if ( (ns1->type == NET_CONNECTED) && n->channel->serverannounce)
                              {
                                if ( strlen(ns1->team) > 0)
                                  { /* a team won */
                                    tprintf(nsock->sock,"pline 0 ---- Team %s%c WON ----\xff", ns1->team, BLACK);
                                  }
                                else
                                  { /* a player won */
                                    tprintf(nsock->sock,"pline 0 ---- Player %s%c WON ----\xff", ns1->nick, BLACK);
                                  }
                              }
                          }
                        nsock=nsock->next;
                      }
                    writewinlist();
                    sendwinlist(n->channel,NULL);	/* Send to all */
                
                  }
              
              } 
            
          }
      }
      
    /* Field Update - f <field codes> */
    if ( !strcasecmp(COMMAND, "f") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %600[^\n\r]", &num, MSG);
        if ( (s >= 1) && (num==n->gameslot))
          {
            valid_param=1;
            /* First, transmit these changes as is to all other players */
            nsock=n->channel->net;
            while (nsock!=NULL)
              {
                if ( (nsock!=n) && (nsock->type == NET_CONNECTED))
                  tprintf(nsock->sock,"f %d %s\xff", n->gameslot, MSG); 
                nsock=nsock->next;
              }
              
            /* Now parse it ourselves, and update our internal knowledge of players field */
            parsefield(n, MSG);
          }
      }
      
    /* Level - lvl <playernumber> <current level> */
    if ( !strcasecmp(COMMAND, "lvl") )
      { 
        valid_param=2;
        s = sscanf(PARAM, "%d %d", &num, &num2);
        if ( (n->channel->status==STATE_INGAME) && (num==n->gameslot) && (n->status == STAT_PLAYING) )
          {
            valid_param=1;
            nsock=n->channel->net;
            while (nsock!=NULL)
              {
                if ( (nsock->type == NET_CONNECTED))
                  tprintf(nsock->sock,"lvl %d %d\xff", n->gameslot,num2);
                nsock=nsock->next;
              }
          }
      }
    
    /* Special Block Use - sb <to use on,0=all> <special block> <playernum> */
    if ( !strcasecmp(COMMAND, "sb") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %s %d", &num, MSG, &num2);
        if ( (s >= 3) && (n->channel->status == STATE_INGAME) && (num2==n->gameslot) && (n->status == STAT_PLAYING))
          {
            valid_param=1;
            nsock=n->channel->net;
            while (nsock!=NULL)
              {
                if ( (nsock!=n) && (nsock->type == NET_CONNECTED) )
                  {
                    tprintf(nsock->sock,"sb %d %s %d\xff", num, MSG, n->gameslot); 
                  }
                nsock=nsock->next;
              }
          }
      }
      
    /* Start/Stop Game - startgame <0/1, 0=stopgame> <playernumber> */
    if ( !strcasecmp(COMMAND, "startgame") )
      {
        valid_param=2;
        s = sscanf(PARAM, "%d %d %600[^\n\r]", &num, &num2, MSG);
        if ( (s >= 2) && is_op(n) && ( ((num==1) && (n->channel->status == STATE_ONLINE)) || ((num==0) && (n->channel->status == STATE_INGAME)) ) && (num2==n->gameslot) && ( (num == 1) || (num == 0)) )
          {
            valid_param=1;
            if (num==1)
              {
                n->channel->status = STATE_INGAME;
                lvprintf(4,"#%s-%s started the game\n", n->channel->name,n->nick);
              }
            else
              {
                n->channel->status = STATE_ONLINE;
                lvprintf(4,"#%s-%s stopped the game\n", n->channel->name,n->nick);
                n->channel->sd_mode=SD_NONE;
              }
              
            /* Read in Game config and winlist, in case they were changed */
/*            gameread();*/
            readwinlist();
            
            /* Set the SuddenDeath timeout, if any */
            if (n->channel->sd_timeout>0)
              {
                n->channel->sd_mode=SD_INIT;
                n->channel->sd_timeleft=n->channel->sd_timeout;
              }
            nsock=n->channel->net;
            while (nsock!=NULL)
              {/* Send to every player the game parameters */
                if ( (nsock->type == NET_CONNECTED))
                  {
                    if (num == 1) /* Start game */
                      {
                        
                        tprintf(nsock->sock,"newgame 0 ");
                        tprintf(nsock->sock,"%d %d %d %d %d %d ", 
                                  n->channel->starting_level, n->channel->lines_per_level,
                                  n->channel->level_increase, n->channel->lines_per_special,
                                  n->channel->special_added, n->channel->special_capacity);
                        for(j=0;j<n->channel->block_leftl;j++) tprintf(nsock->sock,"3");
                        for(j=0;j<n->channel->block_leftz;j++) tprintf(nsock->sock,"5");
                        for(j=0;j<n->channel->block_square;j++)tprintf(nsock->sock,"2");
                        for(j=0;j<n->channel->block_rightl;j++)tprintf(nsock->sock,"4");
                        for(j=0;j<n->channel->block_rightz;j++)tprintf(nsock->sock,"6");
                        for(j=0;j<n->channel->block_halfcross;j++) tprintf(nsock->sock,"7");
                        for(j=0;j<n->channel->block_line;j++)  tprintf(nsock->sock,"1");
                        tprintf(nsock->sock," ");
                        for(j=0;j<n->channel->special_addline;j++) tprintf(nsock->sock,"1");
                        for(j=0;j<n->channel->special_clearline;j++) tprintf(nsock->sock,"2");
                        for(j=0;j<n->channel->special_nukefield;j++) tprintf(nsock->sock,"3");
                        for(j=0;j<n->channel->special_randomclear;j++) tprintf(nsock->sock,"4");
                        for(j=0;j<n->channel->special_switchfield;j++) tprintf(nsock->sock,"5");
                        for(j=0;j<n->channel->special_clearspecial;j++) tprintf(nsock->sock,"6");
                        for(j=0;j<n->channel->special_gravity;j++) tprintf(nsock->sock,"7");
                        for(j=0;j<n->channel->special_quakefield;j++) tprintf(nsock->sock,"8");
                        for(j=0;j<n->channel->special_blockbomb;j++) tprintf(nsock->sock,"9");
                        tprintf(nsock->sock," %d %d\xff", n->channel->average_levels, n->channel->classic_rules);
                        
                        /* And set them to be playing */
                        nsock->status = STAT_PLAYING;
                        
                        /* Give them new timeouts */
                        nsock->timeout = game.timeout_ingame;
                        
                        /* BLANK *ALL* the fields and send them to each player */
                        /* Clear their Field */
                        for(y=0;y<FIELD_MAXY;y++)
                          for(x=0;x<FIELD_MAXX;x++)
                            nsock->field[x][y]=0; /* Nothing */
                                
                        /* Send to every other player */
                        ns1=n->channel->net;
                        while (ns1!=NULL)
                          {
                            if ( (ns1!=nsock) && (ns1->type==NET_CONNECTED) )
                              {
                                sendfield(ns1,nsock);
                              }
                            ns1=ns1->next;
                          }
                      }
                    else  /* End Game */
                      {
                        tprintf(nsock->sock,"endgame\xff");
                        /* And set them to not be playing */
                        nsock->status = STAT_NOTPLAYING;
                        nsock->timeout = game.timeout_outgame;
                      }
                  }
                nsock=nsock->next;
              }
          }
      }
    
      
      
    /* Log invalid params */
    if (valid_param==0)
      lvprintf(1,"#%s-%s - Invalid Command - %s\n", n->channel->name,n->nick, buf);
  }

/* The player has sent their inital team name, well they should have anyway */
void net_waitingforteam(struct net_t *n, char *buf)
  {
    FILE *file_in;
    char strg[1024];
    char *P;
    struct net_t *nsock;
    
    sprintf(strg,"team %d ",n->gameslot);
    if (strncmp(buf,strg,strlen(strg)))
      { /* Incorrect, Kill this player, they never existed ;) */
        lvprintf(1,"Incorrect TEAM statement - %s!\n",buf);
        killsock(n->sock); lostnet(n);
        return;
      }
    
    P=buf+strlen(strg);
    n->type=NET_CONNECTED;
    n->status=STAT_NOTPLAYING;
    strncpy(n->team, P, TEAMLEN); n->team[TEAMLEN]='\0';
     
    nsock=n->channel->net;
    while (nsock!=NULL)
      {
        if ( (nsock != n) && (nsock->type==NET_CONNECTED))
          { 
            /* Send each other player and their team to this player */
            tprintf(n->sock, "playerjoin %d %s\xffteam %d %s\xff", nsock->gameslot, nsock->nick, nsock->gameslot, nsock->team);
          
            /* Send to the other player, this player and their team */
            tprintf(nsock->sock, "playerjoin %d %s\xffteam %d %s\xff", n->gameslot, n->nick, n->gameslot, n->team);
          }
        nsock=nsock->next;
      }
    
    /* Now for the game.motd if it exists. */
    file_in = fopen(FILE_MOTD,"r");
    if (file_in != NULL)
      {/* Exists, so send it to the player */
        while (!feof(file_in))
          {
            if(fscanf(file_in,"%1023[^\n]\n", strg) != 1)
            {
              printf("Error: Failed to read MOTD file: %s.\n",FILE_MOTD);
            }
            tprintf(n->sock,"pline 0 %s\xff", strg);
          }
        fclose(file_in);
      }
      
    /* If game is in progress, send all other players fields */
    /* Tell each other player that we are lost... no really :P */
    if( (n->channel->status == STATE_INGAME) || (n->channel->status == STATE_PAUSED) )
      {
        nsock=n->channel->net;
        while (nsock!=NULL)
          {
            if( (nsock->type==NET_CONNECTED) &&(nsock!=n) )
              {
                /* Each player who has lost, send player_lost */
                if (nsock->status != STAT_PLAYING)
                  {
                    tprintf(n->sock,"playerlost %d\xff", nsock->gameslot);
                  }
                sendfield(n, nsock); /* Send to player, this players field */

                /* And tell this player that the newjoined player has lost */
                tprintf(nsock->sock,"playerlost %d\xff", n->gameslot);
              }
            nsock=nsock->next;
          }
      }
      
    /* If we are ingame, then tell this person that we are! */
    if ( (n->channel->status == STATE_INGAME) || (n->channel->status == STATE_PAUSED) )
      tprintf(n->sock,"ingame\xff");
      
    /* If we are currently paused, kindly let them know that fact to */
    if ( n->channel->status == STATE_PAUSED )
      tprintf(n->sock,"pause 1\xff");  
    
    /* And tell them their channel */
    tprintf(n->sock,"pline 0 %c%s%c %chas joined channel #%s\xff", GREEN,n->nick,BLACK,GREEN,n->channel->name);
    
    lvprintf(2,"#%s-%s New connection\n", n->channel->name,n->nick);
  }

/* Recieving Query commands */
void net_query_init(struct net_t *n, char *buf)
  {
    killsock(n->sock);lostnet(n);
    return;
  }

/* The person has sent their init string */
void net_telnet_init(struct net_t *n, char *buf)
  {
    int gameslot,found,i;
    int tet_err;
    char *dec;
    struct channel_t *chan;
    struct net_t *nsock;
    
    /* Work out game slot */
    gameslot=0; found=1;
    while ( (gameslot < n->channel->maxplayers) && (found) )
      {
        gameslot++;
        found=0;
        nsock=n->channel->net;
        while (nsock!=NULL)
          {
            if( ( (nsock->type==NET_CONNECTED) || (nsock->type==NET_WAITINGFORTEAM) )&&(nsock->gameslot==gameslot) ) found=1;
            nsock=nsock->next;
          }
      }

    if (found)
      { /* No free Gameslots, so tell that the server is full!! */
        tprintf(n->sock,"noconnecting Server is Full!\xff");
        lvprintf(9,"%s: Disconnected because of FULL server.\n",n->host);
        killsock(n->sock); lostnet(n); return;
      }
    
    /* Set Game Socket */
    n->gameslot = gameslot;
  
    lvprintf(10,"INIT: %s\n", buf);

    /* If init string is in fact "playerquery", return number of logged in players, and kill them */
    if (!strcasecmp(buf, "playerquery"))
      {
        lvprintf(9,"%s: Playerquery request\n",n->host);
        tprintf(n->sock, "Number of players logged in: %d\n", numplayers(n->channel));
        return;
      }
      
    /* Version - Return Full version of server */
    if (!strcasecmp(buf, "version"))
      {
        lvprintf(9,"%s: Version request\n",n->host);
        tprintf(n->sock,"%s.%s\n+OK\n", TETVERSION, SERVERBUILD);
        return;
      }
      
    /* ListChan - List of all channels */
    if (!strcasecmp(buf, "listchan"))
      {
        lvprintf(9,"%s: ListChan request\n",n->host);
        chan=chanlist;
        while(chan!=NULL)
          {
            tprintf(n->sock,"\"%s\" \"%s\" %d %d %d %d\n",chan->name, chan->description, numplayers(chan), chan->maxplayers, chan->priority, chan->status);
            chan=chan->next;
          }
        tprintf(n->sock,"+OK\n");
        return;
      }
      
    /* Listuser - List of all users */
    if (!strcasecmp(buf,"listuser"))
      {
        lvprintf(9,"%s: ListUser request\n",n->host);
        chan=chanlist;
        while (chan!=NULL)
          {
            nsock=chan->net;
            while (nsock!=NULL)
              {
                if(nsock->type==NET_CONNECTED)
                  {
                    tprintf(n->sock,"\"%s\" \"%s\" \"%s\" %d %d %d \"%s\"\n",nsock->nick, nsock->team, nsock->version,nsock->gameslot,nsock->status,nsock->securitylevel,nsock->channel->name);
                  }
                nsock=nsock->next;
              }
            chan=chan->next;
          }
        tprintf(n->sock,"+OK\n");
        return;
      }
      
    /* Winlist - Returns Winlist */
    if (!strcasecmp(buf,"getwinlist"))
      {
        lvprintf(4,"%s: GetWinlist request\n",n->host);
        for(i=0;i<MAXWINLIST;i++)
          {
            if(winlist[i].inuse)
              {
                tprintf(n->sock,"\"%s\" %c %lu\n", winlist[i].name,winlist[i].status,winlist[i].score);
              }
          }
        tprintf(n->sock,"+OK\n"); 
        return;
      }
      
    if (strlen(buf) < 7)
      {/* Invalid... quit */
        lvprintf(9,"%s: Disconnected due to invalid command (This can be normal)\n",n->host);
        killsock(n->sock);lostnet(n);
        return;
        
      }
      
    
    
    tet_err = tet_decrypt(buf);
    if (tet_err != 0)
      { /* Error */
        lvprintf(9,"%s: Disconnected due to failed decryption: %s\n",n->host,buf);
        killsock(n->sock); lostnet(n);
        return;
      }
     
    dec = tet_dec2str(buf);
    
    lvprintf(10,"DECR: %s\n", dec);

    /* OK, so dec should now hold "tetrisstart <nickname> <version number> */
    i = sscanf(dec,"tetrisstart %30[^\x20] %10s", n->nick, n->version);
    
        
    if (i < 2)
      {/* To few conversions - Player dies*/
        lvprintf(9,"%s: Disconnected due to invalid tetrisstart: %s\n",n->host,dec);
        killsock(n->sock); lostnet(n);
      }
      
    /* Ensure a valid nickname */
    if (!strcasecmp(n->nick,"server"))
      {
        lvprintf(9,"%s Disconnected due to invalid nickname: %s\n",n->host,n->nick);
        tprintf(n->sock, "noconnecting Nickname %s not allowed!\xff", n->nick);
        killsock(n->sock); lostnet(n);
      }
    
    /* Ensure that Version is OK */
    if (tet_checkversion(n->version) == -1)
      {
        tprintf(n->sock, "noconnecting TetriNET version (%s) does not match Server's (%s)!\xff", n->version, TETVERSION);
        lvprintf(9,"%s: Client version (%s) not allowed!\n", n->nick, n->version);
        killsock(n->sock); lostnet(n);
        return;
      }
    
    /* Ensure that no-one else has this nick */
    found=0;
    chan=chanlist;
    while( (chan!=NULL) && !found)
      {
        nsock=chan->net;
        while ((nsock!=NULL) && !found)
          {
            if( ((nsock->type==NET_CONNECTED) || (nsock->type==NET_WAITINGFORTEAM) )&&(nsock!=n)&&(!strcasecmp(nsock->nick, n->nick)) ) found=1;
            nsock=nsock->next;
          }
        chan=chan->next;
      }
    if (found)
      {
        tprintf(n->sock, "noconnecting Nickname already exists on server!\xff");
        lvprintf(9,"%s: Disconnected since nickname already exists on server: %s\n",n->host,n->nick);
        killsock(n->sock); lostnet(n);
        return;        
      }

    n->team[0] = 0; /* Clear Team */
    
    /* Now waiting for team */
    n->type = NET_WAITINGFORTEAM;
    
    /* Send Winlist to this new arrival */
    sendwinlist(n->channel,n);

    /* Send them their player number */
    tprintf(n->sock, "playernum %d\xff",gameslot);
    
    
  }

/* Someone has just connected. So lets answer them */
void net_telnet(struct net_t *n, char *buf)
  {
    unsigned long ip; int k,l; char s[121]; char strg[121];
    char n1[4], n2[4], n3[4], n4[4];
    struct channel_t *chan, *ochan;
    struct net_t *net;
    int x,y;


    net=malloc(sizeof(struct net_t));
    net->next=NULL;
    
    net->sock=answer(n->sock,s,&ip,0);
    
    while ((net->sock==(-1)) && (errno==EAGAIN))
      net->sock=answer(n->sock,s,&ip,0);
    /* Find a channel */
    chan = chanlist;
    ochan = NULL;
    while ( chan != NULL )
      {
        if ( ((ochan == NULL) || (chan->priority > ochan->priority)) && (numplayers(chan) < chan->maxplayers) && (chan->priority!=0))
          ochan=chan; /* Found a likely channel */
        chan=chan->next;
      }


    /* Save the port stuff */
    net->addr=ip;
    net->port=n->port;
    net->timeout=game.timeout_outgame;
    net->securitylevel=LEVEL_NORMAL;
    net->status=STAT_NOTPLAYING;
    sprintf(net->host,"%s", s);
    if (strlen(s) == 0)
      { /* No resolved host... copy IP */
        sprintf(n1,"%lu", (unsigned long)(n->addr&0xff000000)/(unsigned long)0x1000000);
        sprintf(n2,"%lu", (unsigned long)(n->addr&0x00ff0000)/(unsigned long)0x10000);
        sprintf(n3,"%lu", (unsigned long)(n->addr&0x0000ff00)/(unsigned long)0x100);
        sprintf(n4,"%lu", (unsigned long)n->addr&0x000000ff);
        sprintf(net->host,"%s.%s.%s.%s",n1,n2,n3,n4);
      }
      
      
    
    if(net->sock <0)
      {
        lvprintf(4,"Failed TELNET incoming connection from %s", s);
        killsock(net->sock);
        free(net);
        return;
      }
      
    /* Is this person banned? */
    if (is_banned(net))
      {
        tprintf(net->sock,"noconnecting You are banned from server!\xff");
        killsock(net->sock);
        free(net);
        return;
      }

    if (ochan == NULL)
      { /* No channels found, so create a new one :P */
        if (numchannels() < game.maxchannels)
          {
            chan=chanlist;
            while ( (chan!=NULL) && (chan->next!=NULL) ) chan=chan->next;
            
            
            if (chan==NULL)
              {
                chanlist = malloc(sizeof(struct channel_t));
                chan=chanlist;
              }
            else
              {
                chan->next = malloc(sizeof(struct channel_t));
                chan=chan->next;
              }
              
            chan->next=NULL;
            chan->net=NULL;
            
            
            
            chan->maxplayers=DEFAULTMAXPLAYERS;
            chan->status=STATE_ONLINE;
            chan->description[0]=0;
            chan->priority=DEFAULTPRIORITY;
            chan->sd_mode=SD_NONE;
            chan->persistant=0;
            
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
  
  
            k=0;l=1;
            while (l)
              {
                k++;
                ochan=chanlist;
                if (k==1)
                  {
                    sprintf(strg,"%s", DEFAULTCHANNEL);
                    strncpy(chan->name,strg,CHANLEN-1); chan->name[CHANLEN-1]=0;
                  }
                else
                  {
                    sprintf(strg,"%s%d", DEFAULTCHANNEL,k);
                    strncpy(chan->name,strg,CHANLEN-1); chan->name[CHANLEN-1]=0;
                  }
                l=0;
                while( (ochan != NULL) && (!l) )
                  {
                    if ( (!strcasecmp(chan->name,ochan->name)) && (chan!=ochan))
                      l=1;
                    else
                      ochan=ochan->next;
                  }
              }
  
          }
        else
          {
            lvprintf(4,"Server FULL - Denying\n");
            tprintf(net->sock,"noconnecting Server is Full!\xff");
            killsock(net->sock);  
            free(net);
            return;
          }
      }
    else
      {
        chan=ochan; /* Found a channel */
      }    
    net->channel = chan;
    addnet(chan,net);
    
    net->type=NET_TELNET_INIT;
    strcpy(net->nick,"???");
    
    /* Clear their Field */
    for(y=0;y<FIELD_MAXY;y++)
      for(x=0;x<FIELD_MAXX;x++)
        net->field[x][y]=0; /* Nothing */
    
    lvprintf(9,"Incoming Telnet connection: %s\n", net->host);
    /* Now we wait for the client init string */
  }
  
  

/* Someone has just connected. So lets answer them */
void net_query(struct net_t *n, char *buf)
  {
  }

void lostnet(struct net_t *n)
  {
    int found,playing;
    char MSG[TEAMLEN+4];
    struct net_t *nsock;
    
    struct channel_t *chan,*ochan;
    
    /* Log the lost connection */
    if (n->type == NET_CONNECTED)
      {
        
        lvprintf(4,"#%s-%s Lost telnet connection from %s\n", n->channel->name, n->nick, n->host);
      }
    /* Inform all other active players this one has left iff this one was connected "properly"! */
    if ( n->type == NET_CONNECTED )
      {
        playing=0;
        found=0;/* Number of different teams/players playing */
        MSG[0]=0; /* Store playing team name here */
        nsock=n->channel->net;
        while (nsock!=NULL)
          {
            if ( (nsock!=n) && (nsock->type == NET_CONNECTED) )
              { /* Different player, connected, in same channel */
                tprintf(nsock->sock,"playerleave %d\xff", n->gameslot);
                
                if (strcasecmp(MSG,nsock->team))
                  {/* Different team, so add player */
                    found++;
                    if (nsock->status == STAT_PLAYING) playing++;
                    strcpy(MSG,nsock->team);
                  }
                else if (MSG[0]==0) 
                  {
                    found++;
                    if (nsock->status == STAT_PLAYING) playing++;
                  }
              }
            nsock=nsock->next;
          }
        
        /* If 1 or less players/teams now are playing, AND the player that quit WAS playing, STOPIT */
        if ( (playing <= 1) && (n->channel->status == STATE_INGAME) && (n->status == STAT_PLAYING) )
          {
            nsock=n->channel->net;
            while (nsock!=NULL)
              {
                if ( (nsock!=n) && (nsock->type == NET_CONNECTED))
                  {
                    tprintf(nsock->sock,"endgame\xff");
                    nsock->status=STAT_NOTPLAYING;
                    nsock->timeout=game.timeout_outgame;
                  }
                nsock=nsock->next;
              }
            n->channel->status = STATE_ONLINE;
          }
      }
          
    /* If we're the only numplayers players, then we delete the channel */
    if ( (numallplayers(n->channel)==1) && (!n->channel->persistant) )
      {
        chan=chanlist;
        ochan=NULL;
        while ( (chan != n->channel) && (chan != NULL) ) 
          {
            ochan=chan;
            chan=chan->next;
          }
        if (chan != NULL)
          {
            if (ochan != NULL)
              ochan->next=chan->next;
            else
              chanlist=chan->next;
            free(chan); 
          }
      }
    else
      remnet(n->channel,n);
      
    free(n);
  }

void net_eof(int z)
  {
    struct channel_t *chan;
    struct net_t *nsock;		
    
    nsock=NULL;
    chan=chanlist;
    while ( (chan!=NULL) && (nsock==NULL))
      {
        nsock=chan->net;
        while ((nsock!=NULL) && (nsock->sock!=z))
          {
            nsock=nsock->next;
          }
        chan=chan->next;
      }
    if (nsock==NULL)
      {
        lvprintf(3,"Socket(%d): EOF on unknown socket\n",z);
        close(z); killsock(z);
        return;
      }
      

    killsock(z); lostnet(nsock); 
  }
  
void got_term(int z)
  {
    struct net_t *nsock;
    struct channel_t *chan;

    lvprintf(1,"Got TERM Signal - Quitting\n");
    /* Kill all our socks */
    nsock=NULL;
    chan=chanlist;
    while (chan!=NULL)
      {
        nsock=chan->net;
        while (nsock!=NULL)
          {
            if (nsock->type != NET_FREE)
              {
                killsock(nsock->sock);
                lostnet(nsock);
              }
            nsock=nsock->next;
          }
        chan=chan->next;
      }
      
    /* Write winlist etc... */
    writewinlist();

    /* Remove PID */
    delpid();
    
    /* Done */
    exit(0); 
  }

void init_main(void)
  {
    struct sigaction sv;
    
    lvprintf(0,"\nTetriNET for Linux V%s.%s\n---------------------------------\n", TETVERSION, SERVERBUILD);
    

    gnet=NULL;
    chanlist=NULL;
    
    /* set up error traps: We DON'T want Broken pipes!!! In fact, what the hell ARE broken pipes anyway. */
    sv.sa_handler=SIG_IGN; sigaction(SIGPIPE,&sv,NULL);
    
    /* We want to shut down cleanly */
    sv.sa_handler=got_term; sigaction(SIGTERM,&sv,NULL);
  


  }

void check_timeouts(void)
  {
    struct net_t *n;
    struct channel_t *chan;
    int found,i;
    
    
    found=0;
    chan=chanlist;
    while ( (chan!=NULL) )
      {
        n=chan->net;
        while ( (n!=NULL) && !found)
          {
            if (n->timeout > 0) n->timeout--;
            if (!n->timeout)
              { /* Timeout has occurred */
            
                switch (n->type)
                  {
                    case NET_TELNET_INIT:
                    case NET_WAITINGFORTEAM:
                    case NET_CONNECTED:
                    case NET_QUERY_INIT:
                      {
                        lvprintf(4,"#%s-%s: Timed out!\n", n->channel->name,n->nick);
                        tprintf(n->sock,"pline 0 %cYou have timed out! Disconnecting!\xff",RED);
                        killsock(n->sock);
                        lostnet(n);
                        found=1;
                        break;
                      }
                  }
              }
            if (!found) n=n->next;
          }
        if (!found) 
          chan=chan->next;
        else
          {
            chan=chanlist;
            found=0;
          }
      }
   
    /* Sudden Death Timeouts */
    chan=chanlist;
    while (chan!=NULL)
      {
        if ( (chan->status==STATE_INGAME) && (chan->sd_mode!=SD_NONE) )
          {
            chan->sd_timeleft--;
            if(chan->sd_timeleft <=0)
              {/* Timeout */
                n=chan->net;
                while(n!=NULL)
                  {
                    if ((n->type==NET_CONNECTED) )
                      {
                        if (chan->sd_mode==SD_INIT)
                          {
                            tprintf(n->sock,"gmsg %s\xff",n->channel->sd_message);
                          }
                        else
                          {
                            for(i=0;i<n->channel->sd_lines_per_add;i++)
                              tprintf(n->sock,"sb 0 a 0\xff");
                          }
                      }
                    n=n->next;
                  }
                chan->sd_timeleft=chan->sd_secs_between_lines;
                chan->sd_mode=SD_WAIT;
              }
          }
        chan=chan->next;
      }
  }
  


int main(int argc, char *argv[])
  {
    int xx;
    char buf[1050];
    int buf_len;
    int forknum;
    long int timeticks, otimeticks;
    
 
    /* Initialise */
    init_main();
    init_game();
    init_net();
    init_telnet_port();
    /*init_query_port();*/
    init_winlist();
    init_security();
    readwinlist(); 
    
    if (securityread() < 0)
      securitywrite();
    
    /* Now fork out, and start a new process group */
    /* Fork. If we are the parent, quit, if child, continue */
    if ((forknum=fork()) == -1)
      {
        printf("Error: Unable to fork new process\n");
        exit(5);
      }
    if (forknum > 0) 
      {
        exit(0);
      }
    setsid();
    
    /* Close all stdio */
    close(0);
    close(1);
    close(2);
    
    /* Write out PID */
    writepid();                                      
    
    /* Reset time */
    timeticks = time(NULL);
    otimeticks = timeticks;

    while (1)
      {
        
        timeticks=time(NULL);
        if ((timeticks-otimeticks) > CYCLE)
          { /* Check timeouts */
            check_timeouts();
          }

        /* flush sockets */
        dequeue_sockets();
        
        /* Read data from a currently waiting socket */
        xx=sockgets(buf, &buf_len);
        if (xx>=0)  /* Non-error */
          {
            net_activity(xx, buf, buf_len);
          }
        else if (xx==-1)        /* EOF from someone */
          {
            lvprintf(9,"Close Socket\n");
            net_eof(buf_len);	/* Close this socket */
          }
        else if (xx==-2)	/* Select() error */
          {
            lvprintf(4,"Select error - Whatever the hell that is supposed to be..\n");
          }
          
        
      }
  }