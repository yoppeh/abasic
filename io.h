/**
 * @file io.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Input for the interpreter: the terminal and LOADed files.
 * @details A stack of open inputs is kept. Opening a file puts it on top, and
 * input is taken from it; when it ends, it is closed and input continues
 * from the one below, exactly where it left off. Standard input is at the
 * bottom.
 *
 * Input is read one character at a time: ioNext() moves to the next
 * character and ioPeek() returns the current one. The current line is kept
 * so a syntax error can be shown in context.
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#ifndef IO_H
#define IO_H

#include <stddef.h>

// What ioNext() and ioPeek() return at the end of all input.
#define IO_EOF 0

/**
 * @brief Closes the current input and makes the one below it current.
 * @return 0.
 */
extern int ioCloseInput(void);

/**
 * @brief Initializes the module.
 * @details Arranges for every input to be closed at exit.
 * @return 0.
 */
extern int ioInit(void);

/**
 * @brief Moves to the next character of input.
 * @details At the end of a file, the file is closed and the current
 * character of the input below it is returned.
 * @return The new current character, or IO_EOF at the end of all input.
 */
extern char ioNext(void);

/**
 * @brief Makes a file, or standard input, the current input.
 * @details Nothing is read until the next ioNext(). Until then the current
 * character is a newline, so returning to an input that was never read
 * looks like the start of a line rather than the end of input.
 * @param f The file to read, or NULL for standard input.
 * @return 0, or 1 if the file can't be opened or memory runs out.
 */
extern int ioOpenInput(char *f);

/**
 * @brief Returns the current character, without moving on.
 * @return The current character, or IO_EOF at the end of all input.
 */
extern char ioPeek(void);

/**
 * @brief Returns the column of the current character in its line.
 * @return The 0-based column, or -1 if nothing has been read on this line.
 */
extern long ioColumn(void);

/**
 * @brief Returns the current input line as read so far.
 * @details The line runs up to and including the current character; once
 * the rest of the line has been read, it is the whole line. Used to show
 * where a syntax error is.
 * @param len Set to the length of the line.
 * @return The line, which isn't NUL-terminated, or NULL if there is no input.
 */
extern const char *ioLineText(size_t *len);

/**
 * @brief Reads the next line of standard input, for INPUT.
 * @details Standard input is read through the same buffer the interpreter
 * uses, so the two never lose each other's data, even while a LOADed file is
 * the current input. The current character isn't affected.
 * @return A newly allocated string without the newline, or NULL at the end
 * of input or if memory runs out.
 */
extern char *ioReadLine(void);

#endif // IO_H
