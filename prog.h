/*
 * prog.h
 */

#ifndef PROG_H
#define PROG_H


#include <stdarg.h>

#include "scan.h"


/*
 * DATA TYPES
 */

typedef void (instructionExecuteFunc)(void *);
typedef char * (instructionFormatFunc)(void *);
typedef void (instructionFreeFunc)(void *);

typedef struct instructionType {
	struct instructionType *next;
	keywords keyword;
	instructionExecuteFunc *executeFunc;
	instructionFormatFunc *formatFunc;
	instructionFreeFunc *freeFunc;
} instructionType;

typedef struct progLineType {
	struct progLineType *next;
	instructionType *firstInstruction;
	instructionType *lastInstruction;
	instructionType *currentInstruction;
	long int lineNum;
} progLineType;


/*
 * GLOBAL FUNCTIONS
 */

extern int progAppendInstruction(progLineType *p, keywords k, ...);

extern void progDeleteExpression(symbolType *s);

extern void progDeleteInstructions(instructionType *p);

extern void progDeleteLine(progLineType *p);

extern void progExecuteLine(progLineType *p);

extern int progInit(void);

extern progLineType *progInsertLine(int l);


#endif /* PROG_H */
