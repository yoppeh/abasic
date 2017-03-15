/*
 * util.c
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"


/*
 * CONSTANTS
 */

#define MAX_STRING_LEN_DEF 128


/*
 * DATA
 */

unsigned long int sBufferIncrementz = 128;
int maxStringLen = MAX_STRING_LEN_DEF;


void aborts(char *s, ...) {
	va_list vl;
	va_start(vl, s);
	printf("ERROR: ");
	vprintf(s, vl);
	printf("\n");
	va_end(vl);
	exit(1);
}


void utilError(char *s, ...) {
	va_list vl;
	va_start(vl, s);
	printf("ERROR: ");
	vprintf(s, vl);
	printf("\n");
	va_end(vl);
}


void utilReady(void) {
	printf("\nready\n");
}


char *strmergez(const char **a, const char *d) {
	if (a == NULL) {
		return(NULL);
	}
	if (d == NULL) {
		d = "";
	}
	unsigned long int maxLen = sBufferIncrementz;
	unsigned long int len = 0UL;
	char *rr = NULL;
	char *r = NULL;
	char *t = NULL;
	if ((r = malloc(maxLen)) == NULL) {
		return(NULL);
	}
	t = r;
	while (*a) {
		const char *s = *a;
		while (*s) {
			*t++ = *s++;
			len++;
			if (len == maxLen) {
				maxLen += sBufferIncrementz;
				if ((rr = realloc(r, maxLen)) == NULL) {
					free(r);
					return(NULL);
				}
				r = rr;
				t = r + len;
			}
		}
		if (a[1] == NULL) {
			break;
		}
		s = d;
		while (*s) {
			*t++ = *s++;
			len++;
			if (len == maxLen) {
				maxLen += sBufferIncrementz;
				if ((rr = realloc(r, maxLen)) == NULL) {
					free(r);
					return(NULL);
				}
				r = rr;
				t = r + len;
			}
		}
		a++;
	}
	*t = 0;
	len++;
	if ((rr = realloc(r, len)) != NULL) {
		r = rr;
	}
	return(r);
}


char **strsplitz(const char *s, const char *d) {
	if (s == NULL) {
		return(NULL);
	}
	if (d == NULL) {
		d = "";
	}
	unsigned long dlen = strlen(d);
	unsigned long len = strlen(s) + 1;
	unsigned long max_subs = len;
	unsigned long sub = 0;
	unsigned long i;
	char *str = (char *)(max_subs * sizeof(char *));
	char *mem = malloc((unsigned long)str + len * 2);
	if (mem == NULL) {
		return(NULL);
	}
	str += (unsigned long)mem;
	char *t = str;
	if (dlen == 0) {
		for (i = 0; s[i]; i++) {
			((char **)mem)[sub++] = t;
			*t++ = s[i];
			*t++ = 0;
		}
	} else {
		i = 0;
		while (1) {
			if (!strncmp(s + i, d, dlen) || s[i] == 0) {
				((char **)mem)[sub++] = t;
				*str = 0;
				if (s[i] == 0) {
					break;
				}
				t = str + 1;
				i += dlen;
			} else {
				*str = s[i++];
			}
			str++;
		}
	}
	((char **)mem)[sub] = NULL;
	return((char **)mem);
}
