/*
 * util.h
 */

#ifndef UTIL_H
#define UTIL_H


extern int maxStringLen;


extern void aborts(char *s, ...);

extern void utilError(char *s, ...);

extern void utilReady(void);

extern char *strmergez(const char **a, const char *s);

extern char **strsplitz(const char *s, const char *d);


#endif /* UTIL_H */
