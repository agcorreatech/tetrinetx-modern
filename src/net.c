/*
  net.c

  - Updated : Completely dynamic allocation now (Brendan Grieve)
*/


/* i read somewhere that memcpy() is broken on some machines */
/* it's easy to replace, so i'm not gonna take any chances, because it's
*/
/* pretty important that it work correctly here */
void my_memcpy(dest,src,len)
char *dest,*src; int len;
{
  while (len--) *dest++=*src++;
}

/* bzero() is bsd-only, so here's one for non-bsd systems */
void my_bzero(dest,len)
char *dest; int len;
{
  while (len--) *dest++=0;
}

/* initialize the socklist */
void init_net()
{

  socklist=NULL;
}


int expmem_net()
{
  int tot=0;
  struct sock_list *slist;
  
  slist=socklist;
  while(slist!=NULL)
    {
      if (!(slist->flags & SOCK_UNUSED)) 
        {
          if (slist->inbuf != NULL) tot+=strlen(slist->inbuf)+1;
          if (slist->outbuf != NULL) tot+=strlen(slist->outbuf)+1;
        }
      slist=slist->next;
    }
  return tot;
}

/* puts full hostname in s */
void getmyhostname(s)
char *s;
{
  struct hostent *hp; char *p;
 
  p=getenv("HOSTNAME"); if (p!=NULL) {
    strcpy(s,p); if (strchr(s,'.')!=NULL) return;
  }
  gethostname(s,80);
  if (strchr(s,'.')!=NULL) return;
  hp=gethostbyname(s);
  if (hp==NULL)
    fatal("Hostname self-lookup failed.",0);
  strcpy(s,hp->h_name);
  if (strchr(s,'.')!=NULL) return;
  if (hp->h_aliases[0] == NULL)
    fatal("Can't determine your hostname!",0);
  strcpy(s,hp->h_aliases[0]);
  if (strchr(s,'.')==NULL)
    fatal("Can't determine your hostname!",0);   
}

/* get my ip number */
IP getmyip()
{
  struct hostent *hp; char s[121]; IP ip; struct in_addr *in;

  gethostname(s,120);
  hp=gethostbyname(s);

  if (hp==NULL) fatal("Hostname self-lookup failed.",0);
  in=(struct in_addr *)(hp->h_addr_list[0]);
  ip=(IP)(in->s_addr);
  return ip;
}

void neterror(s)
char *s;
{
  switch(errno) {
  case EADDRINUSE:
    strcpy(s,"Address already in use"); break;
  case EADDRNOTAVAIL:
    strcpy(s,"Address invalid on remote machine"); break;
  case EAFNOSUPPORT:
    strcpy(s,"Address family not supported"); break;
  case EALREADY:
    strcpy(s,"Socket already in use"); break;
  case EBADF:
    strcpy(s,"Socket descriptor is bad"); break;
  case ECONNREFUSED:
    strcpy(s,"Connection refused"); break;
  case EFAULT:
    strcpy(s,"Namespace segment violation"); break;
  case EINPROGRESS:
    strcpy(s,"Operation in progress"); break;
  case EINTR:
    strcpy(s,"Timeout"); break;
  case EINVAL:
    strcpy(s,"Invalid namespace"); break;
  case EISCONN:
    strcpy(s,"Socket already connected"); break;
  case ENETUNREACH:
    strcpy(s,"Network unreachable"); break;
  case ENOTSOCK:
    strcpy(s,"File descriptor, not a socket"); break;
  case ETIMEDOUT:
    strcpy(s,"Connection timed out"); break;
  case ENOTCONN:
    strcpy(s,"Socket is not connected"); break;
  case EHOSTUNREACH:
    strcpy(s,"Host is unreachable"); break;
  case EPIPE:
    strcpy(s,"Broken pipe"); break;
#ifdef ECONNRESET
  case ECONNRESET:
    strcpy(s,"Connection reset by peer"); break;
#endif
#ifdef EACCES
  case EACCES:
    strcpy(s,"Permission denied"); break;
#endif 
  case 0:
    strcpy(s,"Error 0"); break;
  default:
    sprintf(s,"Unforseen error %d",errno); break;
  }
}

/* request a normal socket for i/o */
void setsock(sock,options)
int sock,options;
{
  int parm;
  struct sock_list *slist;
  
  slist= malloc(sizeof(struct sock_list));
  slist->next=socklist;
  socklist=slist;
  
  socklist->inbuf=socklist->outbuf=NULL;
  socklist->flags=options;
  socklist->sock=sock;
  if ( !(socklist->flags &SOCK_NONSOCK)) {
      parm=1; setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,(void *)&parm,sizeof(int));
      parm=0; setsockopt(sock,SOL_SOCKET,SO_LINGER,(void *)&parm,sizeof(int));
    }
  if (options & SOCK_LISTEN) {
    /* Tris says this lets us grab the same port again next time */
    parm=1; setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(void *)&parm,
                       sizeof(int));
  }
  /* yay async i/o ! */
  fcntl(sock,F_SETFL,O_NONBLOCK);     
  return;
}


