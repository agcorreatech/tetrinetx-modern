/*
  net.h
  
  Just about all low level network functions.
  
  NOTE: Apart from some changes, most of net.c and net.h is from the
  excellent eggdrop program "eggdrop". I found that what was in net.c
  absolutely perfectly matched what I needed, and the way it went by
  doing that unique :) Beautifully written. 
  Author is Robey Pointer, robey@netcom.com.
  Eggdrop can be most likely found at http://www.valuserve.com/~robey/eggdrop/

  NOTE: I feel a little bit better. I found this in the README file ;), and 
  thus the same applies into here then:
  
    The files "match.c", "net.c", and "blowfish.c" are exempt from the above
    restrictions.  "match.c" is original code by Chris Fuller (email:
    crf@cfox.bchs.uh.edu) and has been placed by him into the public domain.
    "net.c" is by me and I also choose to place it in the public domain.
    "blowfish.c" is by various sources and is in the public domain.  All 3
    files contain useful functions that could easily be ported to other
    applications -- the other parts of the bot generally don't.
    
  NOTE: net.c has been heavily modified by me to provide more dynamic
  allocation of sockets, instead of static. Should save memory, and allow
  greater than expected number of sockets to be allocated.
*/

#include <unistd.h>
  

/* socket flags: */
#define SOCK_UNUSED     0x01    /* empty socket */
#define SOCK_BINARY     0x02    /* do not buffer input */
#define SOCK_LISTEN     0x04    /* listening port */
#define SOCK_CONNECT    0x08    /* connection attempt */
#define SOCK_NONSOCK    0x10    /* used for file i/o on debug */
#define SOCK_STRONGCONN 0x20    /* don't report success until sure */   

/* this is used by the net module to keep track of sockets and what's
   queued on them */
struct sock_list {
  int sock;
  char flags;
  char *inbuf;
  char *outbuf;
  struct sock_list *next;
};
 

/*#define MAXSOCKS MAXNET*2*/
struct sock_list *socklist;  /* enough to be safe */   

/* i read somewhere that memcpy() is broken on some machines */
/* it's easy to replace, so i'm not gonna take any chances, because it's
*/
/* pretty important that it work correctly here */
void my_memcpy(char *dest,char *src,int len);


/* bzero() is bsd-only, so here's one for non-bsd systems */
void my_bzero(char *dest,int len);


/* initialize the socklist */
void init_net(void);



int expmem_net(void);


/* puts full hostname in s */
void getmyhostname(char *s);


/* get my ip number */
IP getmyip(void);


void neterror(char *s);


/* request a normal socket for i/o */
void setsock(int sock,int options);


int getsock(int options);


/* done with a socket */
void killsock(int sock);


/* returns a socket number for a listening socket that will accept any */
/* connection -- port # is returned in port */
int open_listen(int *port);


/* given network-style IP address, return hostname */
/* hostname will be "##.##.##.##" format if there was an error */
char *hostnamefromip(unsigned long ip);


/* short routine to answer a connect received on a socket made previously
*/
/* by open_listen ... returns hostname of the caller & the new socket */
/* does NOT dispose of old "public" socket! */
int answer(int sock,char *caller,unsigned long *ip,int binary);


/* attempts to read from all the sockets in socklist */
/* fills s with up to 511 bytes if available, and returns the array index
*/
/* on EOF, returns -1, with socket in len */
/* on socket error, returns -2 */
/* if nothing is ready, returns -3 */
int sockread(char *s,int *len,struct sock_list **socket_list);



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

   * the maximum length of the string returned is 512 (including null)
*/

int sockgets(char *s,int *len);


/* dump something to a socket */
void tputs(int z,char *s);


/* tputs might queue data for sockets, let's dump as much of it as */
/* possible */
void dequeue_sockets(void);


IP getip(char *s);


