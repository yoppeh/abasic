/**
 * @file parse.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief The parser: turns lines of BASIC into program lines and runs immediate ones.
 * @details A recursive-descent parser over the scanner's tokens. Expressions
 * follow the BNF's levels: aexp (OR), term1 (AND, NOT), term2 (comparisons),
 * term3 (+ -), term4 (* /), factor1 (^), factor2 (sign), factor3 (operands);
 * sexp is a string expression. Each insXXX() parses one statement, whose
 * keyword is the current token, and adds it to a line.
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "io.h"
#include "prog.h"
#include "scan.h"
#include "util.h"
#include "var.h"

static symbolType *term1(void);
static symbolType *notTerm(void);
static symbolType *leftChain(symbolType *(*operand)(void), const keywords *ops, size_t numOps);
static symbolType *term2(void);
static symbolType *term3(void);
static symbolType *term4(void);
static symbolType *factor1(void);
static symbolType *factor2(void);
static symbolType *factor3(void);
static symbolType *aexp(void);
static symbolType *sexp(void);
static symbolType *stringComparison(symbolType *left);

static symbolType *parseAssignment(void);
static int insAssignment(progLineType *p);
static int insBYE(progLineType *pl);
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
static int insNEW(progLineType *pl);
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
static void showSyntaxError(long column);
static int insDEG(progLineType *pl);
static int insRAD(progLineType *pl);
static int endOfStatement(symbolType *s);
static void immediateStatement(void);


/// @brief Parses <operand> [<op> <operand>]...: a left-associative chain of
/// binary operator nodes.
/// @details That is what the BNF's left-recursive rules (<aexp> ::= <aexp> OR
/// <term1>, ...) describe. ops lists the operators accepted at this level.
static symbolType *leftChain(symbolType *(*operand)(void), const keywords *ops, size_t numOps) {
    symbolType *a;
    symbolType *p;
    symbolType *s;
    size_t i;
    if ((p = operand()) == NULL) {
        return(NULL);
    }
    while (1) {
        s = scanPeek();
        for (i = 0; i < numOps && s->id != ops[i]; i++) {
        }
        if (i == numOps) {
            return(p);
        }
        if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
            break;
        }
        a->l = p;
        p = a;
        scanNext();
        if ((p->r = operand()) == NULL) {
            break;
        }
    }
    progDeleteExpression(p);
    return(NULL);
}


/// @brief Parses <aexp> ::= <term1> | <aexp> OR <term1>.
static symbolType *aexp(void) {
    static const keywords ops[] = { kwOR };
    return(leftChain(term1, ops, 1));
}


/// @brief Parses NOT <term2>, or a <term2>.
/// @details The BNF has NOT only at the start of a <term1>; it is also
/// accepted after AND (A AND NOT B) and repeated (NOT NOT A).
static symbolType *notTerm(void) {
    symbolType *p;
    if (scanPeek()->id != kwNOT) {
        return(term2());
    }
    if ((p = scanNewSymbol(kwNOT, NULL)) == NULL) {
        return(NULL);
    }
    scanNext();
    if ((p->r = notTerm()) == NULL) {
        progDeleteExpression(p);
        return(NULL);
    }
    return(p);
}


/// @brief Parses <term1> ::= <term2> | <term1> AND <term2> | NOT <term2>.
static symbolType *term1(void) {
    static const keywords ops[] = { kwAND };
    return(leftChain(notTerm, ops, 1));
}


/// @brief Parses <term2> ::= <term3> | <term2> <lop> <term3>.
static symbolType *term2(void) {
    static const keywords ops[] = { kwLogicalLT, kwLogicalLTE, kwLogicalEQ, kwLogicalNE, kwLogicalGT, kwLogicalGTE };
    return(leftChain(term3, ops, sizeof(ops) / sizeof(ops[0])));
}


/// @brief Parses <term3> ::= <term4> | <term3> <addop> <term4>.
/// @details term3 and term4 loop rather than recurse on the right, so the
/// operators are left-associative: 10-3-2 is (10-3)-2 and 8/4/2 is (8/4)/2.
static symbolType *term3(void) {
    symbolType *a = NULL;
    symbolType *p = NULL;
    symbolType *s;
    if ((p = term4()) == NULL) {
        goto err;
    }
    s = scanPeek();
    while (s->id == kwOpAdd || s->id == kwOpSub) {
        if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
            goto err;
        }
        a->l = p;
        p = a;
        scanNext();
        if ((p->r = term4()) == NULL) {
            goto err;
        }
        s = scanPeek();
    }
    return(p);
err:
    if (p != NULL) {
        progDeleteExpression(p);
    }
    return(NULL);
}


/// @brief Parses <term4> ::= <factor1> | <term4> <mulop> <factor1>.
static symbolType *term4(void) {
    symbolType *a = NULL;
    symbolType *p = NULL;
    symbolType *s;
    if ((p = factor1()) == NULL) {
        goto err;
    }
    s = scanPeek();
    while (s->id == kwOpMul || s->id == kwOpDiv) {
        if ((a = scanNewSymbol(s->id, NULL)) == NULL) {
            goto err;
        }
        a->l = p;
        p = a;
        scanNext();
        if ((p->r = factor1()) == NULL) {
            goto err;
        }
        s = scanPeek();
    }
    return(p);
err:
    if (p != NULL) {
        progDeleteExpression(p);
    }
    return(NULL);
}


/// @brief Parses <factor1> ::= <factor2> | <factor1> ^ <factor2>, so 2^3^2 is
/// (2^3)^2 = 64.
static symbolType *factor1(void) {
    static const keywords ops[] = { kwOpExp };
    return(leftChain(factor2, ops, 1));
}


/// @brief Parses <factor2> ::= <factor3> | <sign> <factor3>.
static symbolType *factor2(void) {
    symbolType *a = NULL;
    symbolType *p = NULL;
    symbolType *s;
    s = scanPeek();
    if (s->id == kwOpAdd || s->id == kwOpSub) {
        // s is the scanner's shared token, so read it before scanNext().
        keywords sign = (s->id == kwOpAdd) ? kwSignPlus : kwSignMinus;
        scanNext();
        if ((a = scanNewSymbol(sign, NULL)) == NULL) {
            goto err;
        }
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


/// @brief Parses <factor3>: a parenthesized <aexp>, a constant, a variable,
/// a function, or a comparison of strings.
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
                // The whole variable, subscripts included, is the left operand.
                return(stringComparison(p));
            }
            break;
        case kwString:
            if ((p = scanNewSymbol(kwString, s->value)) == NULL) {
                goto err;
            }
            scanNext();
            return(stringComparison(p));
        case kwCHR:
        case kwERR:
            if ((p = sexp()) == NULL) {
                goto err;
            }
            return(stringComparison(p));
        case kwERL:
            // The line number of the last error a TRAP caught.
            if ((p = scanNewSymbol(kwERL, NULL)) == NULL) {
                goto err;
            }
            scanNext();
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
        case kwLOG:
        case kwPEEK:
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
        case kwLEN:
        case kwVAL:
            // Functions of a string that return a number.
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
            s = scanPeek();
            if (s->id != kwParenthesesClose) {
                utilError("expecting close parentheses");
                goto err;
            }
            scanNext();
            break;
        default:
            break;
    }
    return(p);
err:
    if (p != NULL) {
        progDeleteExpression(p);
    }
    return(NULL);
}


/// @brief Parses the rest of a comparison of strings, left <op> <sexp>.
/// @details A string operand can only be used in a comparison with another
/// string. The comparison node's value is set to "$" so eval compares the
/// operands as strings rather than as numbers. Takes ownership of left.
static symbolType *stringComparison(symbolType *left) {
    symbolType *p = NULL;
    symbolType *s = scanPeek();
    if (s->id != kwLogicalLT && s->id != kwLogicalLTE && s->id != kwLogicalEQ && s->id != kwLogicalNE && s->id != kwLogicalGT && s->id != kwLogicalGTE) {
        utilError("expecting logical operator");
        goto err;
    }
    if ((p = scanNewSymbol(s->id, "$")) == NULL) {
        goto err;
    }
    p->l = left;
    left = NULL;
    scanNext();
    if ((p->r = sexp()) == NULL) {
        utilError("expecting string expression");
        goto err;
    }
    return(p);
err:
    progDeleteExpression(left);
    progDeleteExpression(p);
    return(NULL);
}


/// @brief Parses <sexp>: a string literal, a string variable or substring, or
/// a string function. Returns NULL without an error if there is none.
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
    } else if (s->id == kwERR) {
        // The message of the last error a TRAP caught.
        if ((n = scanNewSymbol(kwERR, NULL)) == NULL) {
            goto err;
        }
        scanNext();
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
    // n may already have subscript or argument expressions attached.
    progDeleteExpression(n);
    return(NULL);
}


/// @brief Parses <var> = <exp>, with an <sexp> for a string variable and an
/// <aexp> otherwise, and returns it as a kwAssignment node.
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
        utilError("expecting assignment operator '='");
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


/// @brief Parses an assignment without LET.
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


/// @brief Parses BYE.
static int insBYE(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwBYE, NULL));
}

/// @brief Parses CLR.
static int insCLR(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwCLR, NULL));
}


/// @brief Parses CLS.
static int insCLS(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwCLS, NULL));
}


/// @brief Parses CONT.
static int insCONT(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwCONT, NULL));
}


/// @brief Parses DEG.
static int insDEG(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwDEG, NULL));
}


/// @brief Parses RAD.
static int insRAD(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwRAD, NULL));
}


/// @brief Parses DATA <data string> [,<data string>]...: the rest of the line.
static int insDATA(progLineType *pl) {
    symbolType *s = NULL;
    char **t = NULL;
    int i = 0;
    if ((s = scanGetText()) == NULL) {
        goto err;
    }
    if (s->id != kwText) {
        utilError("unexpected string: [%s]", s->value);
        goto err;
    }
    if ((t = strsplitz(s->value, ",")) == NULL) {
        utilError("couldn't allocate memory");
        goto err;
    }
    // Trim spaces around each item: DATA 1, 2 stores "1" and "2".
    for (i = 0; t[i] != NULL; i++) {
        char *end;
        while (isspace((unsigned char)*t[i])) {
            t[i]++;
        }
        end = t[i] + strlen(t[i]);
        while (end > t[i] && isspace((unsigned char)end[-1])) {
            *--end = 0;
        }
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


/// @brief Parses DIM <var>(<aexp>[,<aexp>]) [,<var>(<aexp>[,<aexp>])]...
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


/// @brief Parses END.
static int insEND(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwEND, NULL));
}


/// @brief Parses FOR <avar> = <aexp> TO <aexp> [STEP <aexp>].
static int insFOR(progLineType *pl) {
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


/// @brief Parses GOSUB <aexp>.
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


/// @brief Parses GOTO <aexp> (or GO TO, whose TO is the current token).
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


/// @brief Parses IF <aexp> THEN {<lineno> | <compound statement>}.
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
        // Take ownership first: a failed compoundStatement can leave some instructions.
        iIsExp = 0;
        if (compoundStatement(&ip)) {
            i = ip.firstInstruction;
            utilError("expecting statement");
            goto err;
        }
        i = ip.firstInstruction;
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


/// @brief Parses INPUT <var> [,<var>]...
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
            // <avar> | <string id> only (BNF). "String variables are allowed
            // only if they are not subscripted. Matrix variables are not
            // allowed", where a matrix variable is "also called a Subscripted
            // Variable. An element of an array or matrix." (Manual, INPUT and
            // Table 1.1)
            utilError("INPUT takes only unsubscripted variables");
            goto err;
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


/// @brief Parses LET <var> = <exp>.
static int insLET(progLineType *pl) {
    symbolType *a = NULL;
    scanNext();
    if ((a = parseAssignment()) == NULL) {
        goto err;
    }
    if (progAppendInstruction(pl, kwLET, a)) {
        goto err;
    }
    return(0);
err:
    if (a != NULL) {
        progDeleteExpression(a);
    }
    return(1);
}


/// @brief Parses LIST [<aexp> [,<aexp>]].
static int insLIST(progLineType *pl) {
    symbolType *l1 = NULL;
    symbolType *l2 = NULL;
    symbolType *s;
    scanNext();
    s = scanPeek();
    if (!endOfStatement(s)) {
        if ((l1 = aexp()) == NULL) {
            goto err;
        }
        s = scanPeek();
        if (s->id == kwComma) {
            scanNext();
            if ((l2 = aexp()) == NULL) {
                goto err;
            }
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


/// @brief Parses LOAD <sexp>.
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
    return(0);
err:
    if (s != NULL) {
        progDeleteExpression(s);
    }
    return(1);
}


/// @brief Parses NEW.
static int insNEW(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwNEW, NULL));
}


/// @brief Parses NEXT <avar>.
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


/// @brief Parses ON <aexp> {GOTO | GOSUB} <aexp> [,<aexp>]...
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


/// @brief Parses POP.
static int insPOP(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwPOP, NULL));
}


/// @brief Parses PRINT [<exp>] [{, | ;} [<exp>]]...
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
        } else {
            // Nothing above consumed this token, so stop rather than loop forever.
            utilError("unexpected %s in PRINT", scanDescribe(s));
            goto err;
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


/// @brief Parses READ <var> [,<var>]...
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
            // <avar> | <string id> only (BNF). "String variables used in READ
            // statements must be dimensioned and cannot be subscripted ...
            // Neither may array variables be used", where an array variable
            // is "the whole array or any element" (Manual, READ and section 1)
            utilError("READ takes only unsubscripted variables");
            goto err;
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


/// @brief Parses REM: the rest of the line.
static int insREM(progLineType *pl) {
    symbolType *s = scanGetText();
    char *t;
    if (s == NULL) {
        return(1);
    }
    if ((t = strdup(s->value)) == NULL) {
        utilError("couldn't allocate memory");
        return(1);
    }
    scanNext();
    return(progAppendInstruction(pl, kwREM, t));
}


/// @brief Parses RESTORE [<aexp>].
static int insRESTORE(progLineType *pl) {
    symbolType *s;
    symbolType *e = NULL;
    scanNext();
    s = scanPeek();
    if (!endOfStatement(s)) {
        if ((e = aexp()) == NULL) {
            goto err;
        }
    }
    if (progAppendInstruction(pl, kwRESTORE, e)) {
        goto err;
    }
    return(0);
err:
    if (e != NULL) {
        progDeleteExpression(e);
    }
    return(1);
}


/// @brief Parses RETURN.
static int insRETURN(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwRETURN, NULL));
}


/// @brief Parses RUN.
static int insRUN(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwRUN, NULL));
}


/// @brief Parses SAVE <sexp>.
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
    return(0);
err:
    if (s != NULL) {
        progDeleteExpression(s);
    }
    return(1);
}


/// @brief Parses STOP.
static int insSTOP(progLineType *pl) {
    scanNext();
    return(progAppendInstruction(pl, kwSTOP, NULL));
}


/// @brief Parses TRAP <aexp>.
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
    return(0);
err:
    progDeleteExpression(s);
    return(1);
}


/// @brief Parses one statement, starting at its keyword (or a variable, for an
/// assignment without LET), and adds it to line pl. Returns 0 or 1.
static int statement(progLineType *pl) {
    symbolType *s = scanPeek();
    int rc = 0;
    switch (s->id) {
        case kwBYE:
            rc = insBYE(pl);
            break;
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
        case kwDEG:
            rc = insDEG(pl);
            break;
        case kwRAD:
            rc = insRAD(pl);
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
            if (!strcasecmp(s->value, "go")) {
                // GO TO is another way to write GOTO; insGOTO skips the TO.
                scanNext();
                if (scanPeek()->id != kwTO) {
                    utilError("expecting TO after GO");
                    rc = 1;
                    break;
                }
                rc = insGOTO(pl);
                break;
            }
            rc = insAssignment(pl);
            break;
        default:
            rc = 1;
            utilError("unrecognized statement: %s", scanDescribe(s));
    }
    return(rc);
}


/// @brief Tells whether a token ends a statement (':', end of line or input).
/// @details Every statement leaves the token after its last argument current,
/// and that must be one of these.
static int endOfStatement(symbolType *s) {
    return(s->id == kwColon || s->id == kwEol || s->id == kwEof);
}


/// @brief Parses <statement> [: <statement>]... up to the end of the line,
/// adding them to line pl. Returns 0, or 1 after a syntax error.
static int compoundStatement(progLineType *pl) {
    symbolType *s;
    while (1) {
        s = scanPeek();
        if (s->id <= kwTRAP || s->id == kwIdentifier) {
            // Any error reported while parsing the statement fails it, even if
            // the statement's own code carried on (PRINT (1 + 2 used to be
            // stored as a bare PRINT after reporting the missing ')').
            if (statement(pl) || utilErrorReported()) {
                return(1);
            }
            // Statements are separated by ':' (GOTO 10 PRINT 1 is an error).
            s = scanPeek();
            if (!endOfStatement(s)) {
                utilError("unexpected %s after statement", scanDescribe(s));
                return(1);
            }
        } else if (s->id == kwEol || s->id == kwEof) {
            return(0);
        } else if (s->id == kwColon) {
            scanNext();
        } else {
            utilError("unexpected %s", scanDescribe(s));
            return(1);
        }
    }
}


/// @brief After a syntax error, shows the line with a caret under the token
/// where the error was found.
/// @details column comes from scanTokenColumn() at the time of the error; the
/// rest of the line must already have been read.
static void showSyntaxError(long column) {
    const char *text;
    size_t len;
    size_t i;
    if (!utilErrorReported()) {
        utilError("syntax error");
    }
    text = ioLineText(&len);
    if (text == NULL || column < 0) {
        return;
    }
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        len--;
    }
    fflush(stdout);
    fprintf(stderr, "  %.*s\n  ", (int)len, text);
    // Keep tabs, so the caret lines up however tabs are displayed.
    for (i = 0; i < (size_t)column && i < len; i++) {
        fputc(text[i] == '\t' ? '\t' : ' ', stderr);
    }
    fputs("^\n", stderr);
}


/// @brief Handles a line that starts with a line number: stores it in the
/// program, or deletes the line if nothing follows the number.
static void deferredStatement(void) {
    symbolType *s = scanPeek();
    long int lineNum = atol(s->value);
    progLineType pl;
    progLineType *p;
    // Parse into a scratch line first, so a syntax error doesn't lose the old line.
    memset(&pl, 0, sizeof(progLineType));
    pl.lineNum = lineNum;
    if (strchr(s->value, '.') != NULL || strchr(s->value, 'e') != NULL || strchr(s->value, 'E') != NULL || lineNum > MAX_LINE_NUMBER) {
        long column = scanTokenColumn();
        utilError("line numbers must be whole numbers from 0 to %ld", MAX_LINE_NUMBER);
        scanSkipLine();
        showSyntaxError(column);
        scanConsumeEol();
        return;
    }
    // Syntax errors in this line say "at line N".
    utilErrorLine(lineNum);
    scanNext();
    if (compoundStatement(&pl)) {
        long column = scanTokenColumn();
        progDeleteInstructions(pl.firstInstruction);
        scanSkipLine();
        showSyntaxError(column);
    } else if (pl.firstInstruction == NULL) {
        // A line number on its own deletes the line.
        progRemoveLine(lineNum);
    } else if ((p = progInsertLine(lineNum)) == NULL) {
        utilError("couldn't allocate memory");
        progDeleteInstructions(pl.firstInstruction);
    } else {
        p->firstInstruction = pl.firstInstruction;
        p->lastInstruction = pl.lastInstruction;
    }
    scanConsumeEol();
}


/// @brief Handles a line without a line number: parses it and runs it at once.
static void immediateStatement(void) {
    progLineType pl;
    memset(&pl, 0, sizeof(progLineType));
    pl.lineNum = -1;
    if (compoundStatement(&pl)) {
        long column = scanTokenColumn();
        progDeleteInstructions(pl.firstInstruction);
        // Don't run what's left of the line as a new statement.
        scanSkipLine();
        showSyntaxError(column);
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