int getsock(options)
int options;
{
  int sock=socket(AF_INET,SOCK_STREAM,0);
  if (sock<0) fatal("Can't open a socket at all!",0);
  setsock(sock,options); return sock;
}

/* done with a socket */
void killsock(sock)
int sock;
{
  struct sock_list *slist,*oslist;
  dequeue_sockets();
  
  oslist=NULL;
  slist=socklist;
  while (slist!=NULL) {
    
    if (slist->sock==sock) {
      close(slist->sock);
      if (slist->inbuf != NULL) nfree(slist->inbuf);
      if (slist->outbuf != NULL) nfree(slist->outbuf);
      slist->flags=SOCK_UNUSED;
      if (oslist == NULL)
        {
          socklist=slist->next;
          free(slist);
        }
      else
        {
          oslist->next=slist->next;
          free(slist);
        }
      return;
    }
    oslist=slist;
    slist=slist->next;
  }
}

/* Connects and binds to a specific port+address */
/* Returns -1 if fail, or port # is return in port, and returns socket number */
int open_listen_socket(port,bindip)
int *port; char *bindip;
{
  int sock; struct sockaddr_in name; socklen_t addrlen;

  sock=getsock(SOCK_LISTEN);
  my_bzero((char *)&name,sizeof(struct sockaddr_in));
  name.sin_family=AF_INET;
  name.sin_port=htons(*port);   /* 0 = just assign us a port */
  name.sin_addr.s_addr=getip(bindip);
  if (bind(sock,(struct sockaddr *)&name,sizeof(name))<0) {
printf("ERROR\n");
    killsock(sock); return -1;
  }
  /* what port are we on? */
  addrlen=sizeof(name);
  if (getsockname(sock,(struct sockaddr *)&name,&addrlen)<0) {
printf("ERROR\n");
    killsock(sock); return -1;
  }
  *port=ntohs(name.sin_port);
  if (listen(sock,5)<0) { printf("Erk\n"); killsock(sock); return -1; }
  return sock;
}

/* returns a socket number for a listening socket that will accept any */
/* connection -- port # is returned in port */
int open_listen(port)
int *port;
{
  return(open_listen_socket(port,"0.0.0.0"));

}

/* given network-style IP address, return hostname */
/* hostname will be "##.##.##.##" format if there was an error */
char *hostnamefromip(ip)
unsigned long ip;
{
  struct hostent *hp; unsigned long addr=ip;
  unsigned char *p; static char s[121];
/*  alarm(10);*/
  hp=gethostbyaddr((char *)&addr,sizeof(addr),AF_INET); /*alarm(0);*/
  if (hp==NULL) {
    p=(unsigned char *)&addr;
    sprintf(s,"%u.%u.%u.%u",p[0],p[1],p[2],p[3]);
    return s;
  }
  strcpy(s,hp->h_name); return s;
}

/* short routine to answer a connect received on a socket made previously
*/
/* by open_listen ... returns hostname of the caller & the new socket */
/* does NOT dispose of old "public" socket! */
int answer(sock,caller,ip,binary)
int sock; char *caller; unsigned long *ip; int binary;
{
  int new_sock; struct sockaddr_in from; socklen_t addrlen;
  addrlen=sizeof(struct sockaddr);
  new_sock=accept(sock,(struct sockaddr *)&from,&addrlen);
  if (new_sock<0) return -1;
  *ip=from.sin_addr.s_addr;
  strcpy(caller,hostnamefromip(*ip));
  *ip=ntohl(*ip);
  /* set up all the normal socket crap */
  setsock(new_sock,(binary ? SOCK_BINARY : 0));
  return new_sock;
}

