/*
 * prog.c
 */

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "io.h"
#include "prog.h"
#include "scan.h"
#include "util.h"
#include "var.h"


/*
 * DATA TYPES
 */

typedef struct assignmentType {
    instructionType ins;
    symbolType *assignment;
} assignmentType;

typedef struct byeType {
    instructionType ins;
} byeType;

typedef struct clrType {
    instructionType ins;
} clrType;

typedef struct clsType {
    instructionType ins;
} clsType;

typedef struct contType {
    instructionType ins;
} contType;

typedef struct dataType {
    instructionType ins;
    char **dataList;
} dataType;

typedef struct degType {
    instructionType ins;
} degType;

typedef struct dimType {
    instructionType ins;
    symbolType **dimList;
    unsigned long int numElements;
} dimType;

typedef struct endType {
    instructionType ins;
} endType;

typedef struct forType {
    instructionType ins;
    symbolType *startPoint;
    symbolType *endPoint;
    symbolType *step;
} forType;

typedef struct gosubType {
    instructionType ins;
    symbolType *targetLine;
} gosubType;

typedef struct gotoType {
    instructionType ins;
    symbolType *targetLine;
} gotoType;

typedef struct ifType {
    instructionType ins;
    symbolType *expression;
    void *gotoOrInstructions;
    char isGoto;
} ifType;

typedef struct inputType {
    instructionType ins;
    symbolType **varList;
    unsigned long int numVars;
} inputType;

typedef struct letType {
    instructionType ins;
    symbolType *assignment;
} letType;

typedef struct listType {
    instructionType ins;
    symbolType *startLine;
    symbolType *endLine;
} listType;

typedef struct loadType {
    instructionType ins;
    symbolType *fileName;
} loadType;

typedef struct newType {
    instructionType ins;
} newType;

typedef struct nextType {
    instructionType ins;
    symbolType *iteratorVar;
} nextType;

typedef struct onType {
    instructionType ins;
    symbolType *expression;
    symbolType *instruction;
    symbolType **targetList;
    unsigned long int numTargets;
} onType;

typedef struct popType {
    instructionType ins;
} popType;

typedef struct printType {
    instructionType ins;
    symbolType **expressionList;
    unsigned long int numExpressions;
} printType;

typedef struct radType {
    instructionType ins;
} radType;

typedef struct readType {
    instructionType ins;
    symbolType **varList;
    unsigned long int numVars;
} readType;

typedef struct remType {
    instructionType ins;
    char *remark;
} remType;

typedef struct restoreType {
    instructionType ins;
    symbolType *targetLine;
} restoreType;

typedef struct returnType {
    instructionType ins;
} returnType;

typedef struct runType {
    instructionType ins;
} runType;

typedef struct saveType {
    instructionType ins;
    symbolType *fileName;
} saveType;

typedef struct stopType {
    instructionType ins;
} stopType;

typedef struct trapType {
    instructionType ins;
    symbolType *targetLine;
} trapType;


/*
 * LOCAL DATA
 */

/*
 * Atari BASIC keeps FOR loops and GOSUB return points on one stack: RETURN
 * discards FOR entries above its GOSUB, and POP removes the top entry,
 * whichever kind it is (Reference Manual, POP).
 */
#define MAX_CONTROL 1024
typedef struct {
    int isFor;                      /* a FOR loop, or a GOSUB return point */
    progLineType *line;             /* the line holding the FOR or GOSUB */
    instructionType *instruction;   /* the FOR or GOSUB instruction itself */
} controlType;
static controlType control[MAX_CONTROL];
static long int controlTop = -1;

/* Atari error numbers (Reference Manual, Appendix B) */
#define ERR_MEMORY 2
#define ERR_VALUE 3
#define ERR_STRING_LENGTH 5
#define ERR_OUT_OF_DATA 6
#define ERR_INPUT 8
#define ERR_DIM 9
#define ERR_STACK 10
#define ERR_OVERFLOW 11
#define ERR_LINE_NOT_FOUND 12
#define ERR_NO_FOR 13
#define ERR_RETURN 16
#define ERR_BAD_STRING 18
#define ERR_EOF 136
#define ERR_FILE_NAME 165
#define ERR_FILE_NOT_FOUND 170

/* PRINT's comma moves to the next multiple of this column (PTABW) */
#define PRINT_TAB_WIDTH 10
/* the output column, for PRINT's comma */
static long int printColumn = 0;

static progLineType *prog = NULL;
static progLineType *progCurrent = NULL;
/* the TRAP line number, or -1 when no TRAP is set (the line needn't exist yet) */
static long int progTrap = -1;
/* the line where STOP or END happened; CONT resumes at the line after it */
static progLineType *progStop = NULL;
/* set by an instruction that moved progCurrent itself (GOTO, IF, NEXT, ...) */
static int progJumped = 0;
/* set by progHalt() when the current instruction failed */
static int progError = 0;
/*
 * The last error a TRAP caught (for PEEK(195), ERR$) and the line of that
 * error or of the last STOP (for PEEK(186)/PEEK(187), ERL).
 */
static long int progErrorLine = 0;
static int progErrorCode = 0;
static char progErrorMessage[256] = "";
/* DEG mode: SIN and COS take, and ATN returns, degrees instead of radians */
static int progDegrees = 0;

#define PI 3.14159265358979323846
static char *memErr = "unable to allocate memory";


/*
 * LOCAL FUNCTIONS
 */

static void progExecute(void);
static void progExecuteAssignment(void *vp);
static void progExecuteBYE(void *vp);
static void progExecuteCLR(void *vp);
static void progExecuteCLS(void *vp);
static void progExecuteCONT(void *vp);
static void progExecuteDATA(void *vp);
static void progExecuteDIM(void *vp);
static void progExecuteEND(void *vp);
static void progExecuteFOR(void *vp);
static void progExecuteGOSUB(void *vp);
static void progExecuteGOTO(void *vp);
static void progExecuteIF(void *vp);
static void progExecuteINPUT(void *vp);
static void progExecuteLET(void *vp);
static void progExecuteLIST(void *vp);
static void progExecuteLOAD(void *vp);
static void progExecuteNEW(void *vp);
static void progExecuteNEXT(void *vp);
static void progExecuteON(void *vp);
static void progExecutePOP(void *vp);
static void progExecutePRINT(void *vp);
static void progExecuteREAD(void *vp);
static void progExecuteREM(void *vp);
static void progExecuteRESTORE(void *vp);
static void progExecuteRETURN(void *vp);
static void progExecuteRUN(void *vp);
static void progExecuteSAVE(void *vp);
static void progExecuteSTOP(void *vp);
static void progExecuteTRAP(void *vp);
static void progExit(void);
static char *progFormatAssignment(void *vp);
static char *progFormatDATA(void *vp);
static char *progFormatDefault(void *vp);
static char *progFormatDIM(void *vp);
static char *progFormatFOR(void *vp);
static char *progFormatGOSUB(void *vp);
static char *progFormatGOTO(void *vp);
static char *progFormatIF(void *vp);
static char *progFormatINPUT(void *vp);
static char *progFormatLET(void *vp);
static char *progFormatLIST(void *vp);
static char *progFormatLOAD(void *vp);
static char *progFormatNEXT(void *vp);
static char *progFormatON(void *vp);
static char *progFormatPRINT(void *vp);
static char *progFormatREAD(void *vp);
static char *progFormatREM(void *vp);
static char *progFormatRESTORE(void *vp);
static char *progFormatSAVE(void *vp);
static char *progFormatTRAP(void *vp);
static void progFreeAssignment(void *vp);
static void progFreeDATA(void *vp);
static void progFreeDefault(void *vp);
static void progFreeDIM(void *vp);
static void progFreeFOR(void *vp);
static void progFreeGOSUB(void *vp);
static void progFreeGOTO(void *vp);
static void progFreeIF(void *vp);
static void progFreeINPUT(void *vp);
static void progFreeLET(void *vp);
static void progFreeLIST(void *vp);
static void progFreeLOAD(void *vp);
static void progFreeNEXT(void *vp);
static void progFreeON(void *vp);
static void progFreePRINT(void *vp);
static void progFreeREAD(void *vp);
static void progFreeREM(void *vp);
static void progFreeRESTORE(void *vp);
static void progFreeSAVE(void *vp);
static void progFreeTRAP(void *vp);

static int progList(int fh, long int start, long int end);
static int progNew(void);
static char *evalCode(symbolType *params);
static char *formatLine(instructionType *i);
static char *eval(symbolType *params);
static int evalDouble(symbolType *e, double *out);
static int evalLong(symbolType *e, long *out);
static int evalSubscripts(symbolType *var, long *d1, long *d2);
static int progAssign(symbolType *a);
static void progForgetLine(progLineType *p);
static void progHalt(void);
static void progPurgeImmediateLine(progLineType *p);
static char *strfmt(const char *fmt, ...) UTIL_PRINTF(1, 2);
static void setResult(char *r, const char *fmt, ...) UTIL_PRINTF(2, 3);
static void formatNumber(char *buf, size_t size, double d);
static progLineType *progFindLine(long int lineNum);
static void progJump(progLineType *p, instructionType *i);
static void progJumpAfter(progLineType *p, instructionType *i);
static int progSetVariable(symbolType *var, const char *value);
static char *progGetVariable(symbolType *var);
static double progTrig(keywords f, double x);
static void progExecuteDEG(void *vp);
static void progExecuteRAD(void *vp);


