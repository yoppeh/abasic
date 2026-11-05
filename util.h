/*
 * util.h
 */

#ifndef UTIL_H
#define UTIL_H


/*
 * Lets GCC and Clang check printf-style format strings against their
 * arguments at every call site.
 */
#if defined(__GNUC__) || defined(__clang__)
#define UTIL_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#define UTIL_NORETURN __attribute__((noreturn))
#else
#define UTIL_PRINTF(fmt, args)
#define UTIL_NORETURN
#endif


extern int maxStringLen;


/* print "ERROR: " and the message to stderr, then exit(1) */
extern void aborts(const char *s, ...) UTIL_PRINTF(1, 2) UTIL_NORETURN;

/*
 * Print "ERROR: " and the message (no trailing newline needed) to stderr.
 * Only the first error since the last utilErrorReset() is printed.
 */
extern void utilError(const char *s, ...) UTIL_PRINTF(1, 2);

/*
 * Like utilError, for a runtime error with an Atari error number (Appendix B
 * of the Atari BASIC Reference Manual), which is shown with the message and
 * available to TRAP handlers through PEEK(195).
 */
extern void utilErrorN(int code, const char *s, ...) UTIL_PRINTF(2, 3);

/* the Atari error number of the most recent error (0 for a syntax error) */
extern int utilLastErrorCode(void);

/*
 * Start a new unit of work (a line being parsed, an instruction being run):
 * the next error is reported again, with no line number and not quiet.
 */
extern void utilErrorReset(void);

/* the program line the current work belongs to: errors say "at line N" */
extern void utilErrorLine(long line);

/* when quiet, errors are only recorded (for utilLastError), not printed */
extern void utilErrorQuiet(int quiet);

/* has an error been reported since the last utilErrorReset()? */
extern int utilErrorReported(void);

/* the text of the most recent error, without "ERROR: " */
extern const char *utilLastError(void);

/* print the most recent error (e.g. one recorded quietly that wasn't trapped after all) */
extern void utilErrorShow(void);

extern void utilReady(void);

extern char *strmergez(const char **a, const char *s);

extern char **strsplitz(const char *s, const char *d);


#endif /* UTIL_H */
