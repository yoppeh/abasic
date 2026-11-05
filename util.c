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

/* set once an error has been reported for the current line or instruction */
static int errorReported = 0;
/* program line the current work belongs to, or -1 */
static long errorLine = -1;
/* record errors without printing them (a TRAP will handle them) */
static int errorQuiet = 0;
/* text and Atari error number (0 for a syntax error) of the most recent error */
static char lastError[256] = "";
static int lastErrorCode = 0;

static void errorRecord(int code, const char *s, va_list vl) UTIL_PRINTF(2, 0);


void aborts(const char *s, ...) {
    va_list vl;
    /* send pending program output first, so the two streams stay in order */
    fflush(stdout);
    va_start(vl, s);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, s, vl);
    fprintf(stderr, "\n");
    va_end(vl);
    exit(1);
}


void utilError(const char *s, ...) {
    va_list vl;
    va_start(vl, s);
    errorRecord(0, s, vl);
    va_end(vl);
}


void utilErrorN(int code, const char *s, ...) {
    va_list vl;
    va_start(vl, s);
    errorRecord(code, s, vl);
    va_end(vl);
}


static void errorRecord(int code, const char *s, va_list vl) {
    /*
     * Only the first error is reported. It comes from the code that found the
     * problem; the callers that then fail because of it would only add vaguer
     * messages ("expecting statement", "couldn't evaluate expression").
     */
    if (errorReported) {
        return;
    }
    errorReported = 1;
    lastErrorCode = code;
    vsnprintf(lastError, sizeof(lastError), s, vl);
    if (errorQuiet) {
        return;
    }
    utilErrorShow();
}


void utilErrorShow(void) {
    char number[16] = "";
    /* send pending program output first, so the two streams stay in order */
    fflush(stdout);
    /* the Atari error number (Appendix B of the manual); syntax errors have none */
    if (lastErrorCode > 0) {
        snprintf(number, sizeof(number), " %d", lastErrorCode);
    }
    if (errorLine >= 0) {
        fprintf(stderr, "ERROR%s at line %ld: %s\n", number, errorLine, lastError);
    } else {
        fprintf(stderr, "ERROR%s: %s\n", number, lastError);
    }
}


int utilLastErrorCode(void) {
    return(lastErrorCode);
}


void utilErrorLine(long line) {
    errorLine = line;
}


void utilErrorQuiet(int quiet) {
    errorQuiet = quiet;
}


int utilErrorReported(void) {
    return(errorReported);
}


const char *utilLastError(void) {
    return(lastError);
}


void utilErrorReset(void) {
    errorReported = 0;
    errorLine = -1;
    errorQuiet = 0;
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
    /* n delimiters give n + 1 substrings (at most len), plus a NULL terminator */
    unsigned long max_subs = len + 1;
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
