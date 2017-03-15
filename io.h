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
 *	0 = success
 *	1 = error
 */
extern int ioCloseInput(void);


/*
 * ioInit
 *
 * Initialize the module.
 *
 * Returns 
 *
 *	0 = success
 *	1 = error
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
 *	0 = EOF
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
 *	0 = success
 *	1 = error
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
 *	0 = EOF
 *	otherwise, the current character in the input stream.
 */
extern char ioPeek(void);


#endif /* IO_H */
