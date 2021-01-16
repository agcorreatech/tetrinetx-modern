/*
  utils.h
  
  Generic addons that didn't really fit anywhere. Currently, I think whats
  mostly in this file were additional functions from eggdrop (see net.h),
  to get net.c to work (with less modification) ;).
*/

/* Writes S to stdout, and quits */
void fatal(char *s, int recoverable);

/* More controlled free() */
void nfree(void *ptr);

/* More controlled malloc() */
void *nmalloc(int size);
