/*
 * parse.c
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "prog.h"
#include "scan.h"
#include "util.h"
#include "var.h"


/*
 * LOCAL FUNCTIONS
 */

static symbolType *term1(void);
static symbolType *term2(void);
static symbolType *term3(void);
static symbolType *term4(void);
static symbolType *factor1(void);
static symbolType *factor2(void);
static symbolType *factor3(void);
static symbolType *aexp(void);
static symbolType *sexp(void);

static symbolType *parseAssignment(void);
static int insAssignment(progLineType *p);
static int insCLR(progLineType *pl);
static int insCLS(progLineType *pl);
static int insCONT(progLineType *pl);
static int insDATA(progLineType *pl);
static int insDIM(progLineType *pl);
static int insEND(progLineType *pl);
static int insFOR(progLineType *pl);
static int insGOSUB(progLineType *pl);
static int insGOTO(progLineType *pl);
static int insIF(progLineType *pl);
static int insINPUT(progLineType *pl);
static int insLET(progLineType *pl);
static int insLIST(progLineType *pl);
static int insLOAD(progLineType *pl);
static int insNEXT(progLineType *pl);
static int insON(progLineType *pl);
static int insPOP(progLineType *pl);
static int insPRINT(progLineType *pl);
static int insREAD(progLineType *pl);
static int insREM(progLineType *pl);
static int insRESTORE(progLineType *pl);
static int insRETURN(progLineType *pl);
static int insRUN(progLineType *pl);
static int insSAVE(progLineType *pl);
static int insSTOP(progLineType *pl);
static int insTRAP(progLineType *pl);

static int statement(progLineType *pl);
static int compoundStatement(progLineType *pl);
static void deferredStatement(void);
static void immediateStatement(void);


