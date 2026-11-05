/*
 * io.h
 *
 * Disk I/O module. As far as output, routines exist to create a new file (which
 * will truncate an existing file) or appending lines to a file. No open file
 * handle is maintained for output.
 *
 * For input, a stack of open files is maintained. Each time a new file is
 * opened, the currently open file is preserved. The new file is opened and the
 * input stream is taken from the file. Once the currently open file is closed,
 * the previously opened file again becomes the current one and input is taken
 * from it. When a file is preserved, it's file pointer and current buffer are
 * maintained and resumed when it again becomes active.
 */

#ifndef IO_H
#define IO_H

#include <stddef.h>


/*
 * GLOBAL CONSTANTS
 */

#define IO_EOF 0


/*
 * GLOBAL FUNCTIONS
 */


/*
 * ioCloseInput
 *
 * Closes the current input file, and restores the previous input file, if any,
 * as the current one.
 *
 * Returns
 *
 *  0 = success
 *  1 = error
 */
extern int ioCloseInput(void);


/*
 * ioInit
 *
 * Initialize the module.
 *
 * Returns
 *
 *  0 = success
 *  1 = error
 */
extern int ioInit(void);


/*
 * ioNext
 *
 * Return the current character from the input stream and advances the file
 * pointer to the next character.
 *
 * Returns
 *
 *  0 = EOF
 *  otherwise, the current character in the input stream.
 */
extern char ioNext(void);


/*
 * ioOpenInput
 *
 * Set the input source to a filename (f). If f is NULL, stdin is used.
 *
 * Returns
 *
 *  0 = success
 *  1 = error
 */
extern int ioOpenInput(char *f);


/*
 * ioPeek
 *
 * Return the current character from the current input stream, without reading
 * the next one (i.e. the file pointer is not advanced).
 *
 * Returns
 *
 *  0 = EOF
 *  otherwise, the current character in the input stream.
 */
extern char ioPeek(void);


/*
 * ioColumn
 *
 * The 0-based column of the current character (the one ioPeek returns) in
 * its input line, or -1 if nothing has been read on this line.
 */
extern long ioColumn(void);


/*
 * ioLineText
 *
 * The current input line as read so far, up to and including the current
 * character (not NUL-terminated; the length is stored in *len). After the
 * rest of the line has been read, this is the whole line. Used to show where
 * a syntax error is.
 */
extern const char *ioLineText(size_t *len);


/*
 * ioReadLine
 *
 * Read the next line of standard input (for INPUT), without the newline.
 * Standard input is read through the same buffer the interpreter uses, so the
 * two never lose each other's data. The scanner's current character is not
 * affected.
 *
 * Returns
 *
 *  a newly allocated string, or NULL at end of input or on error.
 */
extern char *ioReadLine(void);


#endif /* IO_H */
