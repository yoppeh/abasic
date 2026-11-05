/**
 * @file util.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Error reporting and string helpers.
 * @details Errors are printed to stderr as "ERROR: message", or with an Atari
 * error number and program line, "ERROR 12 at line 20: message". Only the
 * first error of each unit of work (a line being parsed, an instruction being
 * run) is printed, so a failure isn't followed by vaguer messages from its
 * callers.
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#ifndef UTIL_H
#define UTIL_H

// Lets GCC and Clang check printf-style format strings against their
// arguments at every call site.
#if defined(__GNUC__) || defined(__clang__)
#define UTIL_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#define UTIL_NORETURN __attribute__((noreturn))
#else
#define UTIL_PRINTF(fmt, args)
#define UTIL_NORETURN
#endif

/**
 * @brief The longest string literal, name or line the scanner accepts.
 * @details Also the size, less one, of the buffer eval uses for numbers.
 */
extern int maxStringLen;

/**
 * @brief Prints "ERROR: " and a message to stderr, then exits with status 1.
 * @param s A printf format, without a trailing newline.
 */
extern void aborts(const char *s, ...) UTIL_PRINTF(1, 2) UTIL_NORETURN;

/**
 * @brief Reports an error that has no Atari error number (a syntax error).
 * @details Prints "ERROR: " and the message to stderr, with the line set by
 * utilErrorLine(). Only the first error since the last utilErrorReset() is
 * printed, and none is printed while quiet (see utilErrorQuiet()), but the
 * message is always kept for utilLastError().
 * @param s A printf format, without a trailing newline.
 */
extern void utilError(const char *s, ...) UTIL_PRINTF(1, 2);

/**
 * @brief Reports a runtime error with an Atari error number.
 * @details Like utilError(), with the number (Appendix B of the Atari BASIC
 * Reference Manual) shown with the message and kept for utilLastErrorCode(),
 * which TRAP handlers read through PEEK(195).
 * @param code The Atari error number.
 * @param s A printf format, without a trailing newline.
 */
extern void utilErrorN(int code, const char *s, ...) UTIL_PRINTF(2, 3);

/**
 * @brief Returns the Atari error number of the most recent error.
 * @return The number, or 0 for a syntax error.
 */
extern int utilLastErrorCode(void);

/**
 * @brief Starts a new unit of work: a line being parsed or an instruction being run.
 * @details The next error is reported again, with no line number and not
 * quiet.
 */
extern void utilErrorReset(void);

/**
 * @brief Sets the program line the current work belongs to.
 * @details Errors then say "at line N" until the next utilErrorReset().
 * @param line The line number, or -1 for none.
 */
extern void utilErrorLine(long line);

/**
 * @brief Sets whether errors are printed.
 * @details While quiet, errors are only recorded, for utilLastError() and
 * utilLastErrorCode(); a TRAP handles them instead.
 * @param quiet Nonzero to record errors without printing them.
 */
extern void utilErrorQuiet(int quiet);

/**
 * @brief Tells whether an error has been reported since the last utilErrorReset().
 * @return Nonzero if one has.
 */
extern int utilErrorReported(void);

/**
 * @brief Returns the text of the most recent error, without "ERROR: ".
 * @return The message; it stays valid until the next error.
 */
extern const char *utilLastError(void);

/**
 * @brief Prints the most recent error.
 * @details For an error recorded while quiet that turns out not to be
 * trapped after all.
 */
extern void utilErrorShow(void);

/**
 * @brief Prints the "ready" prompt.
 */
extern void utilReady(void);

/**
 * @brief Joins strings with a delimiter.
 * @param a The strings, ending with a NULL pointer.
 * @param s The delimiter put between them. NULL joins them with nothing.
 * @return A newly allocated string, or NULL if a is NULL or memory runs out.
 */
extern char *strmergez(const char **a, const char *s);

/**
 * @brief Splits a string at every occurrence of a delimiter.
 * @details The array and the strings are one allocation, so a single free()
 * releases them. An empty delimiter splits the string into single characters.
 * @param s The string to split.
 * @param d The delimiter. NULL is the same as "".
 * @return The pieces, ending with a NULL pointer, or NULL if s is NULL or
 * memory runs out.
 */
extern char **strsplitz(const char *s, const char *d);

#endif // UTIL_H