static symbolType *aexp(void) {
	symbolType *a = NULL;
	symbolType *p = NULL;
	symbolType *s = NULL;
	if ((p = term1()) == NULL) {
		goto err;
	}
	s = scanPeek();
	if (s->id == kwOR) {
		if ((a = scanNewSymbol(kwOR, NULL)) == NULL) {
			goto err;
		}
		a->l = p;
		scanNext();
		if ((a->r = aexp()) == NULL) {
			goto err;
		}
		return(a);
	}
	return(p);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *term1(void) {
	symbolType *a = NULL;
	symbolType *p = NULL;
	symbolType *s;
	s = scanPeek();
	if (s->id == kwNOT) {
		if ((p = scanNewSymbol(kwNOT, NULL)) == NULL) {
			goto err;
		}
		scanNext();
		if ((p->r = term2()) == NULL) {
			goto err;
		}
		return(p);
	} else {
		if ((p = term2()) == NULL) {
			goto err;
		}
		s = scanPeek();
		if (s->id == kwAND) {
			if ((a = scanNewSymbol(kwAND, NULL)) == NULL) {
				goto err;
			}
			scanNext();
			a->l = p;
			if ((a->r = term1()) == NULL) {
				goto err;
			}
			return(a);
		}
		return(p);
	}
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *term2(void) {
	symbolType *a = NULL;
	symbolType *p = NULL;
	symbolType *s;
	if ((p = term3()) == NULL) {
		goto err;
	}
	s = scanPeek();
	if (s->id == kwLogicalLT || s->id == kwLogicalLTE || s->id == kwLogicalEQ || s->id == kwLogicalNE || s->id == kwLogicalGT || s->id == kwLogicalGTE) {
		if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
			goto err;
		}
		scanNext();
		a->l = p;
		if ((a->r = term2()) == NULL) {
			goto err;
		}
		return(a);
	}
	return(p);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *term3(void) {
	symbolType *a = NULL;
	symbolType *p = NULL;
	symbolType *s;
	if ((p = term4()) == NULL) {
		goto err;
	}
	s = scanPeek();
	if (s->id == kwOpAdd || s->id == kwOpSub) {
		if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
			goto err;
		}
		a->l = p;
		scanNext();
		if ((a->r = term3()) == NULL) {
			goto err;
		}
		return(a);
	}
	return(p);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *term4(void) {
	symbolType *a = NULL;
	symbolType *p = NULL;
	symbolType *s;
	if ((p = factor1()) == NULL) {
		goto err;
	}
	s = scanPeek();
	if (s->id == kwOpMul || s->id == kwOpDiv) {
		if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
			goto err;
		}
		scanNext();
		a->l = p;
		if ((a->r = term4()) == NULL) {
			goto err;
		}
		return(a);
	}
	return(p);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *factor1(void) {
	symbolType *a = NULL;
	symbolType *p = NULL;
	symbolType *s;
	if ((p = factor2()) == NULL) {
		goto err;
	}
	s = scanPeek();
	if (s->id == kwOpExp) {
		if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
			goto err;
		}
		scanNext();
		a->l = p;
		if ((a->r = factor1()) == NULL) {
			goto err;
		}
		return(a);
	}
	return(p);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *factor2(void) {
	symbolType *a = NULL;
	symbolType *p = NULL;
	symbolType *s;
	s = scanPeek();
	if (s->id == kwOpAdd || s->id == kwOpSub) {
		scanNext();
		if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
			goto err;
		}
		a->id = (s->id == kwOpAdd) ? kwSignPlus : kwSignMinus;
		if ((a->r = factor3()) == NULL) {
			goto err;
		}
		return(a);
	} else {
		p = factor3();
	}
	return(p);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *factor3(void) {
	symbolType *p = NULL;
	symbolType *s = scanPeek();
	switch (s->id) {
		case kwParenthesesOpen:
			scanNext();
			if ((p = scanNewSymbol(kwSubExpression, NULL)) == NULL) {
				goto err;
			}
			p->id = kwSubExpression;
			if ((p->r = aexp()) == NULL) {
				goto err;
			}
			s = scanPeek();
			if (s->id != kwParenthesesClose) {
				utilError("expected closing parentheses");
				goto err;
			}
			scanNext();
			break;
		case kwIdentifier:
			if ((p = scanNewSymbol(kwIdentifier, s->value)) == NULL) {
				goto err;
			}
			scanNext();
			s = scanPeek();
			if (s->id == kwParenthesesOpen) {
				scanNext();
				if ((p->l = aexp()) == NULL) {
					utilError("expecting expression");
					goto err;
				}
				s = scanPeek();
				if (s->id == kwComma) {
					scanNext();
					if ((p->r = aexp()) == NULL) {
						utilError("expecting expression");
						goto err;
					}
				}
				s = scanPeek();
				if (s->id != kwParenthesesClose) {
					utilError("expecting close parentheses");
					goto err;
				}
				scanNext();
			}
			if (strchr(p->value, '$') != NULL) {
				s = scanPeek();
				if (s->id != kwLogicalLT && s->id != kwLogicalLTE && s->id != kwLogicalEQ && s->id != kwLogicalNE && s->id != kwLogicalGT && s->id != kwLogicalGTE) {
					goto err;
				}
				p->id = s->id;
				if ((p->l = scanNewSymbol(kwIdentifier, p->value)) == NULL) {
					goto err;
				}
				p->value = NULL;
				scanNext();
				if ((p->r = sexp()) == NULL) {
					utilError("expecting string expression");
					goto err;
				}
			}
			break;
		case kwString:
			if ((p = scanNewSymbol(kwString, s->value)) == NULL) {
				goto err;
			}
			scanNext();
			s = scanPeek();
			if (s->id != kwLogicalLT && s->id != kwLogicalLTE && s->id != kwLogicalEQ && s->id != kwLogicalNE && s->id != kwLogicalGT && s->id != kwLogicalGTE) {
				utilError("expecting logical operator");
				goto err;
			}
			if ((p->l = scanNewSymbol(kwString, p->value)) == NULL) {
				goto err;
			}
			p->id = s->id;
			p->value = NULL;
			scanNext();
			if ((p->r = sexp()) == NULL) {
				utilError("expecting string expression");
				goto err;
			}
			break;
		case kwNumeric:
			if ((p = scanNewSymbol(kwNumeric, s->value)) == NULL) {
				goto err;
			}
			scanNext();
			break;
		case kwABS:
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
		case kwSTR:
			if ((p = scanNewSymbol(s->id, NULL)) == NULL) {
				goto err;
			}
			scanNext();
			s = scanPeek();
			if (s->id != kwParenthesesOpen) {
				utilError("expecting open parentheses");
				goto err;
			}
			scanNext();
			if ((p->l = aexp()) == NULL) {
				utilError("expecting arithmetic expression");
				goto err;
			}
			s = scanPeek();
			if (s->id != kwParenthesesClose) {
				utilError("expecting close parentheses");
				goto err;
			}
			scanNext();
			break;
		case kwASC:
		case kwVAL:
			if ((p = scanNewSymbol(s->id, NULL)) == NULL) {
				goto err;
			}
			scanNext();
			s = scanPeek();
			if (s->id != kwParenthesesOpen) {
				utilError("expecting open parentheses");
				goto err;
			}
			scanNext();
			if ((p->l = sexp()) == NULL) {
				utilError("expecting string expression");
				goto err;
			}
			scanNext();
			s = scanPeek();
			if (s->id != kwParenthesesClose) {
				utilError("expecting close parentheses");
				goto err;
			}
			break;
	}
	return(p);
err:
	if (p != NULL) {
		progDeleteExpression(p);
	}
	return(NULL);
}


static symbolType *sexp(void) {
	symbolType *s;
	symbolType *n = NULL;
	s = scanPeek();
	if (s->id == kwString) {
		if ((n = scanNewSymbol(kwString, s->value)) == NULL) {
			goto err;
		}
		scanNext();
	} else if (s->id == kwIdentifier) {
		if (strchr(s->value, '$') != NULL) {
			if ((n = scanNewSymbol(kwIdentifier, s->value)) == NULL) {
				goto err;
			}
			scanNext();
			s = scanPeek();
			if (s->id == kwParenthesesOpen) {
				scanNext();
				if ((n->l = aexp()) == NULL) {
					utilError("expecting arithmetic expression");
					goto err;
				}
				s = scanPeek();
				if (s->id == kwComma) {
					scanNext();
					if ((n->r = aexp()) == NULL) {
						utilError("expecting arithmetic expression");
						goto err;
					}
					s = scanPeek();
				}
				if (s->id != kwParenthesesClose) {
					utilError("expecting close parentheses");
					goto err;
				}
				scanNext();
			}
		} else {
			goto err;
		}
	} else if (s->id == kwCHR) {
		if ((n = scanNewSymbol(s->id, NULL)) == NULL) {
			goto err;
		}
		scanNext();
		s = scanPeek();
		if (s->id != kwParenthesesOpen) {
			utilError("expecting open parentheses");
			goto err;
		}
		scanNext();
		if ((n->l = aexp()) == NULL) {
			utilError("expecting arithmetic expression");
			goto err;
		}
		s = scanPeek();
		if (s->id != kwParenthesesClose) {
			utilError("expecting close parentheses");
			goto err;
		}
		scanNext();
	}
	else if (s->id == kwSTR) {
		if ((n = scanNewSymbol(s->id, NULL)) == NULL) {
			goto err;
		}
		scanNext();
		s = scanPeek();
		if (s->id != kwParenthesesOpen) {
			utilError("expecting open parentheses");
			goto err;
		}
		scanNext();
		if ((n->l = aexp()) == NULL) {
			utilError("expecting arithmetic expression");
			goto err;
		}
		s = scanPeek();
		if (s->id != kwParenthesesClose) {
			utilError("expecting close parentheses");
			goto err;
		}
		scanNext();
	}
	return(n);
err:
	if (n != NULL) {
		free(n);
	}
	return(NULL);
}


static symbolType *parseAssignment(void) {
	symbolType *s;
	symbolType *i = NULL;
	symbolType *e = NULL;
	symbolType *a = NULL;
	s = scanPeek();
	if ((i = scanNewSymbol(kwIdentifier, s->value)) == NULL) {
		goto err;
	}
	scanNext();
	s = scanPeek();
	if (s->id == kwParenthesesOpen) {
		scanNext();
		if ((i->l = aexp()) == NULL) {
			utilError("expecting arithmetic expression");
			goto err;
		}
		s = scanPeek();
		if (s->id == kwComma) {
			scanNext();
			if ((i->r = aexp()) == NULL) {
				utilError("expecting arithmetic expression");
				goto err;
			}
		}
		s = scanPeek();
		if (s->id != kwParenthesesClose) {
			utilError("expecting close parentheses");
			goto err;
		}
		scanNext();
	} 
	s = scanPeek();
	if (s->id == kwLogicalEQ) {
		scanNext();
		s = scanPeek();
		if (strchr(i->value, '$') != NULL) {
			if ((e = sexp()) == NULL) {
				utilError("expecting string expression");
				goto err;
			}
		} else {
			if ((e = aexp()) == NULL) {
				utilError("expecting arithmetic expression");
				goto err;
			}
		}
	} else {
		utilError("expecting assignment operator '=', got %i\n", s->id);
		goto err;
	}
	if ((a = scanNewSymbol(kwAssignment, NULL)) == NULL) {
		goto err;
	}
	a->l = i;
	a->r = e;
	return(a);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} else {
		if (i != NULL) {
			progDeleteExpression(i);
		}
		if (e != NULL) {
			progDeleteExpression(e);
		}
	}
	return(NULL);
}


static int insAssignment(progLineType *pl) {
	symbolType *a = NULL;
	if ((a = parseAssignment()) == NULL) {
		goto err;
	}
	if (progAppendInstruction(pl, kwAssignment, a)) {
		goto err;
	}
	return(0);
err:
	return(1);
}


static int insBYE(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwBYE, NULL));
}

