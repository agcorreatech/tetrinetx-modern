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

/* Bounded string copy, always null-terminated, truncating silently if src
   doesn't fit. Computes the length to copy at runtime (via strlen()+a
   comparison) rather than using strncpy()/snprintf() directly, which
   avoids GCC's -Wstringop-truncation/-Wformat-truncation warnings that
   fire on those when it can't statically prove the source is short
   enough -- the truncation here is intentional and safe (destsize is
   always the real destination buffer size), there's just no compiler
   pragma-free way to tell GCC's heuristic that using strncpy/snprintf
   directly. */
void safe_strcpy(char *dest, size_t destsize, const char *src);
