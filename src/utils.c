/*
  utils.c
*/

void fatal(char *s, int recoverable)
  {
    struct net_t *nsock;
    struct channel_t *chan;
    printf("%s\n", s);
    
    nsock=NULL;
    chan=chanlist;
    while (chan!=NULL)
      {
        nsock=chan->net;
        while (nsock!=NULL)
          {
            killsock(nsock->sock);
            nsock=nsock->next;
          }
        chan=chan->next;
      }
    exit(1);
  }

void nfree(void *ptr)
{
  int i=0;
  if (ptr==NULL) {
    lvprintf(4,"Trying to free NULL pointer\n");
    i=i;
    return;
  }
  free(ptr);
}

void *nmalloc(int size)
{
  void *x; int i=0;
  x=(void *)malloc(size);
  if (x==NULL) {
    i=i;
    lvprintf(4,"*** FAILED MALLOC");
    return NULL;
  }
  return x;
} 