static int insCLR(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwCLR, NULL));
}


static int insCLS(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwCLS, NULL));
}


static int insCONT(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwCONT, NULL));
}


static int insDATA(progLineType *pl) {
	symbolType *s = NULL;
	char **t = NULL;
	int i = 0;
	s = scanGetText();
	if (s->id != kwText) {
		utilError("unexpected string: [%s]", s->value);
		goto err;
	}
	if ((t = strsplitz(s->value, ",")) == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	scanNext();
	if (progAppendInstruction(pl, kwDATA, t)) {
		goto err;
	}
	return(0);
err:
	if (t != NULL) {
		free(t);
	}
	return(1);
}


static int insDIM(progLineType *pl) {
	symbolType **aa = NULL;
	symbolType **a = NULL;
	symbolType *id = NULL;
	symbolType *e1 = NULL;
	symbolType *e2 = NULL;
	symbolType *s;
	unsigned long int maxDims = 1024;
	unsigned long int numDims = 0;
	if ((a = malloc(sizeof(symbolType *) * maxDims)) == NULL) {
		goto err;
	}
	while (1) {
		scanNext();
		s = scanPeek();
		if ((id = scanNewSymbol(kwIdentifier, s->value)) == NULL) {
			goto err;
		}
		scanNext();
		s = scanPeek();
		if (s->id != kwParenthesesOpen) {
			utilError("expecting opening parentheses");
			goto err;
		}
		scanNext();
		if ((e1 = aexp()) == NULL) {
			utilError("expecting arithmetic expression");
			goto err;
		}
		s = scanPeek();
		if (s->id == kwComma) {
			scanNext();
			if ((e2 = aexp()) == NULL) {
				utilError("expecting arithmetic expression");
				goto err;
			}
		}
		s = scanPeek();
		if (s->id != kwParenthesesClose) {
			utilError("expecting closing parentheses");
			goto err;
		}
		id->l = e1;
		id->r = e2;
		a[numDims++] = id;
		id = NULL;
		e1 = NULL;
		e2 = NULL;
		scanNext();
		s = scanPeek();
		if (s->id != kwComma) {
			break;
		}
		if (numDims == maxDims) {
			maxDims <<= 1;
			if ((aa = realloc(a, sizeof(symbolType *) * maxDims)) == NULL) {
				utilError("couldn't allocate memory");
				goto err;
			}
			a = aa;
		}
	}
	if ((aa = realloc(a, sizeof(symbolType *) * numDims)) == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	a = aa;
	if (progAppendInstruction(pl, kwDIM, a, numDims)) {
		goto err;
	}
	return(0);
err:
	if (a != NULL) {
		while (numDims) {
			numDims--;
			progDeleteExpression(a[numDims]->l);
			progDeleteExpression(a[numDims]->r);
			free(a[numDims]);
		}
		free(a);
	}
	if (id != NULL) {
		progDeleteExpression(id);
	}
	if (e1 != NULL) {
		progDeleteExpression(e1);
	}
	if (e2 != NULL) {
		progDeleteExpression(e2);
	}
	return(1);
}


static int insEND(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwEND, NULL));
}