/* attempts to read from all the sockets in socklist */
/* fills s with up to 1023 bytes if available, and returns 0 */
/* on EOF, returns -1, with socket in len */
/* on socket error, returns -2 */
/* if nothing is ready, returns -3 */
int sockread(s,len,socket_list)
char *s; int *len;struct sock_list **socket_list;
{
  fd_set fd; int fds,x; 
  struct timeval t; 
  int grab=1023;
  struct sock_list *slist;
  *socket_list=NULL;
  fds=getdtablesize();
#ifdef FD_SETSIZE
  if (fds>FD_SETSIZE) fds=FD_SETSIZE;    /* fixes YET ANOTHER freebsd
bug!!! */
#endif
  /* timeout: 1 sec */
  t.tv_sec=1; t.tv_usec=0; FD_ZERO(&fd);
  slist=socklist;
  while(slist!=NULL) {
      if (!(slist->flags & SOCK_UNUSED)) {
        FD_SET(slist->sock,&fd);
      slist=slist->next;
    }
  }
#ifdef HPUX
  x=select(fds,(int *)&fd,(int *)NULL,(int *)NULL,&t);
#else
  x=select(fds,&fd,NULL,NULL,&t);
#endif
  if (x>0) {
    /* something happened */
    slist=socklist;
    while(slist!=NULL) {
      if ((!(slist->flags & SOCK_UNUSED)) &&
          ((FD_ISSET(slist->sock,&fd)) )) {
        if (slist->flags & (SOCK_LISTEN|SOCK_CONNECT)) {
          /* listening socket -- don't read, just return activity */
          /* same for connection attempt */
          if (!(slist->flags & SOCK_STRONGCONN)) {
            s[0]=0; *len=0; 
            *socket_list=slist;
            return 0;
          }
          /* (for strong connections, require a read to succeed first) */
          
        }
        x=read(slist->sock,s,grab);
        if (x<=0) {   /* eof */
          *len=slist->sock;
          slist->flags &= ~SOCK_CONNECT;
          return -1;
        }
        s[x]=0; *len=x;
        *socket_list=slist;
        return 0;
      }
      slist=slist->next;
    }
  
  }
  else if (x==-1) return -2;   /* socket error */
  else {
    s[0]=0; *len = 0;
  }
  return -3;
} 


/* sockgets: buffer and read from sockets

   attempts to read from all registered sockets for up to one second.  if
     after one second, no complete data has been received from any of the
     sockets, 's' will be empty, 'len' will be 0, and sockgets will return
     -3.
   if there is returnable data received from a socket, the data will be
     in 's' (null-terminated if non-binary), the length will be returned
     in len, and the socket number will be returned.
   normal sockets have their input buffered, and each call to sockgets
     will return one line terminated with a '\n'.  binary sockets are not
     buffered and return whatever coems in as soon as it arrives.
   listening sockets will return an empty string when a connection comes
in.
     connecting sockets will return an empty string on a successful
connect,
     or EOF on a failed connect.
   if an EOF is detected from any of the sockets, that socket number will
be
     put in len, and -1 will be returned.

   * the maximum length of the string returned is 1024 (including null)
*/

int sockgets(s,len)   
char *s; int *len;
{
  char xx[1026],*p,*px; int ret;
  struct sock_list *slist;

  /* check for stored-up data waiting to be processed */
  slist=socklist;
  while(slist!=NULL) {
    if (!(slist->flags & SOCK_UNUSED) && (slist->inbuf != NULL)) {
      /* look for \r too cos windows can't follow RFCs */
      p=strchr(slist->inbuf,'\xff');
      if (p==NULL) p=strchr(slist->inbuf,'\n');
      if (p!=NULL) {
        *p=0;
        if (strlen(slist->inbuf) > 1022) slist->inbuf[1022]=0;
        strcpy(s,slist->inbuf);
        px=(char *)nmalloc(strlen(p+1)+1); strcpy(px,p+1);
        nfree(slist->inbuf);
        if (px[0]) slist->inbuf=px;
        else { nfree(px); slist->inbuf=NULL; }
        /* strip CR if this was CR/LF combo */
        /* strip FF */
        if (s[strlen(s)]=='\xff') s[strlen(s)]=0;
        if (s[strlen(s)]=='\n') s[strlen(s)]=0;
        *len = strlen(s);       /* <-- oh that looks so cute robey! :) */
        return slist->sock;  
      }
    }
    slist=slist->next;
  }
  /* no pent-up data of any worth -- down to business */
  *len=0;
  slist=NULL;
  ret=sockread(xx,len,&slist);
  if (ret<0) { s[0]=0; return ret; }
  /* binary and listening sockets don't get buffered */
  if (slist->flags & SOCK_CONNECT) {
    if (slist->flags & SOCK_STRONGCONN) {
      slist->flags &= ~SOCK_STRONGCONN;
      /* buffer any data that came in, for future read */
/*      slist->inbuf=(char *)nmalloc(strlen(xx)+1);
      strcpy(slist->inbuf,xx);*/
    }
    slist->flags &= ~SOCK_CONNECT;
    s[0]=0; return slist->sock;
  }
  if (slist->flags & SOCK_BINARY) {
    my_memcpy(s,xx,*len);
    return slist->sock;      
  }
  if (slist->flags & SOCK_LISTEN) return slist->sock;
  /* might be necessary to prepend stored-up data! */
  if (slist->inbuf != NULL) {
    p=slist->inbuf;
    slist->inbuf=(char *)nmalloc(strlen(p)+strlen(xx)+1);
    strcpy(slist->inbuf,p); strcat(slist->inbuf,xx);
    nfree(p);
    if (strlen(slist->inbuf) < 1024) {
      strcpy(xx,slist->inbuf);
      nfree(slist->inbuf); slist->inbuf=NULL;
    }
    else {
      p=slist->inbuf;
      slist->inbuf=(char *)nmalloc(strlen(p)-1021);
      strcpy(slist->inbuf,p+1022); *(p+1022)=0; strcpy(xx,p);
nfree(p);  
      /* (leave the rest to be post-pended later) */
    }
  }
  /* look for EOL marker; if it's there, i have something to show */
  p=strchr(xx,'\xff');
/*  if (p==NULL) p=strchr(xx,'\r');*/
  if (p==NULL) p=strchr(xx,'\n');
  if (p!=NULL) {
    *p=0; strcpy(s,xx); strcpy(xx,p+1);
/*    if (s[strlen(s)-1]=='\r') s[strlen(s)-1]=0;*/
    if (s[strlen(s)]=='\xff') s[strlen(s)]=0;
    if (s[strlen(s)]=='\n') s[strlen(s)]=0;
  }
  else {
    s[0]=0;
    if (strlen(xx)>=1022) {
      /* string is too long, so just insert fake \n */
      strcpy(s,xx); xx[0]=0;
    }
  }

  *len = strlen(s);
  /* anything left that needs to be saved? */
  if (!xx[0]) { if (s[0]) return slist->sock; else return -3; }

  /* prepend old data back */
  if (slist->inbuf != NULL) {
    p=slist->inbuf;
    slist->inbuf=(char *)nmalloc(strlen(p)+strlen(xx)+1);
    strcpy(slist->inbuf,xx); strcat(slist->inbuf,p);
    nfree(p);
  }
  else {
    slist->inbuf=(char *)nmalloc(strlen(xx)+1);
    strcpy(slist->inbuf,xx);
  }
  if (s[0]) return slist->sock; else return -3;
}


