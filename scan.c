/*
 * scan.c
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "scan.h"
#include "util.h"


/*
 * LOCAL DATA
 */

static symbolType *keywordTree = NULL;
static symbolType keywordTable[] = {
	{NULL, NULL, "and", kwAND},
	{NULL, NULL, "or", kwOR},
	{NULL, NULL, "not", kwNOT},
	{NULL, NULL, "bye", kwBYE},
	{NULL, NULL, "clr", kwCLR},
	{NULL, NULL, "cls", kwCLS},
	{NULL, NULL, "cont", kwCONT},
	{NULL, NULL, "data", kwDATA},
	{NULL, NULL, "dim", kwDIM},
	{NULL, NULL, "end", kwEND},
	{NULL, NULL, "for", kwFOR},
	{NULL, NULL, "to", kwTO},
	{NULL, NULL, "step", kwSTEP},
	{NULL, NULL, "gosub", kwGOSUB},
	{NULL, NULL, "goto", kwGOTO},
	{NULL, NULL, "if", kwIF},
	{NULL, NULL, "then", kwTHEN},
	{NULL, NULL, "input", kwINPUT},
	{NULL, NULL, "let", kwLET},
	{NULL, NULL, "list", kwLIST},
	{NULL, NULL, "load", kwLOAD},
	{NULL, NULL, "new", kwNEW},
	{NULL, NULL, "next", kwNEXT},
	{NULL, NULL, "on", kwON},
	{NULL, NULL, "pop", kwPOP},
	{NULL, NULL, "print", kwPRINT},
	{NULL, NULL, "read", kwREAD},
	{NULL, NULL, "rem", kwREM},
	{NULL, NULL, "restore", kwRESTORE},
	{NULL, NULL, "return", kwRETURN},
	{NULL, NULL, "run", kwRUN},
	{NULL, NULL, "save", kwSAVE},
	{NULL, NULL, "stop", kwSTOP},
	{NULL, NULL, "trap", kwTRAP},
	{NULL, NULL, "abs", kwABS},
	{NULL, NULL, "asc", kwASC},
	{NULL, NULL, "atn", kwATN},
	{NULL, NULL, "clog", kwCLOG},
	{NULL, NULL, "cos", kwCOS},
	{NULL, NULL, "exp", kwEXP},
	{NULL, NULL, "int", kwINT},
	{NULL, NULL, "len", kwLEN},
	{NULL, NULL, "log", kwLOG},
	{NULL, NULL, "rnd", kwRND},
	{NULL, NULL, "sgn", kwSGN},
	{NULL, NULL, "sin", kwSIN},
	{NULL, NULL, "sqr", kwSQR},
	{NULL, NULL, "val", kwVAL},
	{NULL, NULL, "chr$", kwCHR},
	{NULL, NULL, "str$", kwSTR}
};
static symbolType *scanCurrent = NULL;
static symbolType *idToKeywordTable[kwNumKeywords];


/*
 * LOCAL FUNCTIONS
 */

static int cmpKeywords(const void *a, const void *b);
static char *scanGetName(void);
static char *scanGetNum(void);
static char *scanGetOp(void);
static char *scanGetString(void);
static symbolType *insertSymbol(symbolType *p, long s, long e);
static int isop(char c);
static int lookup(symbolType *t, char *s);
static void scanExit(void);
static void skipWhite(void);


static int cmpKeywords(const void *a, const void *b) {
	return(strcmp(((symbolType *)a)->value, ((symbolType *)b)->value));
}


static symbolType *insertSymbol(symbolType *p, long s, long e) {
	if (s > e) {
		return(NULL);
	}
	long m = (s + e) / 2;
	symbolType *root = &p[m];
	root->l = insertSymbol(p, s, m - 1);
	root->r = insertSymbol(p, m + 1, e);
	return(root);
}


static int isop(char c) {
	return(c == '+' || c == '-' || c == '*' || c == '/' || c == '^');
}


static int islop(char c) {
	return(c == '<' || c == '>' || c == '=');
}


static int lookup(symbolType *t, char *s) {
	if (s == NULL) {
		return(-1);
	}
	int i;
	while (t != NULL) {
		i = strcasecmp(t->value, s);
		if (i == 0) {
			break;
		} else if (i < 0) {
			t = t->r;
		} else {
			t = t->l;
		}
	}
	if (t != NULL) {
		return(t->id);
	}
	return(-1);
}


int scanConsumeEol(void) {
	ioNext();
	return(scanNext());
}


char *scanGetKeyword(keywords k) {
	if (k >= kwNumKeywords) {
		return(NULL);
	}
	return(idToKeywordTable[k]->value);
}