static int insFOR(progLineType *pl) {
	symbolType **a = NULL;
	symbolType *step = NULL;
	symbolType *to = NULL;
	symbolType *assign = NULL;
	symbolType *s;
	scanNext();
	if ((assign = parseAssignment()) == NULL) {
		utilError("expecting assignment");
		goto err;
	}
	s = scanPeek();
	if (s->id != kwTO) {
		utilError("expecting TO");
		goto err;
	}
	scanNext();
	if ((to = aexp()) == NULL) {
		utilError("expecting arithmetic expression");
		goto err;
	}
	s = scanPeek();
	if (s->id == kwSTEP) {
		scanNext();
		if ((step = aexp()) == NULL) {
			utilError("expecting arithmetic expression");
			goto err;
		}
	}
	if (progAppendInstruction(pl, kwFOR, assign, to, step)) {
		goto err;
	}
	scanNext();
	return(0);
err:
	if (assign != NULL) {
		progDeleteExpression(assign);
	}
	if (to != NULL) {
		progDeleteExpression(to);
	}
	if (step != NULL) {
		progDeleteExpression(step);
	}
	return(1);
}


static int insGOSUB(progLineType *pl) {
	scanNext();
	symbolType *s = aexp();
	if (s == NULL) {
		utilError("expected aexp");
		goto err;
	}
	if (progAppendInstruction(pl, kwGOSUB, s)) {
		goto err;
	}
	return(0);
err:
	if (s != NULL) {
		progDeleteExpression(s);
	}
	return(1);
}