/* dump something to a socket */
void tputs(z,s)
int z; char *s;
{
  int x; char *p;
  struct sock_list *slist;
  if (z<0) return;   /* um... HELLO?!  sanity check please! */

  slist=socklist;
  while(slist!=NULL) {
    if (!(slist->flags & SOCK_UNUSED) && (slist->sock==z)) {
      if (slist->outbuf != NULL) {
        /* already queueing: just add it */
        p=(char *)nmalloc(strlen(slist->outbuf)+strlen(s)+1);
        strcpy(p,slist->outbuf); strcat(p,s);
        nfree(slist->outbuf); slist->outbuf=p;
        return;
      }

      x=write(z,s,strlen(s));
      if (x==(-1)) x=0;
      if (x < strlen(s)) {
        /* socket is full, queue it */
        slist->outbuf=(char *)nmalloc(strlen(s)-x+1);
        strcpy(slist->outbuf,&s[x]);
      }
      return;
    }
    slist=slist->next;
  }
}

/* tputs might queue data for sockets, let's dump as much of it as */
/* possible */
void dequeue_sockets()
{
  char *p;
  struct sock_list *slist;
  
  slist=socklist;
  while(slist!=NULL) {
    if (!(slist->flags & SOCK_UNUSED) && (slist->outbuf != NULL)) {
      /* trick tputs into doing the work */
      p=slist->outbuf;
      slist->outbuf=NULL;
      tputs(slist->sock,p); nfree(p);
    }
    slist=slist->next;
  }
}

/* like fprintf, but instead of preceding the format string with a FILE
   pointer, precede with a socket number */
/* please stop using this one except for server output.  dcc output
   should now use dprintf(idx,"format",[params]);   */
// tprintf(n->sock,"pline 0 %c/set %cBLOCK %cll lz sq rl rz hc ln   %cSet Block Configuration\xff", BLUE,RED,BLUE,NAVY);
void tprintf(int sock, char *format, ...)
{
  va_list va; static char SBUF2[1050];
  
  va_start(va,format);
  
  vsprintf(SBUF2,format,va);
  if (strlen(SBUF2)>1022)
 {  
    SBUF2[1022]=0;   /* server can only take so much */
 }
  tputs(sock,SBUF2);
  if (strlen(SBUF2) > 3) lvprintf(10,"Tprintf(%d): %s\n", sock, SBUF2);
  va_end(va);
} 

IP getip(char *s)
{
  struct hostent *hp; IP ip; struct in_addr *in;
  
  hp=gethostbyname(s);
    
  if (hp==NULL)
    ip=0;
  else
    {
      in=(struct in_addr *)(hp->h_addr_list[0]);
      ip=(IP)(in->s_addr);
    }
  return ip;
}

