/* Simple query routine client :) by Drslum */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#define MTU 1536
#define SERVER_PORT 31456

char *conn(char *, int, char *);
int xfgets(int, char *, int);

int main(int argc, char *argv[])
{
   char cmds[512];
   char input[512];
   printf("-=========================-\n");
   printf("  Query - TetriNET Client\n");
   printf("-=========================-\n");
   
   if (argc != 3) {
      fprintf(stderr, " Usage: %s server_ip server_password\n", argv[0]);
      exit(0);
   }
   sprintf(cmds, "securequery %s\xff", argv[2]);
   while(1) {
   	printf("%s\n",conn(argv[1],SERVER_PORT, cmds));
  	sprintf(cmds, "securequery %s\xff", argv[2]);
	printf("Type \"remove playernumber\" to remove player.\nType \"quit\" to quit or enter to query\n"); 
	if (fgets(input, sizeof(input)-1, stdin) != (char *)NULL) {
		if (!strncasecmp(input, "remove", 6) && strlen(input) >= 9)
		   if (atoi(input+7) >= 1 && atoi(input+7) <= 6)
		   	sprintf(cmds, "killplayer %s %d\xff",argv[2], atoi(input+7));
		if (!strncasecmp(input, "quit", 4))
		   exit(1);
	} else exit(0);
   }
}


char *conn(char *hst, int prt, char *cmds) 
{
   struct sockaddr_in sin;
   struct hostent *he;
   int s;
   char buf[MTU];

/* Now simply checking for hostname...*/
   if ((he = gethostbyname(hst)) == NULL)   {
      printf(" Error: Invalid hostname!\n");
      exit(0);
   }

/* Create a TCP socket for this client to xserve connection */
   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)   {
      close(s);
      printf(" Error: Connection Timeout.\n");
      exit(0);
   }

/* Define connection parameter (e.g. address, port) in sin variable */
   sin.sin_family = AF_INET;
   sin.sin_port   = htons(prt);
   sin.sin_addr   = *((struct in_addr *)he->h_addr);
   bzero(&(sin.sin_zero), 8);

/* Connect to the server */ 
   if (connect(s, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0) {
      close(s);
      printf(" Error: Server is shutted down.\n");
      exit(0);
   }
   printf("------------------------------------------------------------------------\n");
   strcpy(buf, "error\n");
   if (write(s, cmds, strlen(cmds)) > 0)
      while (xfgets(s,buf,MTU) > 0)
	printf("%s", buf);
   close(s);
   if (!strcmp(buf, "error\n")) {
      printf(" No result from server (could be wrong password)\n");
      exit(0);
   }
   printf("------------------------------------------------------------------------\n");
   return(" ");
}

int xfgets(fd, buf, sz)

int fd, sz;
char *buf;

{
  int rd = 0;
  int res= 0;
  while (rd < sz-1 && (res = read(fd, buf + rd, 1)) == 1) {
    if (buf[rd++] == '\n') {
      buf[rd] = 0;
      return(rd);
    }
  }
  return((res < 0) ? -1 : rd);
}