static int insGOTO(progLineType *pl) {
	scanNext();
	symbolType *s = aexp();
	if (s == NULL) {
		utilError("expected aexp");
		return(1);
	}
	if (progAppendInstruction(pl, kwGOTO, s)) {
		goto err;
	}
	return(0);
err:
	if (s != NULL) {
		progDeleteExpression(s);
	}
	return(1);
}


static int insIF(progLineType *pl) {
	symbolType *e = NULL;
	void *i = NULL;
	symbolType *s;
	progLineType ip;
	int iIsExp = 1;
	memset(&ip, 0, sizeof(progLineType));
	scanNext();
	if ((e = aexp()) == NULL) {
		utilError("expecting arithmetic expression");
		goto err;
	}
	s = scanPeek();
	if (s->id != kwTHEN) {
		utilError("expecting keyword 'then'");
		goto err;
	}
	scanNext();
	s = scanPeek();
	if (s->id == kwNumeric) {
		if ((i = aexp()) == NULL) {
			goto err;
		}
		iIsExp = 1;
	} else {
		if (compoundStatement(&ip)) {
			utilError("expecting statement");
			goto err;
		}
		i = ip.firstInstruction;
		iIsExp = 0;
	}
	if (progAppendInstruction(pl, kwIF, e, i, iIsExp)) {
		goto err;
	}
	return(0);
err:
	if (i) {
		if (!iIsExp) {
			progDeleteInstructions(i);
		} else {
			progDeleteExpression(i);
		}
	}
	if (e) {
		progDeleteExpression(e);
	}
	return(1);
}


static int insINPUT(progLineType *pl) {
	symbolType **aa = NULL;
	symbolType **a = NULL;
	symbolType *id = NULL;
	symbolType *e1 = NULL;
	symbolType *e2 = NULL;
	symbolType *s;
	unsigned long int maxVars = 1024;
	unsigned long int numVars = 0;
	if ((a = malloc(sizeof(symbolType *) * maxVars)) == NULL) {
		goto err;
	}
	while (1) {
		scanNext();
		s = scanPeek();
		if (s->id != kwIdentifier) {
			utilError("expecting identifier");
			goto err;
		}
		if ((id = scanNewSymbol(kwIdentifier, s->value)) == NULL) {
			goto err;
		}
		scanNext();
		s = scanPeek();
		if (s->id == kwParenthesesOpen) {
			if (strchr(id->value, '$') != NULL) {
				utilError("string variable cannot be subscripted");
				goto err;
			}
			scanNext();
			if ((e1 = aexp()) == NULL) {
				utilError("expecting arithmetic expression");
				goto err;
			}
			s = scanPeek();
			if (s->id == kwComma) {
				if (strchr(id->value, '$') != NULL) {
					utilError("only one dimension allowed for string variables");
					goto err;
				}
				scanNext();
				if ((e2 = aexp()) == NULL) {
					utilError("expecting arithmetic expression");
					goto err;
				}
			}
			s = scanPeek();
			if (s->id != kwParenthesesClose) {
				utilError("expecting closing parentheses");
				goto err;
			}
			scanNext();
			id->l = e1;
			id->r = e2;
		}
		a[numVars++] = id;
		id = NULL;
		e1 = NULL;
		e2 = NULL;
		s = scanPeek();
		if (s->id != kwComma) {
			break;
		}
		if (numVars == maxVars) {
			maxVars <<= 1;
			if ((aa = realloc(a, (sizeof(symbolType *) * maxVars))) == NULL) {
				utilError("couldn't allocate memory");
				goto err;
			}
			a = aa;
			goto err;
		}
	}
	if ((aa = realloc(a, (sizeof(symbolType *) * numVars))) == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	a = aa;
	if (progAppendInstruction(pl, kwINPUT, a, numVars)) {
		goto err;
	}
	return(0);
err:
	if (a != NULL) {
		while (numVars) {
			numVars--;
			progDeleteExpression(a[numVars]->l);
			progDeleteExpression(a[numVars]->r);
			free(a[numVars]);
		}
		free(a);
	}
	if (id != NULL) {
		progDeleteExpression(id);
	}
	if (e1 != NULL) {
		progDeleteExpression(e1);
	}
	if (e2 != NULL) {
		progDeleteExpression(e2);
	}
	return(1);
}


static int insLET(progLineType *pl) {
	symbolType *a = NULL;
	scanNext();
	if ((a = parseAssignment()) == NULL) {
		goto err;
	}
	if (progAppendInstruction(pl, kwLET, a)) {
		goto err;
	}
	scanNext();
	return(0);
err:
	if (a != NULL) {
		progDeleteExpression(a);
	} 
	return(1);
}


static int insLIST(progLineType *pl) {
	symbolType *l1 = NULL;
	symbolType *l2 = NULL;
	symbolType *s;
	scanNext();
	s = scanPeek();
	if (s->id != kwEol) {
		if ((l1 = aexp()) == NULL) {
			goto err;
		}
		s = scanPeek();
		if (s->id == kwComma) {
			scanNext();
			if ((l2 = aexp()) == NULL) {
				goto err;
			}
			scanNext();
		}
	}
	if (progAppendInstruction(pl, kwLIST, l1, l2)) {
		goto err;
	}
	return(0);
err:
	if (l1 != NULL) {
		progDeleteExpression(l1);
	}
	if (l2 != NULL) {
		progDeleteExpression(l2);
	}
	return(1);
}


static int insLOAD(progLineType *pl) {
	symbolType *s = NULL;
	scanNext();
	if ((s = sexp()) == NULL) {
		utilError("expecting string expression");
		goto err;
	}
	if (progAppendInstruction(pl, kwLOAD, s)) {
		goto err;
	}
	scanNext();
	return(0);
err:
	if (s != NULL) {
		progDeleteExpression(s);
	}
	return(1);
}


static int insNEW(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwNEW, NULL));
}