static char *scanGetLOp(void) {
	char *s = malloc(3);
	char *t = s;
	switch (ioPeek()) {
		case '<':
			*t++ = '<';
			ioNext();
			if (ioPeek() == '=' || ioPeek() == '>') {
				*t++ = ioPeek();
				ioNext();
			}
			*t = 0;
			skipWhite();
			return(s);
		case '>':
			*t++ = '>';
			ioNext();
			if (ioPeek() == '=') {
				*t++ = ioPeek();
				ioNext();
			}
			*t = 0;
			skipWhite();
			return(s);
		case '=':
			*t++ = '=';
			*t = 0;
			ioNext();
			skipWhite();
			return(s);
	}
	if (s) {
		free(s);
	}
	return(NULL);
}


static char *scanGetName(void) {
	char *s = malloc(maxStringLen + 1);
	char *t = s;
	int i = maxStringLen;
	if (s == NULL) {
		aborts("memory allocation error");
	}
	if (!isalpha(ioPeek())) {
		utilError("expected alphabetic character\n");
		free(s);
		return(NULL);
	}
	while (i > 0 && (isalnum(ioPeek()) || ioPeek() == '$')) {
		*t++ = ioPeek();
		ioNext();
		i--;
	}
	if (i < 0) {
		utilError("string exceeds maximum length of %i\n", maxStringLen);
		free(s);
		return(NULL);
	}
	*t = 0;
	skipWhite();
	return(s);
}


static char *scanGetNum(void) {
	char *s = malloc(maxStringLen + 1);
	char *t = s;
	int i = maxStringLen;
	if (!isdigit(ioPeek())) {
		utilError("expected digit");
		return(NULL);
	}
	while (i >= 0 && (isdigit(ioPeek()) || ioPeek() == '.')) {
		*t++ = ioPeek();
		ioNext();
		i--;
	}
	if (i < 0) {
		utilError("string exceeds maximum length of %i", maxStringLen);
		free(s);
		return(NULL);
	}
	*t = 0;
	skipWhite();
	return(s);
}


static char *scanGetOp(void) {
	char *s = malloc(maxStringLen + 1);
	char *t = s;
	int i = maxStringLen;
	if (s == NULL) {
		aborts("memory allocation error");
	}
	if (!isop(ioPeek())) {
		utilError("expected operator");
		free(s);
		return(NULL);
	}
	while (i >= 0 && isop(ioPeek())) {
		*t++ = ioPeek();
		ioNext();
		i--;
	}
	if (i < 0) {
		utilError("string exceeds maximum length of %i", maxStringLen);
		free(s);
		return(NULL);
	}
	*t = 0;
	skipWhite();
	return(s);
}


static char *scanGetString(void) {
	char *s = malloc(maxStringLen + 1);
	char *t = s;
	int i = maxStringLen;
	if (s == NULL) {
		utilError("memory allocation error");
		return(NULL);
	}
	if (ioPeek() != '"') {
		utilError("expected quote");
		free(s);
		return(NULL);
	}
	ioNext();
	while (i >= 0 && ioPeek() != '"' && ioPeek() != '\n' && ioPeek() != IO_EOF) {
		*t++ = ioPeek();
		ioNext();
		i--;
	}
	if (i < 0) {
		utilError("string exceeds maximum length of %i\n", maxStringLen);
		free(s);
		return(NULL);
	}
	if (ioPeek() == '\n' || ioPeek() == IO_EOF) {
		utilError("unterminated string\n");
		free(s);
		return(NULL);
	}
	*t = 0;
	ioNext();
	skipWhite();
	return(s);
}


symbolType *scanGetText(void) {
	char *s = malloc(maxStringLen + 1);
	char *t = scanCurrent->value;
	int i = maxStringLen;
	while (i >= 0 && ioPeek() != '\n' && ioPeek() != IO_EOF) {
		*t++ = ioPeek();
		ioNext();
		i--;
	}
	if (i < 0) {
		utilError("string exceeds maximum length of %i\n", maxStringLen);
		return(NULL);
	}
	*t = 0;
	scanCurrent->id = kwText;
	return(scanCurrent);
}


static void scanExit(void) {
    if (scanCurrent != NULL) {
        free(scanCurrent);
    }
}


int scanInit(void) {
	long l = sizeof(keywordTable) / sizeof(symbolType);
	long i;
	qsort(keywordTable, l, sizeof(symbolType), cmpKeywords);
	if ((keywordTree = insertSymbol(keywordTable, 0, l - 1)) == NULL) {
		return(1);
	}
	atexit(scanExit);
    if ((scanCurrent = malloc(sizeof(symbolType) + maxStringLen + 1)) == NULL) {
        return(1);
    }
	for (i = 0; i < kwNumKeywords; i++) {
		for (l = 0; l < sizeof(keywordTable) / sizeof(symbolType); l++) {
			if (keywordTable[l].id == i) {
				idToKeywordTable[i] = &keywordTable[l];
			}
		}
	}
    scanCurrent->l = NULL;
    scanCurrent->r = NULL;
    scanCurrent->value = (char *)scanCurrent + sizeof(symbolType);
    scanCurrent->id = kwEof;
	return(0);
}


