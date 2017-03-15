/*
 * prog.c
 */

#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "container.h"
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

static stackType(1024) callStack;
static stackType(1024) forLineStack;
static stackType(1024) forInstructionStack;

static progLineType *prog = NULL;
static progLineType *progCurrent = NULL;
static progLineType *progTrap = NULL;
static progLineType *progStop = NULL;
static char *memErr = "unable to allocate memory";


/*
 * LOCAL FUNCTIONS
 */

static void progCleanupNumeric(char *s);
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
static int isNumeric(char *s);
static char *evalCode(symbolType *params);
static char *formatLine(instructionType *i);
static char *eval(symbolType *params);


int progAppendInstruction(progLineType *p, keywords keyword, ...) {
	instructionType *i = NULL;
	va_list vl;
	va_start(vl, keyword);
	switch (keyword) {
		case kwAssignment:
			if ((i = malloc(sizeof(assignmentType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteAssignment;
			i->formatFunc = progFormatAssignment;
			i->freeFunc = progFreeAssignment;
			((assignmentType *)i)->assignment = va_arg(vl, symbolType *);
			break;
		case kwBYE:
			if ((i = malloc(sizeof(byeType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteBYE;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwCLR:
			if ((i = malloc(sizeof(clrType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteCLR;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwCLS:
			if ((i = malloc(sizeof(clsType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteCLS;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwCONT:
			if ((i = malloc(sizeof(contType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteCONT;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwDATA:
			if ((i = malloc(sizeof(dataType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteDATA;
			i->formatFunc = progFormatDATA;
			i->freeFunc = progFreeDATA;
			((dataType *)i)->dataList = va_arg(vl, char **);
			break;
		case kwDIM:
			if ((i = malloc(sizeof(dimType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteDIM;
			i->formatFunc = progFormatDIM;
			i->freeFunc = progFreeDIM;
			((dimType *)i)->dimList = va_arg(vl, symbolType **);
			((dimType *)i)->numElements = va_arg(vl, unsigned long int);
			break;
		case kwEND:
			if ((i = malloc(sizeof(endType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteEND;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwFOR:
			if ((i = malloc(sizeof(forType))) == NULL) {
				utilError(memErr);
				goto err;
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
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteGOSUB;
			i->formatFunc = progFormatGOSUB;
			i->freeFunc = progFreeGOSUB;
			((gosubType *)i)->targetLine = va_arg(vl, symbolType *);
			break;
		case kwGOTO:
			if ((i = malloc(sizeof(gotoType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteGOTO;
			i->formatFunc = progFormatGOTO;
			i->freeFunc = progFreeGOTO;
			((gotoType *)i)->targetLine = va_arg(vl, symbolType *);
			break;
		case kwIF:
			if ((i = malloc(sizeof(ifType))) == NULL) {
				utilError(memErr);
				goto err;
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
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteINPUT;
			i->formatFunc = progFormatINPUT;
			i->freeFunc = progFreeINPUT;
			((inputType *)i)->varList = va_arg(vl, symbolType **);
			((inputType *)i)->numVars = va_arg(vl, unsigned long int);
			break;
		case kwLET:
			if ((i = malloc(sizeof(letType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteLET;
			i->formatFunc = progFormatLET;
			i->freeFunc = progFreeLET;
			((letType *)i)->assignment = va_arg(vl, symbolType *);
			break;
		case kwLIST:
			if ((i = malloc(sizeof(listType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteLIST;
			i->formatFunc = progFormatLIST;
			i->freeFunc = progFreeLIST;
			((listType *)i)->startLine = va_arg(vl, symbolType *);
			((listType *)i)->endLine = va_arg(vl, symbolType *);
			break;
		case kwLOAD:
			if ((i = malloc(sizeof(loadType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteLOAD;
			i->formatFunc = progFormatLOAD;
			i->freeFunc = progFreeLOAD;
			((loadType *)i)->fileName = va_arg(vl, symbolType *);
			break;
		case kwNEW:
			if ((i = malloc(sizeof(newType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteNEW;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwNEXT:
			if ((i = malloc(sizeof(nextType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteNEXT;
			i->formatFunc = progFormatNEXT;
			i->freeFunc = progFreeNEXT;
			((nextType *)i)->iteratorVar = va_arg(vl, symbolType *);
			break;
		case kwON:
			if ((i = malloc(sizeof(onType))) == NULL) {
				utilError(memErr);
				goto err;
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
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecutePOP;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwPRINT:
			if ((i = malloc(sizeof(printType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecutePRINT;
			i->formatFunc = progFormatPRINT;
			i->freeFunc = progFreePRINT;
			((printType *)i)->expressionList = va_arg(vl, symbolType **);
			((printType *)i)->numExpressions = va_arg(vl, unsigned long int);
			break;
		case kwREAD:
			if ((i = malloc(sizeof(readType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteREAD;
			i->formatFunc = progFormatREAD;
			i->freeFunc = progFreeREAD;
			((readType *)i)->varList = va_arg(vl, symbolType **);
			((readType *)i)->numVars = va_arg(vl, unsigned long int);
			break;
		case kwREM:
			if ((i = malloc(sizeof(remType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteREM;
			i->formatFunc = progFormatREM;
			i->freeFunc = progFreeREM;
			((remType *)i)->remark = va_arg(vl, char *);
			break;
		case kwRESTORE:
			if ((i = malloc(sizeof(restoreType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteRESTORE;
			i->formatFunc = progFormatRESTORE;
			i->freeFunc = progFreeRESTORE;
			((restoreType *)i)->targetLine = va_arg(vl, symbolType *);
			break;
		case kwRETURN:
			if ((i = malloc(sizeof(returnType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteRETURN;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwRUN:
			if ((i = malloc(sizeof(runType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteRUN;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwSAVE:
			if ((i = malloc(sizeof(saveType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteSAVE;
			i->formatFunc = progFormatSAVE;
			i->freeFunc = progFreeSAVE;
			((saveType *)i)->fileName = va_arg(vl, symbolType *);
			break;
		case kwSTOP:
			if ((i = malloc(sizeof(stopType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteSTOP;
			i->formatFunc = progFormatDefault;
			i->freeFunc = progFreeDefault;
			break;
		case kwTRAP:
			if ((i = malloc(sizeof(trapType))) == NULL) {
				utilError(memErr);
				goto err;
			}
			i->executeFunc = progExecuteTRAP;
			i->formatFunc = progFormatTRAP;
			i->freeFunc = progFreeTRAP;
			((trapType *)i)->targetLine = va_arg(vl, symbolType *);
			break;
	}
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


static void progCleanupNumeric(char *s) {
	char *p = NULL;
	double d1 = strtod(s, &p);
	double d2 = trunc(d1);
	long long int i = d2;
	if (*p != 0 || p == s) {
		return;
	}
	if (d1 - d2 == 0) {
		sprintf(s, "%i", i);
	}
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
	progLineType *np;
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
	keywords keyword;
	while (progCurrent && progCurrent->currentInstruction) {
		keyword = progCurrent->currentInstruction->keyword;
		progCurrent->currentInstruction->executeFunc(progCurrent->currentInstruction);
		if (keyword == kwRUN || keyword == kwBYE || keyword == kwSTOP || keyword == kwEND) {
			return;
		}
		if (keyword != kwGOTO && keyword != kwGOSUB && keyword != kwIF && keyword != kwRETURN && keyword != kwNEXT) {
			if (progCurrent) {
				if (progCurrent->currentInstruction && progCurrent->currentInstruction->next) {
					progCurrent->currentInstruction = progCurrent->currentInstruction->next;
				} else {
					progCurrent = progCurrent->next;
					if (progCurrent) {
						progCurrent->currentInstruction = progCurrent->firstInstruction;
					}
				}
			}
		}
	}
}


static void progExecuteAssignment(void *vp) {
	assignmentType *ap = (assignmentType *)vp;
	char *expression = NULL;
	char *s1 = NULL;
	char *s2 = NULL;
	long dim1 = 1;
	long dim2 = 1;
	if ((expression = eval(ap->assignment->r)) == NULL) {
		goto err;
	}
	if (ap->assignment->l->l != NULL) {
		s1 = eval(ap->assignment->l->l);
		dim1 = strtol(s1, NULL, 10);
	}
	if (ap->assignment->l->r != NULL) {
		s2 = eval(ap->assignment->l->r);
		dim2 = strtol(s2, NULL, 10);
	}
	if (varSetValue(ap->assignment->l->value, expression, dim1, dim2)) {
		goto err;
	}
err:
	if (s1 != NULL) {
		free(s1);
	}
	if (s2 != NULL) {
		free(s2);
	}
	if (expression != NULL) {
		free(expression);
	}
}


static void progExecuteBYE(void *vp) {
}


static void progExecuteCLR(void *vp) {
	varClearAll();
}


static void progExecuteCLS(void *vp) {
	system("clear");
}


static void progExecuteCONT(void *vp) {
	if (progStop != NULL) {
		progCurrent = progStop;
		progCurrent->currentInstruction = progCurrent->firstInstruction;
		progStop = NULL;
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
	char *s1;
	char *s2;
	long d1;
	long d2;
	for (i = 0UL; i < dt->numElements; i++) {
		d1 = 1L;
		d2 = 1L;
		if (dt->dimList[i]->l != NULL) {
			if ((s1 = eval(dt->dimList[i]->l)) == NULL) {
				utilError("couldn't dimension variable [%s]", dt->dimList[i]->value);
				return;
			}
			d1 = strtol(s1, NULL, 10);
		}
		if (dt->dimList[i]->r != NULL) {
			if ((s2 = eval(dt->dimList[i]->r)) == NULL) {
				utilError("couldn't dimension variable [%s]", dt->dimList[i]->value);
				return;
			}
			d2 = strtol(s2, NULL, 10);
		}
		if (varDim(dt->dimList[i]->value, d1, d2)) {
			utilError("couldn't dimension variable: %s(%i, %i)", dt->dimList[i]->value, d1, d2);
			return;
		}
	}
}


static void progExecuteEND(void *vp) {
}


static void progExecuteFOR(void *vp) {
	forType *fp = (forType *)vp;
	char *expression = NULL;
	long dim1 = 1;
	long dim2 = 1;
	char *s;
	if ((expression = eval(fp->startPoint->r)) == NULL) {
		goto err;
	}
	if (fp->startPoint->l->l != NULL) {
		s = eval(fp->startPoint->l->l);
		dim1 = strtol(s, NULL, 10);
		free(s);
	}
	if (fp->startPoint->l->r != NULL) {
		s = eval(fp->startPoint->l->r);
		dim2 = strtol(s, NULL, 10);
		free(s);
	}
	varSetValue(fp->startPoint->l->value, expression, dim1, dim2);
	stackPush(forLineStack, progCurrent);
	stackPush(forInstructionStack, progCurrent->currentInstruction);
err:
	if (expression != NULL) {
		free(expression);
	}
}


static void progExecuteGOSUB(void *vp) {
	gosubType *gp = (gosubType *)vp;
	if (gp->targetLine == NULL) {
		utilError("expected expression");
		goto err;
	}
	char *s = eval(gp->targetLine);
	if (s == NULL) {
		utilError("couldn't evaluate expression");
		goto err;
	}
	unsigned long int l = strtol(s, NULL, 10);
	free(s);
	progLineType *p = prog;
	while (p && p->lineNum != l) {
		p = p->next;
	}
	if (p && p->lineNum == l) {
		stackPush(callStack, progCurrent->next);
		progCurrent = p;
		progCurrent->currentInstruction = progCurrent->firstInstruction;
		return;
	}
	utilError("line %l not found", l);
err:
	return;
}


static void progExecuteGOTO(void *vp) {
	gotoType *gp = (gotoType *)vp;
	if (gp->targetLine == NULL) {
		utilError("expected expression");
		goto err;
	}
	char *s = eval(gp->targetLine);
	if (s == NULL) {
		utilError("couldn't evaluate expression");
		goto err;
	}
	unsigned long int l = strtol(s, NULL, 10);
	free(s);
	progLineType *p = prog;
	while (p && p->lineNum != l) {
		p = p->next;
	}
	if (p && p->lineNum == l) {
		progCurrent = p;
		progCurrent->currentInstruction = progCurrent->firstInstruction;
		return;
	}
	utilError("line %l not found", l);
err:
	return;
}


static void progExecuteIF(void *vp) {
	ifType *ip = (ifType *)vp;
	char *e = NULL;
	progLineType *p = prog;
	long int i;
	if ((e = eval(ip->expression)) == NULL) {
		goto err;
	}
	i = strtol(e, NULL, 10);
	free(e);
	e = NULL;
	if (i == 1) {
		if (ip->isGoto) {
			if ((e = eval(ip->gotoOrInstructions)) == NULL) {
				goto err;
			}
			i = strtol(e, NULL, 10);
			free(e);
			e = NULL;
			while (p && p->lineNum != i) {
				p = p->next;
			}
			if (p && p->lineNum == i) {
				progCurrent = p;
				progCurrent->currentInstruction = progCurrent->firstInstruction;
				return;
			} else {
				utilError("goto target line %i not found", i);
				goto err;
			}
		} else {
			progCurrent->currentInstruction = ip->gotoOrInstructions;
			return;
		}
	}
	progCurrent = progCurrent->next;
	if (progCurrent) {
		progCurrent->currentInstruction = progCurrent->firstInstruction;
	}
err:
	if (e != NULL) {
		free(e);
	}
	return;
}


static void progExecuteINPUT(void *vp) {
	inputType *ip = (inputType *)vp;
	char *l;
	char *s = NULL;
	char *t;
	long int len, dim1, dim2;
	unsigned long int i;
	for (i = 0L; i < ip->numVars; i++) {
		l = NULL;
		len = 0L;
		getline(&l, &len, stdin);
		dim1 = 1;
		dim2 = 1;
		if (ip->varList[i]->l != NULL) {
			s = eval(ip->varList[i]->l);
			dim1 = strtol(s, NULL, 10);
			free(s);
			s = NULL;
		}
		if (ip->varList[i]->r != NULL) {
			s = eval(ip->varList[i]->r);
			dim2 = strtol(s, NULL, 10);
			free(s);
			s = NULL;
		}
		if ((t = strrchr(l, '\n')) != NULL) {
			*t = 0;
		}
		if (varSetValue(ip->varList[i]->value, l, dim1, dim2)) {
			goto err;
		}
		free(l);
		l = NULL;
	}
err:
	if (s != NULL) {
		free(s);
	}
	if (l != NULL) {
		free(l);
	}
	return;
}


static void progExecuteLET(void *vp) {
	letType *lp = (letType *)vp;
	char *expression = NULL;
	char *s1 = NULL;
	char *s2 = NULL;
	long dim1 = 1;
	long dim2 = 1;
	if ((expression = eval(lp->assignment->r)) == NULL) {
		goto err;
	}
	if (lp->assignment->l->l != NULL) {
		s1 = eval(lp->assignment->l->l);
		dim1 = strtol(s1, NULL, 10);
	}
	if (lp->assignment->l->r != NULL) {
		s2 = eval(lp->assignment->l->r);
		dim2 = strtol(s1, NULL, 10);
	}
	if (varSetValue(lp->assignment->l->value, expression, dim1, dim2)) {
		goto err;
	}
err:
	if (s1 != NULL) {
		free(s1);
	}
	if (s2 != NULL) {
		free(s2);
	}
	if (expression != NULL) {
		free(expression);
	}
}


void progExecuteLine(progLineType *p) {
	progCurrent = p;
	if (progCurrent) {
		progCurrent->currentInstruction = progCurrent->firstInstruction;
	}
	progExecute();
}


static void progExecuteLIST(void *vp) {
	listType *lp = (listType *)vp;
	char *s1 = eval(lp->startLine);
	char *s2 = eval(lp->endLine);
	int l1 = -1;
	int l2 = -1;
	if (s1 != NULL) {
		l1 = strtol(s1, NULL, 10);
		free(s1);
	}
	if (s2 != NULL) {
		l2 = strtol(s2, NULL, 10);
		free(s2);
	}
	progList(1, l1, l2);
}


static void progExecuteLOAD(void *vp) {
	loadType *lp = (loadType *)vp;
	char *fn;
	if ((fn = eval(lp->fileName)) != NULL) {
		if (ioOpenInput(fn)) {
			utilError("couldn't load file [%s]", fn);
		}
	} else {
		utilError("filename required");
		return;
	}
	free(fn);
}


static void progExecuteNEW(void *vp) {
	progNew();
}


static void progExecuteNEXT(void *vp) {
	nextType *np = (nextType *)vp;
	instructionType *ip = stackPeek(forInstructionStack);
	forType *fp = (forType *)ip;
	char *s;
	double i, j, k;
	long dim1 = 1;
	long dim2 = 1;
	char d[128];
	if (fp->startPoint->l->l != NULL) {
		s = eval(fp->startPoint->l->l);
		dim1 = strtol(s, NULL, 10);
		free(s);
	}
	if (fp->startPoint->l->r != NULL) {
		s = eval(fp->startPoint->l->r);
		dim2 = strtol(s, NULL, 10);
		free(s);
	}
	s = varGetValue(fp->startPoint->l->value, dim1, dim2);
	i = strtod(s, NULL);
	free(s);
	s = eval(fp->endPoint);
	j = strtod(s, NULL);
	free(s);
	if (fp->step != NULL) {
		s = eval(fp->step);
		k = strtod(s, NULL);
		free(s);
	} else {
		k = 1;
	}
	i = i + k;
	sprintf(d, "%lf", i);
	varSetValue(fp->startPoint->l->value, d, dim1, dim2);
	if (i > j) {
		if (progCurrent->currentInstruction->next) {
			progCurrent->currentInstruction = progCurrent->currentInstruction->next;
		} else {
			progCurrent->currentInstruction = progCurrent->firstInstruction;
			progCurrent = progCurrent->next;
			if (progCurrent != NULL) {
				progCurrent->currentInstruction = progCurrent->firstInstruction;
			}
		}
		stackPop(forLineStack);
		stackPop(forInstructionStack);
		return;
	} else {
		progCurrent = stackPeek(forLineStack);
		progCurrent->currentInstruction = stackPeek(forInstructionStack);
		if (progCurrent->currentInstruction->next != NULL) {
			progCurrent->currentInstruction = progCurrent->currentInstruction->next;
		} else {
			progCurrent = progCurrent->next;
			if (progCurrent != NULL) {
				progCurrent->currentInstruction = progCurrent->firstInstruction;
			}
		}
	}
err:
	return;
}


static void progExecuteON(void *vp) {
	onType *op = (onType *)vp;
	char *expression = eval(op->expression);
	progLineType plt;
	long int exp;
	if (expression == NULL) {
		goto err;
	}
	exp = strtol(expression, NULL, 10);
	if (exp < 0 || exp > 255) {
		goto err;
	}
	if (exp > 0) {
		if (exp <= op->numTargets) {
			plt.next = NULL;
			plt.firstInstruction = plt.lastInstruction = plt.currentInstruction = NULL;
			plt.lineNum = -1;
			if (progAppendInstruction(&plt, op->instruction->id, op->targetList[exp - 1])) {
				goto err;
			}
			plt.firstInstruction->executeFunc(plt.firstInstruction);
		}
	}
	if (expression != NULL) {
		free(expression);
	}
	return;
err:
	if (expression != NULL) {
		free(expression);
	}
	return;
}


static void progExecutePOP(void *vp) {
	stackPop(callStack);
}


static void progExecutePRINT(void *vp) {
	printType *pp = (printType *)vp;
	char *expression = NULL;
	unsigned long int i;
	char *s = malloc(maxStringLen + 1);
	char *t = s;
	if (s == NULL) {
		goto err;
	}
	s[0] = 0;
	for (i = 0; i < pp->numExpressions; i++) {
		if (pp->expressionList[i]->id == kwComma) {
			if (t > s) {
				char *u = t - 1;
				if (*u == '\n') {
					t--;
				}
			}
			*t++ = '\t';
		} else if (pp->expressionList[i]->id == kwSemicolon) {
			if (t > s) {
				char *u = t - 1;
				if (*u == '\n') {
					t--;
				}
			}
		}
		else {
			if ((expression = eval(pp->expressionList[i])) == NULL) {
				goto err;
			}
			strcpy(t, expression);
			t += strlen(expression);
			free(expression);
			expression = NULL;
			*t++ = '\n';
		}
	}
	if (pp->numExpressions == 0) {
		*t++ = '\n';
	}
	*t = 0;
	printf(s);
	free(s);
err:
	if (expression != NULL) {
		free(expression);
	}
}


static void progExecuteREAD(void *vp) {
	readType *rp = (readType *)vp;
	char *s;
	unsigned long int i;
	long int dim1, dim2;
	for (i = 0L; i < rp->numVars; i++) {
		dim1 = 1;
		dim2 = 1;
		if (rp->varList[i]->l != NULL) {
			s = eval(rp->varList[i]->l);
			dim1 = strtol(s, NULL, 10);
			free(s);
			s = NULL;
		}
		if (rp->varList[i]->r != NULL) {
			s = eval(rp->varList[i]->r);
			dim2 = strtol(s, NULL, 10);
			free(s);
			s = NULL;
		}
		if ((s = varReadData()) == NULL) {
			goto err;
		}
		if (varSetValue(rp->varList[i]->value, s, dim1, dim2)) {
			goto err;
		}
		free(s);
		s = NULL;
	}
err:
	if (s != NULL) {
		free(s);
	}
	return;
}


static void progExecuteREM(void *vp) {
}


static void progExecuteRESTORE(void *vp) {
	restoreType *rp = (restoreType *)vp;
	long int line;
	char *s = NULL;
	if (rp->targetLine != NULL) {
		if ((s = eval(rp->targetLine)) != 0) {
			goto err;
		}
		line = strtol(s, NULL, 10);
	} else {
		line = -1;
	}
	varRestoreData(line);
err:
	if (s != NULL) {
		free(s);
	}
	return;
}


static void progExecuteRETURN(void *vp) {
	progLineType *p;
	p = stackPeek(callStack);
	stackPop(callStack);
	if (p == NULL) {
		return;
	}
	progCurrent = p;
	if (progCurrent) {
		progCurrent->currentInstruction = progCurrent->firstInstruction;
	}
}


static void progExecuteRUN(void *vp) {
	varClearAll();
	stackClear(forLineStack);
	stackClear(forInstructionStack);
	stackClear(callStack);
	progStop = NULL;
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
		utilError("file name required");
		goto err;
	}
	if ((fh = open(fn, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		utilError("unable to create output file [%s]", fn);
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
	return;
}


static void progExecuteSTOP(void *vp) {
	progStop = progCurrent->next;
}


static void progExecuteTRAP(void *vp) {
	trapType *tp = (trapType *)vp;
	if (tp->targetLine == NULL) {
		utilError("expected expression");
		goto err;
	}
	char *s = eval(tp->targetLine);
	if (s == NULL) {
		utilError("couldn't evaluate expression");
		goto err;
	}
	unsigned long int l = strtol(s, NULL, 10);
	free(s);
	progLineType *p = prog;
	while (p && p->lineNum != l) {
		p = p->next;
	}
	if (p && p->lineNum == l) {
		progTrap = p;
		return;
	}
	utilError("line %l not found", l);
err:
	return;
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
	char *s = malloc(strlen(l) + 6);
	if (s == NULL) {
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
	char *l = NULL;
	char *r = NULL;
	char *s = NULL;
	char *ss = NULL;
	unsigned long int sLen = 4;
	unsigned long int len = 1024;
	unsigned long int i;
	long int d1;
	long int d2;
	if ((s = malloc(len)) == NULL) {
		utilError(memErr);
		goto err;
	}
	strcpy(s, scanGetKeyword(kwDIM));
	strcat(s, " ");
	for (i = 0; i < dt->numElements; i++) {
		sLen += strlen(dt->dimList[i]->value) + 4;
		l = eval(dt->dimList[i]->l);
		r = eval(dt->dimList[i]->r);
		if (l != NULL) {
			sLen += strlen(l);
		}
		if (r != NULL) {
			sLen += strlen(r) + 2;
		}
		if (sLen > len) {
			len <<= 1;
			if ((ss = realloc(s, len)) == NULL) {
				utilError(memErr);
				goto err;
			}
			s = ss;
		}
		strcat(s, dt->dimList[i]->value);
		strcat(s, "(");
		if (l != NULL) {
			strcat(s, l);
			free(l);
			l = NULL;
		}
		if (r != NULL) {
			strcat(s, ",");
			strcat(s, r);
			free(r);
			r = NULL;
		}
		strcat(s, ")");
		if (i < dt->numElements - 1) {
			strcat(s, ",");
		}
	}
	return(s);
err:
	if (l != NULL) {
		free(l);
	}
	if (r != NULL) {
		free(r);
	}
	if (s != NULL) {
		free(s);
	}
	return(NULL);
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
		utilError(memErr);
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
	return(s);
err:
	if (s != NULL) {
		free(s);
	}
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
			utilError(memErr);
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
			utilError(memErr);
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
		utilError(memErr);
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
		utilError(memErr);
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
		utilError(memErr);
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
		utilError(memErr);
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
		utilError(memErr);
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
		utilError(memErr);
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
		utilError(memErr);
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
		utilError(memErr);
		goto err;
	}
	strcpy(s, scanGetKeyword(kwPRINT));
	strcat(s, " ");
	for (i = 0UL; i < pp->numExpressions; i++) {
		if (pp->expressionList[i]->id == kwComma) {
			strcat(s, ", ");
		} else if (pp->expressionList[i]->id == kwSemicolon) {
			strcat(s, "; ");
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
		utilError(memErr);
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
	if (rt->targetLine != NULL) {
		if ((t = evalCode(rt->targetLine)) == NULL) {
			goto err;
		}
		if ((s = malloc(strlen(scanGetKeyword(kwRESTORE)) + strlen(t) + 2)) == NULL) {
			utilError(memErr);
			goto err;
		}
		strcpy(s, scanGetKeyword(kwRESTORE));
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


static char *progFormatSAVE(void *vp) {
	saveType *st = (saveType *)vp;
	char *s = NULL;
	char *fn = NULL;
	if ((fn = evalCode(st->fileName)) == NULL) {
		goto err;
	}
	if ((s = malloc(strlen(fn) + strlen(scanGetKeyword(kwSAVE)) + 2)) == NULL) {
		utilError(memErr);
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
			utilError(memErr);
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
				if (dp->dimList[i] != NULL) {
					free(dp->dimList[i]);
				}
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
				if (rp->varList[i] != NULL) {
					free(rp->varList[i]);
				}
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
	stackInit(callStack);
	stackInit(forLineStack);
	stackInit(forInstructionStack);
	srand(time(NULL));
	return(0);
err:
	return(1);
}


progLineType *progInsertLine(int l) {
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
	} else if (prog->lineNum == l) {
		p->next = prog->next;
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
	if ((s = malloc(len)) == NULL) {
		utilError(memErr);
		goto err;
	}
	while (p && ((p->lineNum >= start || start == -1) && (p->lineNum <= end || end == -1))) {
		sprintf(s, "%li ", p->lineNum);
		sLen = strlen(s);
		i = p->firstInstruction;
		while (i) {
			if ((e = i->formatFunc(i)) != NULL) {
				eLen = strlen(e);
				if (sLen + eLen + 5 > len) {
					len <<= 1;
					if ((ss = realloc(s, len)) == NULL) {
						utilError(memErr);
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
err:
	if (e != NULL) {
		free(e);
	}
	if (s != NULL) {
		free(s);
	}
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
}


static int isNumeric(char *s) {
	char haveSign = 0;
	char haveDecimal = 0;
	/* skip whitespace */
	while (isspace(*s)) {
		s++;
	}
	if (*s == 0) {
		return(0);
	}
	/* can have an optional + or - now */
	if (*s == '+' || *s == '-') {
		haveSign = 1;
		s++;
	}
	/* skip whitespace */
	while (isspace(*s)) {
		s++;
	}
	if (*s == 0) {
		return(0);
	}
	/* now, we can have digits and one . */
	while (isdigit(*s) || (*s == '.')) {
		if (*s == '.') {
			if (haveDecimal) {
				return(0);
			}
			haveDecimal = 1;
		}
		s++;
	}
	if (*s == 0) {
		return(1);
	}
	/* skip whitespace */
	while (isspace(*s)) {
		s++;
	}
	if (*s == 0) {
		return(1);
	}
	/* a non-whitespace, non string-terminator character was found */
	return(0);
}


static char *eval(symbolType *expr) {
	if (expr == NULL) {
		return(NULL);
	}
	char *r = malloc(maxStringLen + 1);
	char *left = NULL;
	char *right = NULL;
	char *s = NULL;
	double o1 = 0;
	double o2 = 0;
	long d1 = 1;
	long d2 = 1;
	int e1, e2;
	if (r == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	switch (expr->id) {
		case kwString:
		case kwNumeric:
			strcpy(r, expr->value);
			break;
		case kwIdentifier:
			if (expr->l != NULL) {
				if ((left = eval(expr->l)) != NULL) {
					d1 = strtol(left, NULL, 10);
					free(left);
					left = NULL;
				}
			}
			if (expr->r != NULL) {
				if ((right = eval(expr->r)) != NULL) {
					d2 = strtol(right, NULL, 10);
					free(right);
					right = NULL;
				}
			}
			if ((right = varGetValue(expr->value, d1, d2)) != NULL) {
				strcpy(r, right);
			}
			break;
		case kwOpAdd:
		case kwOpSub:
		case kwOpMul:
		case kwOpDiv:
		case kwOpExp:
		case kwLogicalLT:
		case kwLogicalLTE:
		case kwLogicalEQ:
		case kwLogicalNE:
		case kwLogicalGT:
		case kwLogicalGTE:
			left = eval(expr->l);
			right = eval(expr->r);
			if (left != NULL) {
				o1 = strtod(left, NULL);
			}
			if (right != NULL) {
				o2 = strtod(right, NULL);
			}
			e1 = isNumeric(left);
			e2 = isNumeric(right);
			switch (expr->id) {
				case kwOpAdd:
					sprintf(r, "%f", o1 + o2);
					break;
				case kwOpSub:
					sprintf(r, "%f", o1 - o2);
					break;
				case kwOpMul:
					sprintf(r, "%f", o1 * o2);
					break;
				case kwOpDiv:
					sprintf(r, "%f", o1 / o2);
					break;
				case kwOpExp:
					sprintf(r, "%f", pow(o1, o2));
					break;
				case kwLogicalLT:
					if (e1 + e2 != 0) {
						if (o1 < o2) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					} else {
						if (strcmp(left, right) < 0) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					}
					r[1] = 0;
					break;
				case kwLogicalLTE:
					if (e1 + e2 != 0) {
						if (o1 <= o2) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					} else {
						if (strcmp(left, right) <= 0) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					}
					r[1] = 0;
					break;
				case kwLogicalEQ:
					if (e1 + e2 != 0) {
						if (o1 == o2) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					} else {
						if (strcmp(left, right) == 0) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					}
					r[1] = 0;
					break;
				case kwLogicalNE:
					if (e1 + e2 != 0) {
						if (o1 != o2) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					} else {
						if (strcmp(left, right) != 0) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					}
					r[1] = 0;
					break;
				case kwLogicalGT:
					if (e1 + e2 != 0) {
						if (o1 > o2) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					} else {
						if (strcmp(left, right) > 0) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					}
					r[1] = 0;
					break;
				case kwLogicalGTE:
					if (e1 + e2 != 0) {
						if (o1 >= o2) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					} else {
						if (strcmp(left, right) >= 0) {
							r[0] = '1';
						} else {
							r[0] = '0';
						}
					}
					r[1] = 0;
					break;
			}
			break;
		case kwAssignment:
			right = eval(expr->r);
			if (expr->l->l != NULL) {
				if ((s = eval(expr->l->l)) == NULL) {
					goto err;
				}
				d1 = strtol(s, NULL, 10);
				free(s);
				if ((s = eval(expr->l->r)) == NULL) {
					goto err;
				}
				d2 = strtol(s, NULL, 10);
				free(s);
			}
			if (varSetValue(expr->l->value, right, d1, d2)) {
				goto err;
			}
			break;
		case kwSignPlus:
			right = eval(expr->r);
			o2 = strtod(right, NULL);
			sprintf(r, "%f", o2);
			break;
		case kwSignMinus:
			right = eval(expr->r);
			o2 = strtod(right, NULL);
			o2 = 0 - o2;
			sprintf(r, "%f", o2);
			break;
		case kwSubExpression:
			right = eval(expr->r);
			strcpy(r, right);
			break;
		case kwABS:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = abs(o1);
			sprintf(r, "%f", o1);
			break;
		case kwASC:
			left = eval(expr->l);
			o1 = left[0];
			sprintf(r, "%f", o1);
			break;
		case kwATN:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = atan(o1);
			sprintf(r, "%f", o1);
			break;
		case kwCLOG:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = log10(o1);
			sprintf(r, "%f", o1);
			break;
		case kwCOS:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = cos(o1);
			sprintf(r, "%f", o1);
			break;
		case kwEXP:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = exp(o1);
			sprintf(r, "%f", o1);
			break;
		case kwINT:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = trunc(o1);
			sprintf(r, "%f", o1);
			break;
		case kwLEN:
			left = eval(expr->l);
			sprintf(r, "%i", strlen(left));
			break;
		case kwLOG:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = log(o1);
			sprintf(r, "%f", o1);
			break;
		case kwRND:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = (double)rand() / (double)RAND_MAX;
			sprintf(r, "%f", o1);
			break;
		case kwSGN:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			if (o1 > 0) {
				r[0] = '1';
				r[1] = 0;
			} else if (o1 == 0) {
				r[0] = '0';
				r[1] = 0;
			} else if (o1 < 0) {
				r[0] = '-';
				r[1] = '1';
				r[2] = 0;
			}
			break;
		case kwSIN:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = sin(o1);
			sprintf(r, "%f", o1);
			break;
		case kwSQR:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = sqrt(o1);
			sprintf(r, "%f", o1);
			break;
		case kwVAL:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			sprintf(r, "%f", o1);
			break;
		case kwCHR:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			o1 = trunc(o1);
			r[0] = (char)o1;
			r[1] = 0;
			break;
		case kwSTR:
			left = eval(expr->l);
			o1 = strtod(left, NULL);
			sprintf(r, "%f", o1);
			break;
		case kwOR:
			left = eval(expr->l);
			right = eval(expr->r);
			o1 = strtod(left, NULL);
			o2 = strtod(right, NULL);
			if (o1 || o2) {
				r[0] = '1';
			} else {
				r[0] = '0';
			}
			r[1] = 0;
			break;
		case kwAND:
			left = eval(expr->l);
			right = eval(expr->r);
			o1 = strtod(left, NULL);
			o2 = strtod(right, NULL);
			if (o1 && o2) {
				r[0] = '1';
			} else {
				r[0] = '0';
			}
			r[1] = 0;
			break;
		case kwNOT:
			left = eval(expr->r);
			o1 = strtod(left, NULL);
			if (o1) {
				r[0] = '0';
			} else {
				r[0] = '1';
			}
			r[1] = 0;
			break;
	}
	if (left != NULL) {
		free(left);
	}
	if (right != NULL) {
		free(right);
	}
	if (isNumeric(r)) {
		progCleanupNumeric(r);
	}
	return(r);
err:
	if (r != NULL) {
		free(r);
	}
	if (left != NULL) {
		free(left);
	}
	if (right != NULL) {
		free(right);
	}
	return(NULL);
}


static char *evalCode(symbolType *exp) {
	char *s = malloc(maxStringLen + 1);
	char *t = NULL;
	char *r = NULL;
	char *l = NULL;
	char ch;
	if (exp == NULL) {
		return(strdup(""));
	}
	if (s == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	switch (exp->id) {
		case kwAssignment:
			l = evalCode(exp->l);
			r = evalCode(exp->r);
			sprintf(s, "%s = %s", l, r);
			break;
		case kwOpAdd:
		case kwOpSub:
		case kwOpMul:
		case kwOpDiv:
			l = evalCode(exp->l);
			r = evalCode(exp->r);
			switch (exp->id) {
				case kwOpAdd:
					ch = '+';
					break;
				case kwOpSub:
					ch = '-';
					break;
				case kwOpMul:
					ch = '*';
					break;
				case kwOpDiv:
					ch = '/';
					break;
			}
			sprintf(s, "%s %c %s", l, ch, r);
			break;
		case kwLogicalLT:
		case kwLogicalLTE:
		case kwLogicalEQ:
		case kwLogicalNE:
		case kwLogicalGT:
		case kwLogicalGTE:
			l = evalCode(exp->l);
			r = evalCode(exp->r);
			switch (exp->id) {
				case kwLogicalLT:
					t = "<";
					break;
				case kwLogicalLTE:
					t = "<=";
					break;
				case kwLogicalEQ:
					t = "=";
					break;
				case kwLogicalNE:
					t = "<>";
					break;
				case kwLogicalGT:
					t = ">";
					break;
				case kwLogicalGTE:
					t = ">=";
					break;
			}
			sprintf(s, "%s %s %s", l, t, r);
			break;
		case kwSignPlus:
		case kwSignMinus:
			if (exp->id == kwSignPlus) {
				strcpy(s, "+");
			} else {
				strcpy(s, "-");
			}
			r = evalCode(exp->r);
			strcat(s, r);
			break;
		case kwIdentifier:
		case kwNumeric:
			strcpy(s, exp->value);
			if (exp->id == kwIdentifier && exp->l != NULL) {
				strcat(s, "(");
				if (exp->l != NULL) {
					l = evalCode(exp->l);
					strcat(s, l);
					if (exp->r != NULL) {
						strcat(s, ",");
						r = evalCode(exp->r);
						strcat(s, r);
					}
				}
				strcat(s, ")");
			}
			break;
		case kwString:
			sprintf(s, "\"%s\"", exp->value);
			break;
		case kwSubExpression:
			l = evalCode(exp->r);
			sprintf(s, "(%s)", l);
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
			l = evalCode(exp->l);
			sprintf(s, "%s(%s)", scanGetKeyword(exp->id), l);
			break;
		case kwAND:
			l = evalCode(exp->l);
			r = evalCode(exp->r);
			sprintf(s, "%s and %s", l, r);
			break;
		case kwOR:
			l = evalCode(exp->l);
			r = evalCode(exp->r);
			sprintf(s, "%s or %s", l, r);
			break;
		case kwNOT:
			r = evalCode(exp->r);
			sprintf(s, "not %s", r);
			break;
	}
	if (l != NULL) {
		free(l);
	}
	if (r != NULL) {
		free(r);
	}
	return(s);
err:
	if (s != NULL) {
		free(s);
	}
	if (r != NULL) {
		free(r);
	}
	if (l != NULL) {
		free(l);
	}
	return(NULL);
}


static char *formatLine(instructionType *i) {
	char *e = NULL;
	char *s = NULL;
	char *ss = NULL;
	unsigned long int len = 1024;
	unsigned long int eLen = 0;
	unsigned long int sLen = 0;
	if ((s = malloc(len)) == NULL) {
		utilError(memErr);
		goto err;
	}
	s[0] = 0;
	while (i) {
		if ((e = i->formatFunc(i)) != NULL) {
			eLen = strlen(e);
			if (sLen + eLen + 5 > len) {
				len <<= 1;
				if ((ss = realloc(s, len)) == NULL) {
					utilError(memErr);
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