static int insNEXT(progLineType *pl) {
	symbolType *s = NULL;
	symbolType *v = NULL;
	scanNext();
	s = scanPeek();
	if (s->id != kwIdentifier) {
		utilError("expecting identifier");
		goto err;
	}
	if ((v = scanNewSymbol(kwIdentifier, s->value)) == NULL) {
		goto err;
	}
	if (progAppendInstruction(pl, kwNEXT, v)) {
		goto err;
	}
	scanNext();
	return(0);
err:
	if (v != NULL) {
		progDeleteExpression(v);
	}
	return(1);
}


static int insON(progLineType *pl) {
	symbolType *s = NULL;
	symbolType *i = NULL;
	symbolType *v = NULL;
	symbolType **t = NULL;
	symbolType **tt = NULL;
	unsigned long int maxTargets = 1024;
	unsigned long int numTargets = 0;
	scanNext();
	if ((v = aexp()) == NULL) {
		utilError("expecting arithmetic expression");
		goto err;
	}
	s = scanPeek();
	if (s->id != kwGOTO && s->id != kwGOSUB) {
		utilError("expecting GOTO or GOSUB");
		goto err;
	}
	if ((i = scanNewSymbol(s->id, NULL)) == NULL) {
		goto err;
	}
	if ((t = malloc(sizeof(symbolType *) * maxTargets)) == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	while (1) {
		scanNext();
		if ((t[numTargets] = aexp()) == NULL) {
			utilError("expecting arithmetic expression");
			goto err;
		}
		numTargets++;
		s = scanPeek();
		if (s->id != kwComma) {
			break;
		}
		if (numTargets == maxTargets) {
			maxTargets <<= 1;
			if ((tt = realloc(t, sizeof(symbolType *) * maxTargets)) == NULL) {
				utilError("couldn't allocate memory");
				goto err;
			}
			t = tt;
		}
	}
	if ((tt = realloc(t, sizeof(symbolType *) * numTargets)) == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	t = tt;
	if (progAppendInstruction(pl, kwON, v, i, t, numTargets)) {
		goto err;
	}
	return(0);
err:
	if (i != NULL) {
		progDeleteExpression(i);
	}
	if (v != NULL) {
		progDeleteExpression(v);
	}
	if (t != NULL) {
		while (numTargets) {
			numTargets--;
			progDeleteExpression(t[numTargets]);
		}
		free(t);
	}
	return(1);
}


static int insPOP(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwPOP, NULL));
}