symbolType *scanNewSymbol(keywords k, char *v) {
	symbolType *s;
	int l = 0;
	if (v != NULL) {
		l = strlen(v) + 1;
	}
	if ((s = malloc(sizeof(symbolType) + l)) == NULL) {
		utilError("couldn't allocate memory");
		return(NULL);
	}
	s->l = s->r = NULL;
	s->id = k;
	s->value = (char *)s + sizeof(symbolType);
	if (l > 0) {
		strcpy(s->value, v);
	} else {
		s->value = NULL;
	}
	return(s);
}


int scanNext(void) {
	char *s = NULL;
	int i;
	skipWhite();
	if (ioPeek() == IO_EOF) {
        scanCurrent->id = kwEof;
        scanCurrent->value[0] = 0;
		return(0);
	} else if (ioPeek() == '\n') {
        scanCurrent->id = kwEol;
        scanCurrent->value[0] = 0;
		skipWhite();
		return(0);
	} else if (ioPeek() == '(') {
		scanCurrent->id = kwParenthesesOpen;
		scanCurrent->value[0] = 0;
		ioNext();
		skipWhite();
		return(0);
	} else if (ioPeek() == ')') {
		scanCurrent->id = kwParenthesesClose;
		scanCurrent->value[0] = 0;
		ioNext();
		skipWhite();
		return(0);
	} else if (ioPeek() == ',') {
		scanCurrent->id = kwComma;
		scanCurrent->value[0] = 0;
		ioNext();
		skipWhite();
		return(0);
	} else if (ioPeek() == ';') {
		scanCurrent->id = kwSemicolon;
		scanCurrent->value[0] = 0;
		ioNext();
		skipWhite();
		return(0);
	} else if (ioPeek() == ':') {
		scanCurrent->id = kwColon;
		scanCurrent->value[0] = 0;
		ioNext();
		skipWhite();
		return(0);
	} else if (isalpha(ioPeek())) {
		if ((s = scanGetName()) != NULL) {
            i = lookup(keywordTree, s);
            if (i == -1) {
                scanCurrent->id = kwIdentifier;
            	strcpy(scanCurrent->value, s);
            } else {
                scanCurrent->id = i;
				scanCurrent->value[0] = 0;
            }
            free(s);
            return(0);
        }
        return(1);
	} else if (isdigit(ioPeek())) {
        if ((s = scanGetNum()) != NULL) {
            scanCurrent->id = kwNumeric;
            strcpy(scanCurrent->value, s);
            free(s);
            return(0);
        }
        return(1);
	} else if (isop(ioPeek())) {
		if ((s = scanGetOp()) != NULL) {
			scanCurrent->value[0] = 0;
			if (!strcmp(s, "+")) {
            	scanCurrent->id = kwOpAdd;
			} else if (!strcmp(s, "-")) {
				scanCurrent->id = kwOpSub;
			} else if (!strcmp(s, "*")) {
				scanCurrent->id = kwOpMul;
			} else if (!strcmp(s, "/")) {
				scanCurrent->id = kwOpDiv;
			} else if (!strcmp(s, "^")) {
				scanCurrent->id = kwOpExp;
			}
            free(s);
            return(0);
        }
		return(1);
	} else if (islop(ioPeek())) {
		if ((s = scanGetLOp()) != NULL) {
			scanCurrent->value[0] = 0;
			if (!strcmp(s, "<")) {
				scanCurrent->id = kwLogicalLT;
			} else if (!strcmp(s, "<=")) {
				scanCurrent->id = kwLogicalLTE;
			} else if (!strcmp(s, "=")) {
				scanCurrent->id = kwLogicalEQ;
			} else if (!strcmp(s, "<>")) {
				scanCurrent->id = kwLogicalNE;
			} else if (!strcmp(s, ">")) {
				scanCurrent->id = kwLogicalGT;
			} else if (!strcmp(s, ">=")) {
				scanCurrent->id = kwLogicalGTE;
			}
			free(s);
			return(0);
		}
	} else if (ioPeek() == '"') {
		if ((s = scanGetString()) != NULL) {
            scanCurrent->id = kwString;
            strcpy(scanCurrent->value, s);
            free(s);
            return(0);
        }
        return(1);
	}
	skipWhite();
	return(1);
}


symbolType *scanPeek(void) {
    return(scanCurrent);
}


static void skipWhite(void) {
	while (isspace(ioPeek()) && ioPeek() != '\n') {
		ioNext();
	}
}
