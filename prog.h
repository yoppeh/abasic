/**
 * @file prog.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief The program: its lines, their statements, and running them.
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#ifndef PROG_H
#define PROG_H

#include <stdarg.h>

#include "scan.h"

// Atari BASIC line numbers are 0 to 32767.
#define MAX_LINE_NUMBER 32767L

/**
 * @brief Runs a statement.
 */
typedef void (instructionExecuteFunc)(void *);

/**
 * @brief Returns a statement's source text, for LIST and SAVE, newly allocated.
 */
typedef char * (instructionFormatFunc)(void *);

/**
 * @brief Frees a statement and everything it holds.
 */
typedef void (instructionFreeFunc)(void *);

/**
 * @brief One statement of a line.
 * @details Each kind of statement has its own struct that starts with an
 * instructionType, followed by what the statement needs (its expressions).
 * The callbacks are called with a pointer to that struct.
 */
typedef struct instructionType {
    struct instructionType *next;
    keywords keyword;
    instructionExecuteFunc *executeFunc;
    instructionFormatFunc *formatFunc;
    instructionFreeFunc *freeFunc;
} instructionType;

/**
 * @brief A line of the program, or a line typed to be run at once.
 * @details lineNum is -1 for a line run at once. currentInstruction is the
 * statement being run, which can be in a list belonging to an IF.
 */
typedef struct progLineType {
    struct progLineType *next;
    instructionType *firstInstruction;
    instructionType *lastInstruction;
    instructionType *currentInstruction;
    long int lineNum;
} progLineType;

/**
 * @brief Adds a statement to the end of a line.
 * @details The arguments after k are the statement's parsed parts, which
 * depend on k (for example an assignment expression for LET, or a list of
 * expressions and its length for PRINT). The statement takes them over.
 * @param p The line.
 * @param k The kind of statement.
 * @return 0, or 1 after reporting an error.
 */
extern int progAppendInstruction(progLineType *p, keywords k, ...);

/**
 * @brief Frees an expression tree.
 * @param s The root. NULL does nothing.
 */
extern void progDeleteExpression(symbolType *s);

/**
 * @brief Frees a list of statements.
 * @param p The first statement. NULL does nothing.
 */
extern void progDeleteInstructions(instructionType *p);

/**
 * @brief Removes a line from the program and frees it.
 * @details The GOSUB/FOR stack is cleared, since it may refer to the line.
 * @param p The line.
 */
extern void progDeleteLine(progLineType *p);

/**
 * @brief Runs a line, and wherever it leads: a GOTO or RUN goes on into the program.
 * @param p The line; one typed to be run at once has lineNum -1.
 */
extern void progExecuteLine(progLineType *p);

/**
 * @brief Initializes the module.
 * @details Seeds RND and arranges for the program to be freed at exit.
 * @return 0.
 */
extern int progInit(void);

/**
 * @brief Adds an empty line to the program, in line number order.
 * @details A line with the same number is replaced.
 * @param l The line number.
 * @return The new line, or NULL if memory runs out.
 */
extern progLineType *progInsertLine(long int l);

/**
 * @brief Deletes a line from the program, if there is one.
 * @param l The line number.
 */
extern void progRemoveLine(long int l);

#endif // PROG_H