static int insPRINT(progLineType *pl) {
	symbolType *s;
	symbolType *e;
	symbolType **aa = NULL;
	symbolType **a = NULL;
	unsigned long int maxExps = 1024;
	unsigned long int numExps = 0;
	if ((a = malloc(sizeof(symbolType *) * maxExps)) == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	scanNext();
	while (1) {
		s = scanPeek();
		if ((e = sexp()) != NULL) {
			a[numExps++] = e;
		} else if ((e = aexp()) != NULL) {
			a[numExps++] = e;
		} else if (s->id == kwComma || s->id == kwSemicolon) {
			if ((a[numExps++] = scanNewSymbol(s->id, NULL)) == NULL) {
				goto err;
			}
			scanNext();
		} else if (s->id == kwColon || s->id == kwEol || s->id == kwEof) {
			break;
		} 
		if (numExps == maxExps) {
			maxExps <<= 1;
			if ((aa = realloc(a, sizeof(symbolType *) * maxExps)) == NULL) {
				utilError("couldn't allocate memory");
				goto err;
			}
			a = aa;
		}
	}
	if (numExps > 0) {
		if ((aa = realloc(a, sizeof(symbolType *) * numExps)) == NULL) {
			utilError("couldn't allocate memory");
			goto err;
		}
	} else {
		free(a);
		aa = NULL;
	}
	a = aa;
	if (progAppendInstruction(pl, kwPRINT, a, numExps)) {
		goto err;
	}
	return(0);
err:
	if (a != NULL) {
		while (numExps) {
			numExps--;
			progDeleteExpression(a[numExps]);
		}
		free(a);
	}
	return(1);
}


static int insREAD(progLineType *pl) {
	symbolType **aa = NULL;
	symbolType **a = NULL;
	symbolType *id = NULL;
	symbolType *e1 = NULL;
	symbolType *e2 = NULL;
	symbolType *s;
	unsigned long int maxVars = 1024;
	unsigned long int numVars = 0;
	if ((a = malloc(sizeof(symbolType *) * maxVars)) == NULL) {
		goto err;
	}
	while (1) {
		scanNext();
		s = scanPeek();
		if (s->id != kwIdentifier) {
			utilError("expecting identifier");
			goto err;
		}
		if ((id = scanNewSymbol(kwIdentifier, s->value)) == NULL) {
			goto err;
		}
		scanNext();
		s = scanPeek();
		if (s->id == kwParenthesesOpen) {
			if (strchr(id->value, '$') != NULL) {
				utilError("string variable cannot be subscripted");
				goto err;
			}
			scanNext();
			if ((e1 = aexp()) == NULL) {
				utilError("expecting arithmetic expression");
				goto err;
			}
			s = scanPeek();
			if (s->id == kwComma) {
				scanNext();
				if ((e2 = aexp()) == NULL) {
					utilError("expecting arithmetic expression");
					goto err;
				}
			}
			s = scanPeek();
			if (s->id != kwParenthesesClose) {
				utilError("expecting closing parentheses");
				goto err;
			}
			scanNext();
			id->l = e1;
			id->r = e2;
		}
		a[numVars++] = id;
		id = NULL;
		e1 = NULL;
		e2 = NULL;
		s = scanPeek();
		if (s->id != kwComma) {
			break;
		}
		if (numVars == maxVars) {
			maxVars <<= 1;
			if ((aa = realloc(a, sizeof(symbolType *) * maxVars)) == NULL) {
				utilError("couldn't allocate memory");
				goto err;
			}
			a = aa;
		}
	}
	if ((aa = realloc(a, sizeof(symbolType *) * numVars)) == NULL) {
		utilError("couldn't allocate memory");
		goto err;
	}
	a = aa;
	if (progAppendInstruction(pl, kwREAD, a, numVars)) {
		goto err;
	}
	return(0);
err:
	if (a != NULL) {
		while (numVars) {
			numVars--;
			progDeleteExpression(a[numVars]->l);
			progDeleteExpression(a[numVars]->r);
			free(a[numVars]);
		}
		free(a);
	}
	if (id != NULL) {
		progDeleteExpression(id);
	}
	if (e1 != NULL) {
		progDeleteExpression(e1);
	}
	if (e2 != NULL) {
		progDeleteExpression(e2);
	}
	return(1);
}


static int insREM(progLineType *pl) {
	symbolType *s = scanGetText();
	char *t = strdup(s->value);
	if (t == NULL) {
		utilError("couldn't allocate memory");
		return(1);
	}
	scanNext();
	return(progAppendInstruction(pl, kwREM, t));
}


static int insRESTORE(progLineType *pl) {
	symbolType *s;
	symbolType *e = NULL;
	scanNext();
	s = scanPeek();
	if (s->id != kwEol) {
		if ((e = aexp()) == NULL) {
			goto err;
		}
	}
	if (progAppendInstruction(pl, kwRESTORE, e)) {
		goto err;
	}
	scanNext();
	return(0);
err:
	if (e != NULL) {
		progDeleteExpression(e);
	}
	scanNext();
	return(1);
}


static int insRETURN(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwRETURN, NULL));
}


static int insRUN(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwRUN, NULL));
}