int progAppendInstruction(progLineType *p, keywords keyword, ...) {
    instructionType *i = NULL;
    va_list vl;
    va_start(vl, keyword);
    switch (keyword) {
        case kwAssignment:
            if ((i = malloc(sizeof(assignmentType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteAssignment;
            i->formatFunc = progFormatAssignment;
            i->freeFunc = progFreeAssignment;
            ((assignmentType *)i)->assignment = va_arg(vl, symbolType *);
            break;
        case kwBYE:
            if ((i = malloc(sizeof(byeType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteBYE;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwCLR:
            if ((i = malloc(sizeof(clrType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteCLR;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwCLS:
            if ((i = malloc(sizeof(clsType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteCLS;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwCONT:
            if ((i = malloc(sizeof(contType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteCONT;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwDATA:
            if ((i = malloc(sizeof(dataType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteDATA;
            i->formatFunc = progFormatDATA;
            i->freeFunc = progFreeDATA;
            ((dataType *)i)->dataList = va_arg(vl, char **);
            break;
        case kwDIM:
            if ((i = malloc(sizeof(dimType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteDIM;
            i->formatFunc = progFormatDIM;
            i->freeFunc = progFreeDIM;
            ((dimType *)i)->dimList = va_arg(vl, symbolType **);
            ((dimType *)i)->numElements = va_arg(vl, unsigned long int);
            break;
        case kwEND:
            if ((i = malloc(sizeof(endType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteEND;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwFOR:
            if ((i = malloc(sizeof(forType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteFOR;
            i->formatFunc = progFormatFOR;
            i->freeFunc = progFreeFOR;
            ((forType *)i)->startPoint = va_arg(vl, symbolType *);
            ((forType *)i)->endPoint = va_arg(vl, symbolType *);
            ((forType *)i)->step = va_arg(vl, symbolType *);
            break;
        case kwGOSUB:
            if ((i = malloc(sizeof(gosubType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteGOSUB;
            i->formatFunc = progFormatGOSUB;
            i->freeFunc = progFreeGOSUB;
            ((gosubType *)i)->targetLine = va_arg(vl, symbolType *);
            break;
        case kwGOTO:
            if ((i = malloc(sizeof(gotoType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteGOTO;
            i->formatFunc = progFormatGOTO;
            i->freeFunc = progFreeGOTO;
            ((gotoType *)i)->targetLine = va_arg(vl, symbolType *);
            break;
        case kwIF:
            if ((i = malloc(sizeof(ifType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteIF;
            i->formatFunc = progFormatIF;
            i->freeFunc = progFreeIF;
            ((ifType *)i)->expression = va_arg(vl, symbolType *);
            ((ifType *)i)->gotoOrInstructions = va_arg(vl, void *);
            ((ifType *)i)->isGoto = va_arg(vl, int);
            break;
        case kwINPUT:
            if ((i = malloc(sizeof(inputType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteINPUT;
            i->formatFunc = progFormatINPUT;
            i->freeFunc = progFreeINPUT;
            ((inputType *)i)->varList = va_arg(vl, symbolType **);
            ((inputType *)i)->numVars = va_arg(vl, unsigned long int);
            break;
        case kwLET:
            if ((i = malloc(sizeof(letType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteLET;
            i->formatFunc = progFormatLET;
            i->freeFunc = progFreeLET;
            ((letType *)i)->assignment = va_arg(vl, symbolType *);
            break;
        case kwLIST:
            if ((i = malloc(sizeof(listType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteLIST;
            i->formatFunc = progFormatLIST;
            i->freeFunc = progFreeLIST;
            ((listType *)i)->startLine = va_arg(vl, symbolType *);
            ((listType *)i)->endLine = va_arg(vl, symbolType *);
            break;
        case kwLOAD:
            if ((i = malloc(sizeof(loadType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteLOAD;
            i->formatFunc = progFormatLOAD;
            i->freeFunc = progFreeLOAD;
            ((loadType *)i)->fileName = va_arg(vl, symbolType *);
            break;
        case kwNEW:
            if ((i = malloc(sizeof(newType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteNEW;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwNEXT:
            if ((i = malloc(sizeof(nextType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteNEXT;
            i->formatFunc = progFormatNEXT;
            i->freeFunc = progFreeNEXT;
            ((nextType *)i)->iteratorVar = va_arg(vl, symbolType *);
            break;
        case kwON:
            if ((i = malloc(sizeof(onType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteON;
            i->formatFunc = progFormatON;
            i->freeFunc = progFreeON;
            ((onType *)i)->expression = va_arg(vl, symbolType *);
            ((onType *)i)->instruction = va_arg(vl, symbolType *);
            ((onType *)i)->targetList = va_arg(vl, symbolType **);
            ((onType *)i)->numTargets = va_arg(vl, unsigned long int);
            break;
        case kwPOP:
            if ((i = malloc(sizeof(popType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecutePOP;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwPRINT:
            if ((i = malloc(sizeof(printType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecutePRINT;
            i->formatFunc = progFormatPRINT;
            i->freeFunc = progFreePRINT;
            ((printType *)i)->expressionList = va_arg(vl, symbolType **);
            ((printType *)i)->numExpressions = va_arg(vl, unsigned long int);
            break;
        case kwREAD:
            if ((i = malloc(sizeof(readType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteREAD;
            i->formatFunc = progFormatREAD;
            i->freeFunc = progFreeREAD;
            ((readType *)i)->varList = va_arg(vl, symbolType **);
            ((readType *)i)->numVars = va_arg(vl, unsigned long int);
            break;
        case kwREM:
            if ((i = malloc(sizeof(remType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteREM;
            i->formatFunc = progFormatREM;
            i->freeFunc = progFreeREM;
            ((remType *)i)->remark = va_arg(vl, char *);
            break;
        case kwRESTORE:
            if ((i = malloc(sizeof(restoreType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteRESTORE;
            i->formatFunc = progFormatRESTORE;
            i->freeFunc = progFreeRESTORE;
            ((restoreType *)i)->targetLine = va_arg(vl, symbolType *);
            break;
        case kwRETURN:
            if ((i = malloc(sizeof(returnType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteRETURN;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwRUN:
            if ((i = malloc(sizeof(runType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteRUN;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwSAVE:
            if ((i = malloc(sizeof(saveType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteSAVE;
            i->formatFunc = progFormatSAVE;
            i->freeFunc = progFreeSAVE;
            ((saveType *)i)->fileName = va_arg(vl, symbolType *);
            break;
        case kwSTOP:
            if ((i = malloc(sizeof(stopType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteSTOP;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwTRAP:
            if ((i = malloc(sizeof(trapType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteTRAP;
            i->formatFunc = progFormatTRAP;
            i->freeFunc = progFreeTRAP;
            ((trapType *)i)->targetLine = va_arg(vl, symbolType *);
            break;
        case kwDEG:
            if ((i = malloc(sizeof(degType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteDEG;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        case kwRAD:
            if ((i = malloc(sizeof(radType))) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                break;
            }
            i->executeFunc = progExecuteRAD;
            i->formatFunc = progFormatDefault;
            i->freeFunc = progFreeDefault;
            break;
        default:
            break;
    }
    /*
     * Failures inside the switch break out rather than jump to err, so that
     * va_end always runs. i is NULL after a failed malloc (already reported)
     * or an unsupported keyword.
     */
    va_end(vl);
    if (i == NULL) {
        utilError("unsupported instruction");
        goto err;
    }
    i->next = NULL;
    i->keyword = keyword;
    if (p->firstInstruction == NULL) {
        p->firstInstruction = p->lastInstruction = i;
    } else {
        p->lastInstruction->next = i;
        p->lastInstruction = i;
    }
    return(0);
err:
    return(1);
}


/*
 * Evaluate an expression and convert it to a number. Returns 0 on success.
 */
static int evalDouble(symbolType *e, double *out) {
    char *s = eval(e);
    if (s == NULL) {
        return(1);
    }
    *out = strtod(s, NULL);
    free(s);
    return(0);
}


/*
 * Evaluate an expression used as an integer (line number, subscript, ...).
 * As the BNF says ("rounded to integer"), Atari BASIC rounds, so 2.5 is 3.
 */
static int evalLong(symbolType *e, long *out) {
    double d;
    if (evalDouble(e, &d)) {
        return(1);
    }
    /* converting an out-of-range double to an integer is undefined */
    if (!(d > -2147483648.0 && d < 2147483647.0)) {
        utilErrorN(ERR_VALUE, "value out of range");
        return(1);
    }
    *out = (long)floor(d + 0.5);
    return(0);
}


/*
 * Evaluate the optional subscripts of an identifier node (l and r). Missing
 * subscripts are -1. Returns 0 on success.
 */
static int evalSubscripts(symbolType *var, long *d1, long *d2) {
    /* -1: no such subscript */
    *d1 = -1;
    *d2 = -1;
    if (var->l != NULL && evalLong(var->l, d1)) {
        return(1);
    }
    if (var->r != NULL && evalLong(var->r, d2)) {
        return(1);
    }
    return(0);
}


/*
 * Execute an assignment node (l = identifier, r = expression). On failure the
 * program is halted and 1 is returned.
 */
static int progAssign(symbolType *a) {
    char *value = NULL;
    if ((value = eval(a->r)) == NULL || progSetVariable(a->l, value)) {
        free(value);
        progHalt();
        return(1);
    }
    free(value);
    return(0);
}


/*
 * Called before a program line is freed: drop everything that might still
 * point at it (or at its instructions).
 */
static void progForgetLine(progLineType *p) {
    controlTop = -1;
    if (progStop == p) {
        progStop = NULL;
    }
}


/*
 * Report that the current instruction failed (after printing the error).
 * progExecute then jumps to the TRAP line, if one is set, or stops the program.
 */
static void progHalt(void) {
    progError = 1;
}


/*
 * An immediate-mode line lives on the caller's stack and its instructions are
 * freed after it runs, so remove any FOR or GOSUB entries that still refer to it.
 */
static void progPurgeImmediateLine(progLineType *p) {
    long int i;
    long int j = 0;
    for (i = 0; i <= controlTop; i++) {
        if (control[i].line != p) {
            control[j++] = control[i];
        }
    }
    controlTop = j - 1;
}


/*
 * sprintf into a newly allocated string of exactly the right size.
 */
static char *strfmt(const char *fmt, ...) {
    va_list vl;
    int n;
    char *s;
    va_start(vl, fmt);
    n = vsnprintf(NULL, 0, fmt, vl);
    va_end(vl);
    if (n < 0 || (s = malloc((size_t)n + 1)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        return(NULL);
    }
    va_start(vl, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, vl);
    va_end(vl);
    return(s);
}


/*
 * Format a number the way it is printed and stored: integers without a
 * decimal point, other values with up to 15 significant digits and no
 * trailing zeros.
 */
static void formatNumber(char *buf, size_t size, double d) {
    if (d == 0) {
        d = 0; /* print -0 as 0 */
    }
    snprintf(buf, size, "%.15g", d);
}


static progLineType *progFindLine(long int lineNum) {
    progLineType *p = prog;
    while (p != NULL && p->lineNum != lineNum) {
        p = p->next;
    }
    return(p);
}


/*
 * Continue execution at instruction i of line p (its first instruction if i
 * is NULL). A NULL line ends the program.
 */
static void progJump(progLineType *p, instructionType *i) {
    progCurrent = p;
    if (p != NULL) {
        p->currentInstruction = (i != NULL) ? i : p->firstInstruction;
    }
    progJumped = 1;
}


/*
 * Continue execution with whatever follows instruction i of line p: the next
 * instruction, or the start of the next line.
 */
static void progJumpAfter(progLineType *p, instructionType *i) {
    if (i != NULL && i->next != NULL) {
        progJump(p, i->next);
    } else {
        progJump(p->next, NULL);
    }
}


/*
 * Store a value in a variable. Numeric variables (no '$') always hold a
 * normalized number, so INPUT "007" stores 7.
 */
static int progSetVariable(symbolType *var, const char *value) {
    char num[64];
    char *old = NULL;
    char *s = NULL;
    long d1, d2, max, len, n, newLen;
    int rc = 1;
    if (strchr(var->value, '$') == NULL) {
        if (evalSubscripts(var, &d1, &d2)) {
            return(1);
        }
        formatNumber(num, sizeof(num), strtod(value, NULL));
        return(varSetValue(var->value, num, d1, d2));
    }
    /* A$ = value: the whole string (varSetValue cuts it to the DIM size) */
    if (var->l == NULL) {
        return(varSetValue(var->value, value, -1, -1));
    }
    /*
     * A$(i) = value: replace from character i on; the string ends after the
     * new text. A$(i,j) = value: overwrite characters i to j, keeping the rest.
     */
    if ((max = varStringMax(var->value)) < 0 || evalLong(var->l, &d1)) {
        return(1);
    }
    d2 = max;
    if (var->r != NULL && evalLong(var->r, &d2)) {
        return(1);
    }
    if (d1 < 1 || d1 > max || d2 < d1 || d2 > max) {
        utilErrorN(ERR_STRING_LENGTH, "string index out of range");
        return(1);
    }
    if ((old = varGetValue(var->value, -1, -1)) == NULL) {
        return(1);
    }
    len = (long)strlen(old);
    n = (long)strlen(value);
    if (n > d2 - d1 + 1) {
        n = d2 - d1 + 1;
    }
    newLen = d1 - 1 + n;
    if (var->r != NULL && len > newLen) {
        newLen = len;
    }
    if ((s = malloc((size_t)newLen + 1)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto done;
    }
    /* a gap between the old end and i is filled with spaces */
    memset(s, ' ', (size_t)newLen);
    memcpy(s, old, (size_t)(len < newLen ? len : newLen));
    memcpy(s + d1 - 1, value, (size_t)n);
    s[newLen] = 0;
    rc = varSetValue(var->value, s, -1, -1);
done:
    free(old);
    free(s);
    return(rc);
}


/*
 * The value of a variable reference. For a string, A$(i) is character i to
 * the end and A$(i,j) is characters i to j.
 */
static char *progGetVariable(symbolType *var) {
    char *s;
    char *sub;
    long d1, d2, len;
    if (strchr(var->value, '$') == NULL) {
        if (evalSubscripts(var, &d1, &d2)) {
            return(NULL);
        }
        return(varGetValue(var->value, d1, d2));
    }
    if ((s = varGetValue(var->value, -1, -1)) == NULL || var->l == NULL) {
        return(s);
    }
    len = (long)strlen(s);
    d2 = len;
    if (evalLong(var->l, &d1) || (var->r != NULL && evalLong(var->r, &d2))) {
        free(s);
        return(NULL);
    }
    /* "The starting location cannot be greater than the current length of the string" */
    if (d1 < 1 || d1 > len || d2 > len || d2 < d1) {
        utilErrorN(ERR_STRING_LENGTH, "string index out of range");
        free(s);
        return(NULL);
    }
    sub = strndup(s + d1 - 1, (size_t)(d2 - d1 + 1));
    free(s);
    if (sub == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
    }
    return(sub);
}


/*
 * SIN, COS or ATN, in degrees when DEG is on. Whole multiples of 90 degrees
 * give exact results, so SIN(180) is 0 rather than 1.2e-16.
 */
static double progTrig(keywords f, double x) {
    static const double quarterSin[4] = { 0, 1, 0, -1 };
    long q;
    if (f == kwATN) {
        return(progDegrees ? atan(x) * 180.0 / PI : atan(x));
    }
    if (progDegrees) {
        if (fabs(x) < 1e15 && fmod(x, 90.0) == 0) {
            q = (((long)(x / 90.0)) % 4 + 4) % 4;
            return(f == kwSIN ? quarterSin[q] : quarterSin[(q + 1) % 4]);
        }
        x = x * PI / 180.0;
    }
    return(f == kwSIN ? sin(x) : cos(x));
}


void progDeleteExpression(symbolType *e) {
    if (e == NULL) {
        return;
    }
    if (e->l) {
        progDeleteExpression(e->l);
    }
    if (e->r) {
        progDeleteExpression(e->r);
    }
    free(e);
}


void progDeleteInstructions(instructionType *i) {
    instructionType *n;
    while (i) {
        n = i->next;
        i->freeFunc(i);
        i = n;
    }
}


void progDeleteLine(progLineType *p) {
    progLineType *cp = prog;
    progForgetLine(p);
    progDeleteInstructions(p->firstInstruction);
    if (prog == p) {
        prog = prog->next;
    } else {
        cp = prog;
        while (cp->next && cp->next != p) {
            cp = cp->next;
        }
        if (cp->next == p) {
            cp->next = p->next;
        }
    }
    free(p);
}


static void progExecute(void) {
    instructionType *i;
    progLineType *p;
    keywords keyword;
    long int line;
    while (progCurrent && progCurrent->currentInstruction) {
        i = progCurrent->currentInstruction;
        keyword = i->keyword;
        line = progCurrent->lineNum;
        progJumped = 0;
        progError = 0;
        /* each instruction may report its own error */
        utilErrorReset();
        if (line != -1) {
            /* errors say which line; with a TRAP set, the program handles them instead */
            utilErrorLine(line);
            utilErrorQuiet(progTrap != -1);
        }
        i->executeFunc(i);
        if (progError) {
            progError = 0;
            /* TRAP catches one error in a program line, then has to be set again */
            if (progTrap != -1 && line != -1) {
                long int target = progTrap;
                /* for PEEK(195), PEEK(186)/PEEK(187), ERL and ERR$ */
                progErrorLine = line;
                progErrorCode = utilLastErrorCode();
                snprintf(progErrorMessage, sizeof(progErrorMessage), "%s", utilLastError());
                progTrap = -1;
                if ((p = progFindLine(target)) != NULL) {
                    progJump(p, NULL);
                    continue;
                }
                /* the TRAP line doesn't exist: show the error after all, then that */
                utilErrorShow();
                utilErrorReset();
                utilErrorLine(line);
                utilErrorN(ERR_LINE_NOT_FOUND, "TRAP line %ld not found", target);
            }
            progCurrent = NULL;
            return;
        }
        if (keyword == kwRUN || keyword == kwSTOP || keyword == kwEND) {
            return;
        }
        /* instructions that didn't move execution themselves fall through to the next one */
        if (!progJumped && progCurrent != NULL) {
            progJumpAfter(progCurrent, i);
        }
    }
}


static void progExecuteAssignment(void *vp) {
    progAssign(((assignmentType *)vp)->assignment);
}


static void progExecuteBYE(void *vp) {
    exit(0);
}


static void progExecuteCLR(void *vp) {
    /* variables only: the DATA list and pointer are untouched */
    varClearVariables();
}


static void progExecuteDEG(void *vp) {
    progDegrees = 1;
}


static void progExecuteRAD(void *vp) {
    progDegrees = 0;
}


static void progExecuteCLS(void *vp) {
    system("clear");
}


static void progExecuteCONT(void *vp) {
    progLineType *p = progStop;
    if (p != NULL) {
        progStop = NULL;
        /*
         * "On CONT, execution resumes at the next numbered line": statements
         * after the STOP or END on the same line are not run (Manual, CONT).
         */
        progJump(p->next, NULL);
    }
}


static void progExecuteDATA(void *vp) {
    long int lineNum = -1;
    if (progCurrent != NULL) {
        lineNum = progCurrent->lineNum;
    }
    varAppendData(((dataType *)vp)->dataList, lineNum);
}


static void progExecuteDIM(void *vp) {
    dimType *dt = (dimType *)vp;
    unsigned long int i;
    long d1;
    long d2;
    for (i = 0UL; i < dt->numElements; i++) {
        /* varDim reports what's wrong (already dimensioned, size, ...) */
        if (evalSubscripts(dt->dimList[i], &d1, &d2) || varDim(dt->dimList[i]->value, d1, d2)) {
            progHalt();
            return;
        }
    }
}


static void progExecuteEND(void *vp) {
    /* "Program may be restarted using CONT" (Manual, Appendix A) */
    if (progCurrent != NULL && progCurrent->lineNum != -1) {
        progStop = progCurrent;
    }
}


static void progExecuteFOR(void *vp) {
    forType *fp = (forType *)vp;
    long int top;
    /*
     * Running a FOR for a variable that already has an active loop (e.g. by
     * jumping back to the FOR line) restarts it: drop that loop and any loops
     * inside it, so the stack doesn't grow.
     */
    for (top = controlTop; top >= 0; top--) {
        if (control[top].isFor && !strcasecmp(((forType *)control[top].instruction)->startPoint->l->value, fp->startPoint->l->value)) {
            controlTop = top - 1;
            break;
        }
    }
    if (controlTop + 1 >= MAX_CONTROL) {
        utilErrorN(ERR_STACK, "too many nested FOR loops and GOSUBs");
        progHalt();
        return;
    }
    if (progAssign(fp->startPoint)) {
        return;
    }
    controlTop++;
    control[controlTop].isFor = 1;
    control[controlTop].line = progCurrent;
    control[controlTop].instruction = vp;
}


static void progExecuteGOSUB(void *vp) {
    gosubType *gp = (gosubType *)vp;
    progLineType *p;
    long int l;
    if (gp->targetLine == NULL || evalLong(gp->targetLine, &l)) {
        utilError("couldn't evaluate expression");
        goto err;
    }
    if ((p = progFindLine(l)) == NULL) {
        utilErrorN(ERR_LINE_NOT_FOUND, "line %ld not found", l);
        goto err;
    }
    if (controlTop + 1 >= MAX_CONTROL) {
        utilErrorN(ERR_STACK, "too many nested FOR loops and GOSUBs");
        goto err;
    }
    /* RETURN resumes after this instruction, so the rest of the line still runs */
    controlTop++;
    control[controlTop].isFor = 0;
    control[controlTop].line = progCurrent;
    control[controlTop].instruction = progCurrent->currentInstruction;
    progJump(p, NULL);
    return;
err:
    progHalt();
}


static void progExecuteGOTO(void *vp) {
    gotoType *gp = (gotoType *)vp;
    progLineType *p;
    long int l;
    if (gp->targetLine == NULL || evalLong(gp->targetLine, &l)) {
        utilError("couldn't evaluate expression");
        goto err;
    }
    if ((p = progFindLine(l)) == NULL) {
        utilErrorN(ERR_LINE_NOT_FOUND, "line %ld not found", l);
        goto err;
    }
    progJump(p, NULL);
    return;
err:
    progHalt();
}


static void progExecuteIF(void *vp) {
    ifType *ip = (ifType *)vp;
    progLineType *p;
    double condition;
    long int l;
    if (evalDouble(ip->expression, &condition)) {
        goto err;
    }
    if (condition == 0) {
        /* false: skip the rest of the line */
        progJump(progCurrent->next, NULL);
        return;
    }
    /* any nonzero value is true */
    if (ip->isGoto) {
        if (evalLong(ip->gotoOrInstructions, &l)) {
            goto err;
        }
        if ((p = progFindLine(l)) == NULL) {
            utilErrorN(ERR_LINE_NOT_FOUND, "line %ld not found", l);
            goto err;
        }
        progJump(p, NULL);
    } else if (ip->gotoOrInstructions != NULL) {
        progJump(progCurrent, ip->gotoOrInstructions);
    }
    return;
err:
    progHalt();
}


/*
 * INPUT shows a ? prompt. "Arithmetic numbers can be typed on the same line
 * separated by commas"; a string takes a line of its own, or the rest of the
 * current one after a comma. When a line runs out, ? asks for another.
 * Non-numeric input for a numeric variable is error 8 (Manual, INPUT).
 */
static void progExecuteINPUT(void *vp) {
    inputType *ip = (inputType *)vp;
    char *line = NULL;
    char *pos = NULL;
    char *end;
    char num[64];
    double d;
    unsigned long int i;
    for (i = 0L; i < ip->numVars; i++) {
        if (pos == NULL) {
            free(line);
            fputs("?", stdout);
            /* show the prompt before waiting for input */
            fflush(stdout);
            /* read through the io module, which shares stdin with the program reader */
            if ((line = ioReadLine()) == NULL) {
                utilErrorN(ERR_EOF, "end of input");
                goto err;
            }
            printColumn = 0;
            pos = line;
        }
        if (strchr(ip->varList[i]->value, '$') != NULL) {
            if (progSetVariable(ip->varList[i], pos)) {
                goto err;
            }
            pos = NULL;
            continue;
        }
        d = strtod(pos, &end);
        if (end == pos) {
            /* nothing but spaces before the comma or the end (just RETURN) reads as 0 */
            while (isspace((unsigned char)*end)) {
                end++;
            }
            if (*end != 0 && *end != ',') {
                utilErrorN(ERR_INPUT, "'%s' is not a number", pos);
                goto err;
            }
            d = 0;
        }
        while (isspace((unsigned char)*end)) {
            end++;
        }
        if (*end != 0 && *end != ',') {
            utilErrorN(ERR_INPUT, "'%s' is not a number", pos);
            goto err;
        }
        pos = *end == ',' ? end + 1 : NULL;
        formatNumber(num, sizeof(num), d);
        if (progSetVariable(ip->varList[i], num)) {
            goto err;
        }
    }
    free(line);
    return;
err:
    free(line);
    progHalt();
}


static void progExecuteLET(void *vp) {
    progAssign(((letType *)vp)->assignment);
}


void progExecuteLine(progLineType *p) {
    /* a line typed at the prompt starts at the left margin */
    printColumn = 0;
    progCurrent = p;
    if (progCurrent) {
        progCurrent->currentInstruction = progCurrent->firstInstruction;
    }
    progExecute();
    if (p != NULL && p->lineNum == -1) {
        progPurgeImmediateLine(p);
    }
}


static void progExecuteLIST(void *vp) {
    listType *lp = (listType *)vp;
    long int l1 = -1;
    long int l2 = -1;
    if (lp->startLine != NULL && evalLong(lp->startLine, &l1)) {
        progHalt();
        return;
    }
    if (lp->endLine == NULL) {
        /* LIST n lists only line n */
        l2 = l1;
    } else if (evalLong(lp->endLine, &l2)) {
        progHalt();
        return;
    }
    if (progList(1, l1, l2)) {
        progHalt();
    }
}


static void progExecuteLOAD(void *vp) {
    loadType *lp = (loadType *)vp;
    char *fn;
    if ((fn = eval(lp->fileName)) == NULL) {
        progHalt();
        return;
    }
    if (ioOpenInput(fn)) {
        utilErrorN(ERR_FILE_NOT_FOUND, "couldn't open [%s]", fn);
        progHalt();
    } else {
        /*
         * "LOAD ... clear[s] the old program from memory before loading the
         * new one" (Manual, ENTER). The file is read as input from here on.
         */
        progNew();
        progCurrent = NULL;
    }
    free(fn);
}


static void progExecuteNEW(void *vp) {
    progNew();
    /* the line being executed was just freed */
    progCurrent = NULL;
}


static void progExecuteNEXT(void *vp) {
    nextType *np = (nextType *)vp;
    forType *fp = NULL;
    char *s;
    double i, j;
    double k = 1;
    long dim1;
    long dim2;
    long int top;
    char d[64];
    /*
     * Find the loop for this variable; loops started inside it are abandoned.
     * The search stops at a GOSUB: a NEXT can't close a loop from outside the
     * subroutine.
     */
    for (top = controlTop; top >= 0 && control[top].isFor; top--) {
        fp = (forType *)control[top].instruction;
        if (!strcasecmp(fp->startPoint->l->value, np->iteratorVar->value)) {
            break;
        }
    }
    if (top < 0 || !control[top].isFor) {
        utilErrorN(ERR_NO_FOR, "NEXT %s without FOR", np->iteratorVar->value);
        goto err;
    }
    controlTop = top;
    if (evalSubscripts(fp->startPoint->l, &dim1, &dim2)) {
        goto err;
    }
    if ((s = varGetValue(fp->startPoint->l->value, dim1, dim2)) == NULL) {
        goto err;
    }
    i = strtod(s, NULL);
    free(s);
    if (evalDouble(fp->endPoint, &j)) {
        goto err;
    }
    if (fp->step != NULL && evalDouble(fp->step, &k)) {
        goto err;
    }
    formatNumber(d, sizeof(d), i + k);
    /* compare the value actually stored, so the last pass matches what the program sees */
    i = strtod(d, NULL);
    if (varSetValue(fp->startPoint->l->value, d, dim1, dim2)) {
        goto err;
    }
    if (k >= 0 ? i > j : i < j) {
        /* loop finished: carry on after the NEXT */
        controlTop--;
        return;
    }
    /* go round again, starting after the FOR */
    progJumpAfter(control[controlTop].line, control[controlTop].instruction);
    return;
err:
    progHalt();
}


static void progExecuteON(void *vp) {
    onType *op = (onType *)vp;
    progLineType plt;
    long int exp;
    /*
     * "The aexp must evaluate to a positive number which is then rounded to
     * the nearest positive integer ... up to 255. If the resulting number is 0
     * or is greater than the number of linenos in the list" control passes to
     * the next statement (Manual, ON).
     */
    if (evalLong(op->expression, &exp)) {
        progHalt();
        return;
    }
    if (exp < 0 || exp > 255) {
        utilErrorN(ERR_VALUE, "ON value must be 0 to 255");
        progHalt();
        return;
    }
    if (exp > 0) {
        if ((unsigned long int)exp <= op->numTargets) {
            plt.next = NULL;
            plt.firstInstruction = plt.lastInstruction = plt.currentInstruction = NULL;
            plt.lineNum = -1;
            if (progAppendInstruction(&plt, op->instruction->id, op->targetList[exp - 1])) {
                progHalt();
                return;
            }
            plt.firstInstruction->executeFunc(plt.firstInstruction);
            /* the target expression belongs to this ON, so free only the wrapper */
            free(plt.firstInstruction);
        }
    }
}


/* remove the top entry of the FOR/GOSUB stack, whichever kind it is */
static void progExecutePOP(void *vp) {
    if (controlTop >= 0) {
        controlTop--;
    }
}


/* write program output, keeping track of the column for PRINT's comma */
static void progOutput(const char *s) {
    for (; *s != 0; s++) {
        putchar(*s);
        printColumn = *s == '\n' ? 0 : printColumn + 1;
    }
}


static void progExecutePRINT(void *vp) {
    printType *pp = (printType *)vp;
    char *expression = NULL;
    unsigned long int i;
    /*
     * Each value ends the line unless a ',' (tab) or ';' follows it, so the
     * newline is held back until we know what comes next.
     */
    int pendingNewline = 0;
    for (i = 0; i < pp->numExpressions; i++) {
        if (pp->expressionList[i]->id == kwComma) {
            pendingNewline = 0;
            /* "A comma causes tabbing to the next tab location", every 10 columns */
            do {
                progOutput(" ");
            } while (printColumn % PRINT_TAB_WIDTH != 0);
        } else if (pp->expressionList[i]->id == kwSemicolon) {
            pendingNewline = 0;
        } else {
            if ((expression = eval(pp->expressionList[i])) == NULL) {
                progHalt();
                break;
            }
            if (pendingNewline) {
                progOutput("\n");
            }
            progOutput(expression);
            free(expression);
            expression = NULL;
            pendingNewline = 1;
        }
    }
    if (pendingNewline || pp->numExpressions == 0) {
        progOutput("\n");
    }
}


static void progExecuteREAD(void *vp) {
    readType *rp = (readType *)vp;
    char *s = NULL;
    char *end;
    unsigned long int i;
    for (i = 0L; i < rp->numVars; i++) {
        if ((s = varReadData()) == NULL) {
            utilErrorN(ERR_OUT_OF_DATA, "out of data");
            goto err;
        }
        /* "the data type in the DATA statement must match the variable type" */
        if (strchr(rp->varList[i]->value, '$') == NULL && s[0] != 0) {
            strtod(s, &end);
            if (end == s || *end != 0) {
                utilErrorN(ERR_INPUT, "DATA item '%s' is not a number", s);
                goto err;
            }
        }
        if (progSetVariable(rp->varList[i], s)) {
            goto err;
        }
        free(s);
        s = NULL;
    }
    return;
err:
    free(s);
    progHalt();
}


static void progExecuteREM(void *vp) {
}


static void progExecuteRESTORE(void *vp) {
    restoreType *rp = (restoreType *)vp;
    long int line = -1;
    if (rp->targetLine != NULL && evalLong(rp->targetLine, &line)) {
        progHalt();
        return;
    }
    varRestoreData(line);
}


static void progExecuteRETURN(void *vp) {
    long int top;
    /* FOR loops started inside the subroutine are discarded with it */
    for (top = controlTop; top >= 0 && control[top].isFor; top--) {
    }
    if (top < 0) {
        utilErrorN(ERR_RETURN, "RETURN without GOSUB");
        progHalt();
        return;
    }
    controlTop = top - 1;
    /* resume after the GOSUB (or the ON ... GOSUB) */
    progJumpAfter(control[top].line, control[top].instruction);
}


static void progExecuteRUN(void *vp) {
    varClearAll();
    controlTop = -1;
    progStop = NULL;
    progTrap = -1;
    progCurrent = prog;
    while (progCurrent) {
        progCurrent->currentInstruction = progCurrent->firstInstruction;
        while (progCurrent->currentInstruction) {
            if (progCurrent->currentInstruction->keyword == kwDATA) {
                progCurrent->currentInstruction->executeFunc(progCurrent->currentInstruction);
            }
            progCurrent->currentInstruction = progCurrent->currentInstruction->next;
        }
        progCurrent->currentInstruction = progCurrent->firstInstruction;
        progCurrent = progCurrent->next;
    }
    progCurrent = prog;
    progExecute();
}


static void progExecuteSAVE(void *vp) {
    saveType *sp = (saveType *)vp;
    char *fn = eval(sp->fileName);
    int fh = -1;
    if (fn == NULL) {
        goto err;
    }
    if ((fh = open(fn, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
        utilErrorN(ERR_FILE_NAME, "couldn't create [%s]", fn);
        goto err;
    }
    progList(fh, -1, -1);
    close(fh);
    free(fn);
    fh = -1;
    return;
err:
    if (fn != NULL) {
        free(fn);
    }
    if (fh != -1) {
        close(fh);
    }
    progHalt();
}


static void progExecuteSTOP(void *vp) {
    char message[64];
    if (progCurrent != NULL && progCurrent->lineNum != -1) {
        /* CONT resumes at the line after this one */
        progStop = progCurrent;
        /* PEEK(186)/PEEK(187) give the line of a STOP too */
        progErrorLine = progCurrent->lineNum;
        snprintf(message, sizeof(message), "%sSTOPPED AT LINE %ld\n", printColumn != 0 ? "\n" : "", progCurrent->lineNum);
        progOutput(message);
    }
}


static void progExecuteTRAP(void *vp) {
    trapType *tp = (trapType *)vp;
    long int l;
    if (tp->targetLine == NULL || evalLong(tp->targetLine, &l)) {
        progHalt();
        return;
    }
    if (l < 0 || l > 65535) {
        utilErrorN(ERR_VALUE, "TRAP value must be 0 to 65535");
        progHalt();
        return;
    }
    /*
     * The line is looked up when an error happens, so it needn't exist yet.
     * "The TRAP may be cleared by executing a TRAP statement with an aexp whose
     * value is from 32767 to 65535 (e.g., 40000)" (Manual, TRAP).
     */
    progTrap = l < 32767 ? l : -1;
}


static void progExit(void) {
    progNew();
}


static char *progFormatAssignment(void *vp) {
    assignmentType *ap = (assignmentType *)vp;
    char *s;
    if (ap == NULL || ap->assignment == NULL) {
        return(NULL);
    }
    if ((s = evalCode(ap->assignment)) == NULL) {
        return(NULL);
    }
    return(s);
}


static char *progFormatDATA(void *vp) {
    char *l = strmergez((const char **)((dataType *)vp)->dataList, ",");
    char *s;
    if (l == NULL) {
        return(NULL);
    }
    if ((s = malloc(strlen(l) + 6)) == NULL) {
        free(l);
        return(NULL);
    }
    strcpy(s, scanGetKeyword(kwDATA));
    strcat(s, " ");
    strcat(s, l);
    free(l);
    return(s);
}


static char *progFormatDefault(void *vp) {
    return(strdup(scanGetKeyword(((instructionType *)vp)->keyword)));
}


static char *progFormatDIM(void *vp) {
    dimType *dt = (dimType *)vp;
    char **parts = NULL;
    char *list = NULL;
    char *s = NULL;
    unsigned long int i;
    /* one string per element, e.g. "A(10)" or "B(N,2)", joined with commas */
    if ((parts = calloc(dt->numElements + 1, sizeof(char *))) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        return(NULL);
    }
    for (i = 0; i < dt->numElements; i++) {
        if ((parts[i] = evalCode(dt->dimList[i])) == NULL) {
            goto done;
        }
    }
    if ((list = strmergez((const char **)parts, ",")) != NULL) {
        s = strfmt("%s %s", scanGetKeyword(kwDIM), list);
        free(list);
    }
done:
    for (i = 0; i < dt->numElements; i++) {
        free(parts[i]);
    }
    free(parts);
    return(s);
}


static char *progFormatFOR(void *vp) {
    forType *ft = (forType *)vp;
    char *s = NULL;
    char *start = NULL;
    char *end = NULL;
    char *step = NULL;
    unsigned long int len = 15;
    if ((start = evalCode(ft->startPoint)) == NULL) {
        goto err;
    }
    len += strlen(start);
    if ((end = evalCode(ft->endPoint)) == NULL) {
        goto err;
    }
    len += strlen(end);
    if (ft->step != NULL) {
        if ((step = evalCode(ft->step)) == NULL) {
            goto err;
        }
        len += strlen(step);
    }
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwFOR));
    strcat(s, " ");
    strcat(s, start);
    strcat(s, " ");
    strcat(s, scanGetKeyword(kwTO));
    strcat(s, " ");
    strcat(s, end);
    if (step != NULL) {
        strcat(s, " ");
        strcat(s, scanGetKeyword(kwSTEP));
        strcat(s, " ");
        strcat(s, step);
    }
    free(start);
    free(end);
    free(step);
    return(s);
err:
    free(start);
    free(end);
    free(step);
    free(s);
    return(NULL);
}


static char *progFormatGOSUB(void *vp) {
    gosubType *gt = (gosubType *)vp;
    char *t = NULL;
    char *s = NULL;
    if (gt->targetLine != NULL) {
        if ((t = evalCode(gt->targetLine)) == NULL) {
            goto err;
        }
        if ((s = malloc(strlen(scanGetKeyword(kwGOSUB)) + strlen(t) + 2)) == NULL) {
            utilErrorN(ERR_MEMORY, "%s", memErr);
            goto err;
        }
        strcpy(s, scanGetKeyword(kwGOSUB));
        strcat(s, " ");
        strcat(s, t);
        free(t);
        t = NULL;
    }
    return(s);
err:
    if (s != NULL) {
        free(s);
    }
    if (t != NULL) {
        free(t);
    }
    return(NULL);
}


static char *progFormatGOTO(void *vp) {
    gotoType *gt = (gotoType *)vp;
    char *t = NULL;
    char *s = NULL;
    if (gt->targetLine != NULL) {
        if ((t = evalCode(gt->targetLine)) == NULL) {
            goto err;
        }
        if ((s = malloc(strlen(scanGetKeyword(kwGOTO)) + strlen(t) + 2)) == NULL) {
            utilErrorN(ERR_MEMORY, "%s", memErr);
            goto err;
        }
        strcpy(s, scanGetKeyword(kwGOTO));
        strcat(s, " ");
        strcat(s, t);
        free(t);
        t = NULL;
    }
    return(s);
err:
    if (s != NULL) {
        free(s);
    }
    if (t != NULL) {
        free(t);
    }
    return(NULL);
}


static char *progFormatIF(void *vp) {
    ifType *ft = (ifType *)vp;
    char *s = NULL;
    char *x = NULL;
    char *t = NULL;
    if (ft->expression == NULL) {
        goto err;
    }
    if ((x = evalCode(ft->expression)) == NULL) {
        utilError("unable to format if expression");
        goto err;
    }
    if (ft->gotoOrInstructions == NULL) {
        goto err;
    }
    if (ft->isGoto) {
        if ((t = evalCode(ft->gotoOrInstructions)) == NULL) {
            utilError("unable to format if target");
            goto err;
        }
    } else {
        if ((t = formatLine(ft->gotoOrInstructions)) == NULL) {
            utilError("unable to format if target");
            goto err;
        }
    }
    if ((s = malloc(strlen(x) + strlen(t) + strlen(scanGetKeyword(kwIF)) + strlen(scanGetKeyword(kwTHEN)) + 4)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwIF));
    strcat(s, " ");
    strcat(s, x);
    strcat(s, " ");
    strcat(s, scanGetKeyword(kwTHEN));
    strcat(s, " ");
    strcat(s, t);
    free(x);
    free(t);
    return(s);
err:
    if (x != NULL) {
        free(x);
    }
    if (t != NULL) {
        free(t);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatINPUT(void *vp) {
    inputType *ip = (inputType *)vp;
    unsigned long int i;
    unsigned long int len = 0;
    char *s = NULL;
    char *v = NULL;
    for (i = 0; i < ip->numVars; i++) {
        if ((v = evalCode(ip->varList[i])) == NULL) {
            goto err;
        }
        len += strlen(v);
        free(v);
        v = NULL;
    }
    len += ip->numVars;
    len += strlen(scanGetKeyword(kwINPUT));
    len += 2;
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwINPUT));
    strcat(s, " ");
    for (i = 0; i < ip->numVars; i++) {
        if ((v = evalCode(ip->varList[i])) == NULL) {
            goto err;
        }
        strcat(s, v);
        free(v);
        v = NULL;
        if (i < ip->numVars - 1) {
            strcat(s, ",");
        }
    }
    return(s);
err:
    if (v != NULL) {
        free(v);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatLET(void *vp) {
    letType *lp = (letType *)vp;
    char *s = NULL;
    char *a = NULL;
    if ((a = evalCode(lp->assignment)) == NULL) {
        utilError("expected assignment");
        goto err;
    }
    if ((s = malloc(strlen(a) + strlen(scanGetKeyword(kwLET)) + 2)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwLET));
    strcat(s, " ");
    strcat(s, a);
    free(a);
    return(s);
err:
    if (a != NULL) {
        free(a);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatLIST(void *vp) {
    listType *lt = (listType *)vp;
    unsigned long int len = 3;
    char *s = NULL;
    char *l1 = NULL;
    char *l2 = NULL;
    if (lt->startLine != NULL) {
        if ((l1 = evalCode(lt->startLine)) == NULL) {
            goto err;
        }
        len += strlen(l1);
    }
    if (lt->endLine != NULL) {
        if ((l2 = evalCode(lt->endLine)) == NULL) {
            goto err;
        }
        len += strlen(l2);
    }
    len += strlen(scanGetKeyword(kwLIST));
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwLIST));
    if (l1 != NULL) {
        strcat(s, " ");
        strcat(s, l1);
        free(l1);
    }
    if (l2 != NULL) {
        strcat(s, ",");
        strcat(s, l2);
        free(l2);
    }
    return(s);
err:
    if (l1 != NULL) {
        free(l1);
    }
    if (l2 != NULL) {
        free(l2);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatLOAD(void *vp) {
    loadType *lt = (loadType *)vp;
    char *s = NULL;
    char *fn = NULL;
    if ((fn = evalCode(lt->fileName)) == NULL) {
        goto err;
    }
    if ((s = malloc(strlen(fn) + strlen(scanGetKeyword(kwLOAD)) + 2)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwLOAD));
    strcat(s, " ");
    strcat(s, fn);
    free(fn);
    return(s);
err:
    if (fn != NULL) {
        free(fn);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatNEXT(void *vp) {
    nextType *nt = (nextType *)vp;
    char *s = NULL;
    char *v = NULL;
    if ((v = evalCode(nt->iteratorVar)) == NULL) {
        goto err;
    }
    if ((s = malloc(strlen(v) + strlen(scanGetKeyword(kwNEXT)) + 2)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwNEXT));
    strcat(s, " ");
    strcat(s, v);
    free(v);
    return(s);
err:
    if (v != NULL) {
        free(v);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatON(void *vp) {
    onType *ot = (onType *)vp;
    char *s = NULL;
    char *e = NULL;
    char *i = NULL;
    char *t = NULL;
    unsigned long int len = 0UL;
    unsigned long int j;
    if ((e = evalCode(ot->expression)) == NULL) {
        goto err;
    }
    len += strlen(e);
    i = scanGetKeyword(ot->instruction->id);
    len += strlen(i);
    len += strlen(scanGetKeyword(kwON));
    len += 4;
    len += ot->numTargets;
    for (j = 0; j < ot->numTargets; j++) {
        if ((t = evalCode(ot->targetList[j])) == NULL) {
            goto err;
        }
        len += strlen(t);
        free(t);
    }
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwON));
    strcat(s, " ");
    strcat(s, e);
    strcat(s, " ");
    strcat(s, i);
    strcat(s, " ");
    for (j = 0; j < ot->numTargets; j++) {
        if ((t = evalCode(ot->targetList[j])) == NULL) {
            goto err;
        }
        strcat(s, t);
        if (j < ot->numTargets - 1) {
            strcat(s, ",");
        }
        free(t);
    }
    free(e);
    return(s);
err:
    if (e != NULL) {
        free(e);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatPRINT(void *vp) {
    printType *pp = (printType *)vp;
    char *s = NULL;
    char *e = NULL;
    unsigned long int len = strlen(scanGetKeyword(kwPRINT)) + 2;
    unsigned long int i;
    for (i = 0UL; i < pp->numExpressions; i++) {
        if (pp->expressionList[i]->id == kwComma || pp->expressionList[i]->id == kwSemicolon) {
            len += 2;
        } else {
            if ((e = evalCode(pp->expressionList[i])) == NULL) {
                goto err;
            }
            len += strlen(e);
            len++;
            free(e);
            e = NULL;
        }
    }
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwPRINT));
    /* no trailing spaces: none after a bare PRINT or a final ',' or ';' */
    if (pp->numExpressions > 0) {
        strcat(s, " ");
    }
    for (i = 0UL; i < pp->numExpressions; i++) {
        if (pp->expressionList[i]->id == kwComma) {
            strcat(s, i + 1 < pp->numExpressions ? ", " : ",");
        } else if (pp->expressionList[i]->id == kwSemicolon) {
            strcat(s, i + 1 < pp->numExpressions ? "; " : ";");
        } else {
            if ((e = evalCode(pp->expressionList[i])) == NULL) {
                goto err;
            }
            strcat(s, e);
            free(e);
            e = NULL;
        }
    }
    return(s);
err:
    if (e != NULL) {
        free(e);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatREAD(void *vp) {
    readType *rp = (readType *)vp;
    unsigned long int i;
    unsigned long int len = 0;
    char *s = NULL;
    char *v = NULL;
    for (i = 0; i < rp->numVars; i++) {
        if ((v = evalCode(rp->varList[i])) == NULL) {
            goto err;
        }
        len += strlen(v);
        free(v);
        v = NULL;
    }
    len += rp->numVars;
    len += strlen(scanGetKeyword(kwREAD));
    len += 2;
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwREAD));
    strcat(s, " ");
    for (i = 0; i < rp->numVars; i++) {
        if ((v = evalCode(rp->varList[i])) == NULL) {
            goto err;
        }
        strcat(s, v);
        free(v);
        v = NULL;
        if (i < rp->numVars - 1) {
            strcat(s, ",");
        }
    }
    return(s);
err:
    if (v != NULL) {
        free(v);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatREM(void *vp) {
    char *s = malloc(strlen(((remType *)vp)->remark) + 5);
    if (s == NULL) {
        return(NULL);
    }
    strcpy(s, scanGetKeyword(kwREM));
    strcat(s, " ");
    strcat(s, ((remType *)vp)->remark);
    return(s);
}


static char *progFormatRESTORE(void *vp) {
    restoreType *rt = (restoreType *)vp;
    char *t = NULL;
    char *s = NULL;
    if (rt->targetLine == NULL) {
        /* plain RESTORE: returning NULL here made LIST and SAVE drop it */
        return(strdup(scanGetKeyword(kwRESTORE)));
    }
    if ((t = evalCode(rt->targetLine)) == NULL) {
        goto err;
    }
    if ((s = malloc(strlen(scanGetKeyword(kwRESTORE)) + strlen(t) + 2)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwRESTORE));
    strcat(s, " ");
    strcat(s, t);
    free(t);
    t = NULL;
    return(s);
err:
    if (s != NULL) {
        free(s);
    }
    if (t != NULL) {
        free(t);
    }
    return(NULL);
}


static char *progFormatSAVE(void *vp) {
    saveType *st = (saveType *)vp;
    char *s = NULL;
    char *fn = NULL;
    if ((fn = evalCode(st->fileName)) == NULL) {
        goto err;
    }
    if ((s = malloc(strlen(fn) + strlen(scanGetKeyword(kwSAVE)) + 2)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    strcpy(s, scanGetKeyword(kwSAVE));
    strcat(s, " ");
    strcat(s, fn);
    free(fn);
    return(s);
err:
    if (fn != NULL) {
        free(fn);
    }
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}


static char *progFormatTRAP(void *vp) {
    trapType *tt = (trapType *)vp;
    char *t = NULL;
    char *s = NULL;
    if (tt->targetLine != NULL) {
        if ((t = evalCode(tt->targetLine)) == NULL) {
            goto err;
        }
        if ((s = malloc(strlen(scanGetKeyword(kwTRAP)) + strlen(t) + 2)) == NULL) {
            utilErrorN(ERR_MEMORY, "%s", memErr);
            goto err;
        }
        strcpy(s, scanGetKeyword(kwTRAP));
        strcat(s, " ");
        strcat(s, t);
        free(t);
        t = NULL;
    }
    return(s);
err:
    if (s != NULL) {
        free(s);
    }
    if (t != NULL) {
        free(t);
    }
    return(NULL);
}


static void progFreeAssignment(void *vp) {
    assignmentType *ap = (assignmentType *)vp;
    if (ap != NULL) {
        if (ap->assignment != NULL) {
            progDeleteExpression(ap->assignment);
        }
        free(ap);
    }
}


static void progFreeDATA(void *vp) {
    dataType *dp = (dataType *)vp;
    if (dp != NULL) {
        if (dp->dataList != NULL) {
            free(dp->dataList);
        }
        free(dp);
    }
}


static void progFreeDefault(void *vp) {
    if (vp != NULL) {
        free(vp);
    }
}


static void progFreeDIM(void *vp) {
    dimType *dp = (dimType *)vp;
    unsigned long int i;
    if (dp != NULL) {
        if (dp->dimList != NULL) {
            for (i = 0; i < dp->numElements; i++) {
                /* each entry is an identifier with its size expressions in l and r */
                progDeleteExpression(dp->dimList[i]);
            }
            free(dp->dimList);
        }
        free(dp);
    }
}


static void progFreeFOR(void *vp) {
    forType *fp = (forType *)vp;
    if (fp != NULL) {
        if (fp->startPoint != NULL) {
            progDeleteExpression(fp->startPoint);
        }
        if (fp->endPoint != NULL) {
            progDeleteExpression(fp->endPoint);
        }
        if (fp->step != NULL) {
            progDeleteExpression(fp->step);
        }
        free(fp);
    }
}


static void progFreeGOSUB(void *vp) {
    gosubType *gp = (gosubType *)vp;
    if (gp != NULL) {
        if (gp->targetLine != NULL) {
            progDeleteExpression(gp->targetLine);
        }
        free(gp);
    }
}


static void progFreeGOTO(void *vp) {
    gotoType *gp = (gotoType *)vp;
    if (gp != NULL) {
        if (gp->targetLine != NULL) {
            progDeleteExpression(gp->targetLine);
        }
        free(gp);
    }
}


static void progFreeIF(void *vp) {
    ifType *ip = (ifType *)vp;
    if (ip != NULL) {
        if (ip->expression != NULL) {
            progDeleteExpression(ip->expression);
        }
        if (ip->isGoto) {
            progDeleteExpression((symbolType *)ip->gotoOrInstructions);
        } else {
            progDeleteInstructions((instructionType *)ip->gotoOrInstructions);
        }
        free(ip);
    }
}


static void progFreeINPUT(void *vp) {
    inputType *ip = (inputType *)vp;
    unsigned long int i;
    if (ip != NULL) {
        if (ip->varList != NULL) {
            for (i = 0; i < ip->numVars; i++) {
                if (ip->varList[i] != NULL) {
                    progDeleteExpression(ip->varList[i]);
                }
            }
            free(ip->varList);
        }
        free(ip);
    }
}


static void progFreeLET(void *vp) {
    letType *lp = (letType *)vp;
    if (lp != NULL) {
        if (lp->assignment != NULL) {
            progDeleteExpression(lp->assignment);
        }
        free(lp);
    }
}


static void progFreeLIST(void *vp) {
    listType *lp = (listType *)vp;
    if (lp != NULL) {
        if (lp->startLine != NULL) {
            progDeleteExpression(lp->startLine);
        }
        if (lp->endLine != NULL) {
            progDeleteExpression(lp->endLine);
        }
        free(lp);
    }
}


static void progFreeLOAD(void *vp) {
    loadType *lp = (loadType *)vp;
    if (lp != NULL) {
        if (lp->fileName != NULL) {
            progDeleteExpression(lp->fileName);
        }
        free(lp);
    }
}


static void progFreeNEXT(void *vp) {
    nextType *np = (nextType *)vp;
    if (np != NULL) {
        if (np->iteratorVar != NULL) {
            progDeleteExpression(np->iteratorVar);
        }
        free(np);
    }
}


static void progFreeON(void *vp) {
    onType *op = (onType *)vp;
    unsigned long int i;
    if (op != NULL) {
        if (op->expression != NULL) {
            progDeleteExpression(op->expression);
        }
        if (op->instruction != NULL) {
            progDeleteExpression(op->instruction);
        }
        if (op->targetList != NULL) {
            for (i = 0; i < op->numTargets; i++) {
                if (op->targetList[i] != NULL) {
                    progDeleteExpression(op->targetList[i]);
                }
            }
            free(op->targetList);
        }
        free(op);
    }
}


static void progFreePRINT(void *vp) {
    printType *pp = (printType *)vp;
    unsigned long int i;
    if (pp != NULL) {
        if (pp->expressionList != NULL) {
            for (i = 0; i < pp->numExpressions; i++) {
                if (pp->expressionList[i] != NULL) {
                    progDeleteExpression(pp->expressionList[i]);
                }
            }
            free(pp->expressionList);
        }
        free(pp);
    }
}


static void progFreeREAD(void *vp) {
    readType *rp = (readType *)vp;
    unsigned long int i;
    if (rp != NULL) {
        if (rp->varList != NULL) {
            for (i = 0; i < rp->numVars; i++) {
                /* each entry is an identifier with its subscripts in l and r */
                progDeleteExpression(rp->varList[i]);
            }
            free(rp->varList);
        }
        free(rp);
    }
}


static void progFreeREM(void *vp) {
    remType *rp = (remType *)vp;
    if (rp != NULL) {
        if (rp->remark != NULL) {
            free(rp->remark);
        }
        free(rp);
    }
}


static void progFreeRESTORE(void *vp) {
    restoreType *rp = (restoreType *)vp;
    if (rp != NULL) {
        if (rp->targetLine != NULL) {
            progDeleteExpression(rp->targetLine);
        }
        free(rp);
    }
}


static void progFreeSAVE(void *vp) {
    saveType *sp = (saveType *)vp;
    if (sp != NULL) {
        if (sp->fileName != NULL) {
            progDeleteExpression(sp->fileName);
        }
        free(sp);
    }
}


static void progFreeTRAP(void *vp) {
    trapType *tp = (trapType *)vp;
    if (tp != NULL) {
        if (tp->targetLine) {
            progDeleteExpression(tp->targetLine);
        }
        free(tp);
    }
}


int progInit(void) {
    atexit(progExit);
    srand((unsigned int)time(NULL));
    return(0);
}


progLineType *progInsertLine(long int l) {
    progLineType *p = malloc(sizeof(progLineType));
    progLineType *n;
    if (p == NULL) {
        return(NULL);
    }
    p->next = NULL;
    p->lineNum = l;
    p->firstInstruction = p->lastInstruction = p->currentInstruction = NULL;
    if (prog == NULL) {
        prog = p;
        return(p);
    } else if (prog->lineNum > l) {
        /* new first line */
        p->next = prog;
        prog = p;
        return(p);
    } else if (prog->lineNum == l) {
        p->next = prog->next;
        progForgetLine(prog);
        progDeleteInstructions(prog->firstInstruction);
        free(prog);
        prog = p;
        return(p);
    }
    n = prog;
    while (n->next && n->next->lineNum < l) {
        n = n->next;
    }
    if (n->next) {
        if (n->next->lineNum == l) {
            p->next = n->next->next;
            progForgetLine(n->next);
            progDeleteInstructions(n->next->firstInstruction);
            free(n->next);
            n->next = p;
            return(p);
        }
    }
    p->next = n->next;
    n->next = p;
    return(p);
}


static int progList(int fh, long int start, long int end) {
    progLineType *p = prog;
    instructionType *i;
    char *e = NULL;
    char *s = NULL;
    char *ss = NULL;
    unsigned long int len = 1024;
    unsigned long int eLen = 0;
    unsigned long int sLen = 0;
    int rc = 1;
    /* this writes to fh directly, so send anything printf has buffered first */
    fflush(stdout);
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    while (p && (p->lineNum <= end || end == -1)) {
        if (start != -1 && p->lineNum < start) {
            p = p->next;
            continue;
        }
        sprintf(s, "%li ", p->lineNum);
        i = p->firstInstruction;
        while (i) {
            if ((e = i->formatFunc(i)) != NULL) {
                eLen = strlen(e);
                sLen = strlen(s);
                /* room for this text, " : " and the terminator */
                while (sLen + eLen + 4 > len) {
                    len <<= 1;
                    if ((ss = realloc(s, len)) == NULL) {
                        utilErrorN(ERR_MEMORY, "%s", memErr);
                        goto err;
                    }
                    s = ss;
                }
                strcat(s, e);
                if (i->next != NULL) {
                    strcat(s, " : ");
                }
                free(e);
                e = NULL;
            }
            i = i->next;
        }
        write(fh, s, strlen(s));
        s[0] = '\n';
        write(fh, s, 1);
        p = p->next;
    }
    rc = 0;
err:
    if (e != NULL) {
        free(e);
    }
    if (s != NULL) {
        free(s);
    }
    return(rc);
}


int progNew(void) {
    progLineType *p = prog;
    progLineType *t;
    while (p) {
        t = p->next;
        progDeleteInstructions(p->firstInstruction);
        free(p);
        p = t;
    }
    prog = NULL;
    controlTop = -1;
    progStop = NULL;
    progTrap = -1;
    /* "NEW clears BASIC's internal symbol table" (Manual, NEW); DEG stays until RAD */
    varClearAll();
    return(0);
}


void progRemoveLine(long int l) {
    progLineType *p = progFindLine(l);
    if (p != NULL) {
        progDeleteLine(p);
    }
}


/*
 * Bounded write into eval's result buffer (maxStringLen + 1 bytes).
 */
static void setResult(char *r, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(r, (size_t)maxStringLen + 1, fmt, vl);
    va_end(vl);
}


/*
 * Store number d as eval's result. A result that isn't a number is a value
 * error (3); one of 1E98 or more, or below 1E-99, is error 11, as on the Atari
 * (Manual, Appendix B).
 */
static int numberResult(char *r, double d) {
    if (isnan(d)) {
        utilErrorN(ERR_VALUE, "value error");
        return(1);
    }
    if (isinf(d) || fabs(d) >= 1e98) {
        utilErrorN(ERR_OVERFLOW, "number too large");
        return(1);
    }
    if (d != 0 && fabs(d) < 1e-99) {
        utilErrorN(ERR_OVERFLOW, "number too small");
        return(1);
    }
    formatNumber(r, (size_t)maxStringLen + 1, d);
    return(0);
}


/* write a number into eval's result buffer, bailing out of eval if it's out of range */
#define SET_NUMBER(r, d) do { if (numberResult((r), (d))) { goto err; } } while (0)


/* evaluate a sub-expression, bailing out of eval if it fails */
#define EVAL_ARG(dst, e) do { if (((dst) = eval(e)) == NULL) { goto err; } } while (0)

/*
 * Make string s itself eval's result, instead of copying it into the
 * fixed-size buffer, so strings can be as long as their DIM size.
 */
#define TAKE_RESULT(s) do { free(r); r = (s); (s) = NULL; } while (0)


static char *eval(symbolType *expr) {
    char *r = NULL;
    char *left = NULL;
    char *right = NULL;
    double o1 = 0;
    double o2 = 0;
    int cmp;
    int t = 0;
    if (expr == NULL) {
        return(NULL);
    }
    if ((r = malloc(maxStringLen + 1)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    r[0] = 0;
    switch (expr->id) {
        case kwString:
            if ((left = strdup(expr->value)) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                goto err;
            }
            TAKE_RESULT(left);
            break;
        case kwNumeric:
            SET_NUMBER(r, strtod(expr->value, NULL));
            break;
        case kwIdentifier:
            /* subscripts index an array, or select a substring of a string */
            if ((right = progGetVariable(expr)) == NULL) {
                goto err;
            }
            TAKE_RESULT(right);
            break;
        case kwOpAdd:
        case kwOpSub:
        case kwOpMul:
        case kwOpDiv:
        case kwOpExp:
            EVAL_ARG(left, expr->l);
            EVAL_ARG(right, expr->r);
            o1 = strtod(left, NULL);
            o2 = strtod(right, NULL);
            switch (expr->id) {
                case kwOpAdd:
                    o1 = o1 + o2;
                    break;
                case kwOpSub:
                    o1 = o1 - o2;
                    break;
                case kwOpMul:
                    o1 = o1 * o2;
                    break;
                case kwOpDiv:
                    if (o2 == 0) {
                        utilErrorN(ERR_OVERFLOW, "division by zero");
                        goto err;
                    }
                    o1 = o1 / o2;
                    break;
                default:
                    o1 = pow(o1, o2);
                    break;
            }
            SET_NUMBER(r, o1);
            break;
        case kwLogicalLT:
        case kwLogicalLTE:
        case kwLogicalEQ:
        case kwLogicalNE:
        case kwLogicalGT:
        case kwLogicalGTE:
            EVAL_ARG(left, expr->l);
            EVAL_ARG(right, expr->r);
            /* the parser marks comparisons of strings with a value of "$" */
            if (expr->value != NULL && expr->value[0] == '$') {
                cmp = strcmp(left, right);
            } else {
                o1 = strtod(left, NULL);
                o2 = strtod(right, NULL);
                cmp = (o1 > o2) - (o1 < o2);
            }
            switch (expr->id) {
                case kwLogicalLT:
                    t = cmp < 0;
                    break;
                case kwLogicalLTE:
                    t = cmp <= 0;
                    break;
                case kwLogicalEQ:
                    t = cmp == 0;
                    break;
                case kwLogicalNE:
                    t = cmp != 0;
                    break;
                case kwLogicalGT:
                    t = cmp > 0;
                    break;
                default:
                    t = cmp >= 0;
                    break;
            }
            setResult(r, "%d", t ? 1 : 0);
            break;
        case kwAssignment:
            EVAL_ARG(right, expr->r);
            if (progSetVariable(expr->l, right)) {
                goto err;
            }
            TAKE_RESULT(right);
            break;
        case kwSignPlus:
        case kwSignMinus:
            EVAL_ARG(right, expr->r);
            o2 = strtod(right, NULL);
            SET_NUMBER(r, expr->id == kwSignMinus ? 0 - o2 : o2);
            break;
        case kwSubExpression:
            EVAL_ARG(right, expr->r);
            TAKE_RESULT(right);
            break;
        case kwERL:
            SET_NUMBER(r, (double)progErrorLine);
            break;
        case kwERR:
            if ((left = strdup(progErrorMessage)) == NULL) {
                utilErrorN(ERR_MEMORY, "%s", memErr);
                goto err;
            }
            TAKE_RESULT(left);
            break;
        case kwABS:
        case kwASC:
        case kwATN:
        case kwCLOG:
        case kwCOS:
        case kwEXP:
        case kwINT:
        case kwLEN:
        case kwLOG:
        case kwRND:
        case kwSGN:
        case kwSIN:
        case kwSQR:
        case kwVAL:
        case kwCHR:
        case kwSTR:
        case kwPEEK:
            EVAL_ARG(left, expr->l);
            o1 = strtod(left, NULL);
            switch (expr->id) {
                case kwABS:
                    SET_NUMBER(r, fabs(o1));
                    break;
                case kwASC:
                    SET_NUMBER(r, (double)(unsigned char)left[0]);
                    break;
                case kwATN:
                    SET_NUMBER(r, progTrig(kwATN, o1));
                    break;
                case kwCLOG:
                    if (o1 <= 0) {
                        utilErrorN(ERR_VALUE, "CLOG of a number that isn't positive");
                        goto err;
                    }
                    SET_NUMBER(r, log10(o1));
                    break;
                case kwCOS:
                    SET_NUMBER(r, progTrig(kwCOS, o1));
                    break;
                case kwEXP:
                    SET_NUMBER(r, exp(o1));
                    break;
                case kwINT:
                    /* "the greatest integer less than or equal to": INT(-14.7) is -15 */
                    SET_NUMBER(r, floor(o1));
                    break;
                case kwLEN:
                    SET_NUMBER(r, (double)strlen(left));
                    break;
                case kwLOG:
                    if (o1 <= 0) {
                        utilErrorN(ERR_VALUE, "LOG of a number that isn't positive");
                        goto err;
                    }
                    SET_NUMBER(r, log(o1));
                    break;
                case kwPEEK:
                    /*
                     * Only the locations a TRAP handler reads (Manual, TRAP and
                     * Appendix I): 195 is the error number, 186 and 187 the line.
                     */
                    if (o1 == 195) {
                        SET_NUMBER(r, (double)progErrorCode);
                    } else if (o1 == 186) {
                        SET_NUMBER(r, (double)(progErrorLine & 255));
                    } else if (o1 == 187) {
                        SET_NUMBER(r, (double)(progErrorLine >> 8));
                    } else {
                        utilErrorN(ERR_VALUE, "PEEK(%g) isn't available; only 186, 187 and 195 are", o1);
                        goto err;
                    }
                    break;
                case kwRND:
                    /* [0, 1): RAND_MAX itself must not give 1 */
                    SET_NUMBER(r, (double)rand() / ((double)RAND_MAX + 1.0));
                    break;
                case kwSGN:
                    SET_NUMBER(r, (double)((o1 > 0) - (o1 < 0)));
                    break;
                case kwSIN:
                    SET_NUMBER(r, progTrig(kwSIN, o1));
                    break;
                case kwSQR:
                    if (o1 < 0) {
                        utilErrorN(ERR_VALUE, "SQR of a negative number");
                        goto err;
                    }
                    SET_NUMBER(r, sqrt(o1));
                    break;
                case kwVAL: {
                    char *end;
                    /* "not possible to use the VAL function with a string that does not start with a number" */
                    o1 = strtod(left, &end);
                    if (end == left) {
                        utilErrorN(ERR_BAD_STRING, "VAL of \"%s\", which isn't a number", left);
                        goto err;
                    }
                    SET_NUMBER(r, o1);
                    break;
                }
                case kwCHR:
                    /* rounded like other integer arguments; an out-of-range double to char is undefined */
                    o1 = floor(o1 + 0.5);
                    if (!(o1 >= 0 && o1 <= 255)) {
                        utilErrorN(ERR_VALUE, "value out of range");
                        goto err;
                    }
                    r[0] = (char)(int)o1;
                    r[1] = 0;
                    break;
                default: /* kwSTR */
                    SET_NUMBER(r, o1);
                    break;
            }
            break;
        case kwOR:
        case kwAND:
            EVAL_ARG(left, expr->l);
            EVAL_ARG(right, expr->r);
            o1 = strtod(left, NULL);
            o2 = strtod(right, NULL);
            t = (expr->id == kwOR) ? (o1 || o2) : (o1 && o2);
            setResult(r, "%d", t ? 1 : 0);
            break;
        case kwNOT:
            EVAL_ARG(left, expr->r);
            setResult(r, "%d", strtod(left, NULL) ? 0 : 1);
            break;
        default:
            break;
    }
    free(left);
    free(right);
    return(r);
err:
    free(r);
    free(left);
    free(right);
    return(NULL);
}


/*
 * Rebuild the source text of an expression (for LIST and SAVE). Returns a
 * newly allocated string, or NULL on error.
 */
static char *evalCode(symbolType *exp) {
    char *s = NULL;
    char *l = NULL;
    char *r = NULL;
    const char *op = NULL;
    if (exp == NULL) {
        return(strdup(""));
    }
    switch (exp->id) {
        case kwAssignment:
        case kwLogicalEQ:
            op = "=";
            break;
        case kwOpAdd:
            op = "+";
            break;
        case kwOpSub:
            op = "-";
            break;
        case kwOpMul:
            op = "*";
            break;
        case kwOpDiv:
            op = "/";
            break;
        case kwOpExp:
            op = "^";
            break;
        case kwLogicalLT:
            op = "<";
            break;
        case kwLogicalLTE:
            op = "<=";
            break;
        case kwLogicalNE:
            op = "<>";
            break;
        case kwLogicalGT:
            op = ">";
            break;
        case kwLogicalGTE:
            op = ">=";
            break;
        case kwAND:
            op = "and";
            break;
        case kwOR:
            op = "or";
            break;
        default:
            break;
    }
    if (op != NULL) {
        if ((l = evalCode(exp->l)) != NULL && (r = evalCode(exp->r)) != NULL) {
            s = strfmt("%s %s %s", l, op, r);
        }
    } else {
        switch (exp->id) {
            case kwSignPlus:
            case kwSignMinus:
                if ((r = evalCode(exp->r)) != NULL) {
                    s = strfmt("%c%s", exp->id == kwSignPlus ? '+' : '-', r);
                }
                break;
            case kwIdentifier:
                if (exp->l == NULL) {
                    s = strdup(exp->value);
                } else if ((l = evalCode(exp->l)) != NULL) {
                    if (exp->r == NULL) {
                        s = strfmt("%s(%s)", exp->value, l);
                    } else if ((r = evalCode(exp->r)) != NULL) {
                        s = strfmt("%s(%s,%s)", exp->value, l, r);
                    }
                }
                break;
            case kwNumeric:
                s = strdup(exp->value);
                break;
            case kwString:
                s = strfmt("\"%s\"", exp->value);
                break;
            case kwSubExpression:
                if ((r = evalCode(exp->r)) != NULL) {
                    s = strfmt("(%s)", r);
                }
                break;
            case kwABS:
            case kwASC:
            case kwATN:
            case kwCLOG:
            case kwCOS:
            case kwEXP:
            case kwINT:
            case kwLEN:
            case kwLOG:
            case kwRND:
            case kwSGN:
            case kwSIN:
            case kwSQR:
            case kwVAL:
            case kwCHR:
            case kwSTR:
            case kwPEEK:
                if ((l = evalCode(exp->l)) != NULL) {
                    s = strfmt("%s(%s)", scanGetKeyword(exp->id), l);
                }
                break;
            case kwNOT:
                if ((r = evalCode(exp->r)) != NULL) {
                    s = strfmt("not %s", r);
                }
                break;
            case kwERL:
            case kwERR:
                s = strdup(scanGetKeyword(exp->id));
                break;
            default:
                s = strdup("");
                break;
        }
    }
    free(l);
    free(r);
    return(s);
}


static char *formatLine(instructionType *i) {
    char *e = NULL;
    char *s = NULL;
    char *ss = NULL;
    unsigned long int len = 1024;
    unsigned long int eLen = 0;
    unsigned long int sLen = 0;
    if ((s = malloc(len)) == NULL) {
        utilErrorN(ERR_MEMORY, "%s", memErr);
        goto err;
    }
    s[0] = 0;
    while (i) {
        if ((e = i->formatFunc(i)) != NULL) {
            eLen = strlen(e);
            sLen = strlen(s);
            /* room for this text, " : " and the terminator */
            while (sLen + eLen + 4 > len) {
                len <<= 1;
                if ((ss = realloc(s, len)) == NULL) {
                    utilErrorN(ERR_MEMORY, "%s", memErr);
                    goto err;
                }
                s = ss;
            }
            strcat(s, e);
            if (i->next != NULL) {
                strcat(s, " : ");
            }
            free(e);
            e = NULL;
        }
        i = i->next;
    }
    return(s);
err:
    if (s != NULL) {
        free(s);
    }
    return(NULL);
}