static int insSAVE(progLineType *pl) {
	symbolType *s = NULL;
	scanNext();
	if ((s = sexp()) == NULL) {
		utilError("expecting string expression");
		goto err;
	}
	if (progAppendInstruction(pl, kwSAVE, s)) {
		goto err;
	}
	scanNext();
	return(0);
err:
	if (s != NULL) {
		progDeleteExpression(s);
	}
	return(1);
}


static int insSTOP(progLineType *pl) {
	scanNext();
	return(progAppendInstruction(pl, kwSTOP, NULL));
}


static int insTRAP(progLineType *pl) {
	symbolType *s = NULL;
	scanNext();
	if ((s = aexp()) == NULL) {
		utilError("expected aexp");
		goto err;
	}
	if (progAppendInstruction(pl, kwTRAP, s)) {
		goto err;
	}
	scanNext();
	return(0);
err:
	return(1);
}


static int statement(progLineType *pl) {
	symbolType *s = scanPeek();
	int rc = 0;
    switch (s->id) {
        case kwCLR:
            rc = insCLR(pl);
            break;
        case kwCLS:
            rc = insCLS(pl);
            break;
		case kwCONT:
			rc = insCONT(pl);
			break;
        case kwDATA:
            rc = insDATA(pl);
            break;
        case kwDIM:
            rc = insDIM(pl);
            break;
        case kwEND:
            rc = insEND(pl);
            break;
        case kwFOR:
            rc = insFOR(pl);
            break;
        case kwGOSUB:
            rc = insGOSUB(pl);
            break;
        case kwGOTO:
            rc = insGOTO(pl);
            break;
        case kwIF:
            rc = insIF(pl);
            break;
        case kwINPUT:
            rc = insINPUT(pl);
            break;
        case kwLET:
            rc = insLET(pl);
            break;
        case kwLIST:
            rc = insLIST(pl);
            break;
		case kwLOAD:
			rc = insLOAD(pl);
			break;
        case kwNEXT:
            rc = insNEXT(pl);
            break;
		case kwNEW:
			rc = insNEW(pl);
			break;
        case kwON:
            rc = insON(pl);
            break;
        case kwPOP:
            rc = insPOP(pl);
            break;
        case kwPRINT:
            rc = insPRINT(pl);
            break;
        case kwREAD:
            rc = insREAD(pl);
            break;
        case kwREM:
            rc = insREM(pl);
            break;
        case kwRESTORE:
            rc = insRESTORE(pl);
            break;
        case kwRETURN:
            rc = insRETURN(pl);
            break;
		case kwRUN:
			rc = insRUN(pl);
			break;
		case kwSAVE:
			rc = insSAVE(pl);
			break;
        case kwSTOP:
            rc = insSTOP(pl);
            break;
        case kwTRAP:
            rc = insTRAP(pl);
            break;
		case kwIdentifier:
			rc = insAssignment(pl);
			break;
        default:
            rc = 1;
            utilError("unrecognized statement: %s", s->value);
    }
	return(rc);
}


static int compoundStatement(progLineType *pl) {
    symbolType *s;
	while (1) {
		s = scanPeek();
		if (s->id <= kwTRAP || s->id == kwIdentifier) {
			if (statement(pl)) {
				return(1);
			}
			s = scanPeek();
		} else if (s->id == kwEol || s->id == kwEof) {
			return(0);
		} else if (s->id == kwColon) {
			scanNext();
		} else {
			utilError("unexpected input: %s", s->value);
			return(1);
		}
	}
}


static void deferredStatement(void) {
	symbolType *s = scanPeek();
	progLineType *pl = progInsertLine(atoi(s->value));
	scanNext();
	if (pl == NULL) {
		utilError("couldn't allocate memory");
		return;
	}
	if (compoundStatement(pl)) {
		progDeleteLine(pl);
	}
	scanConsumeEol();
}


static void immediateStatement(void) {
	progLineType pl;
	memset(&pl, 0, sizeof(progLineType));
	pl.lineNum = -1;
	if (compoundStatement(&pl)) {
		progDeleteInstructions(pl.firstInstruction);
		goto err;
	}
	progExecuteLine(&pl);
	progDeleteInstructions(pl.firstInstruction);
err:
	utilReady();
	scanConsumeEol();
}


void parseProgram(void) {
	symbolType *s = NULL;
	scanNext();
	while (1) {
		s = scanPeek();
        switch (s->id) {
            case kwBYE:
            case kwEof:
                goto term;
                break;
			case kwEol:
				scanConsumeEol();
				break;
            case kwNumeric:
                deferredStatement();
                break;
			default:
				immediateStatement();
				break;
        }
	}
term:
	return;
}
