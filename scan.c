/**
 * @file scan.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief The scanner: splits input into tokens.
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
#include "scan.h"
#include "util.h"

// "Variable names may be up to 120 characters long" (Manual, section 1).
#define MAX_NAME_LEN 120
// Room for text put back by splitName(): what's left of a few names.
#define MAX_PENDING 1024

/// @brief The keywords as a balanced binary tree over keywordTable, for lookup().
static symbolType *keywordTree = NULL;
/// @brief Every keyword and its name; scanInit() sorts it by name.
static symbolType keywordTable[] = {
    {NULL, NULL, "and", kwAND},
    {NULL, NULL, "or", kwOR},
    {NULL, NULL, "not", kwNOT},
    {NULL, NULL, "bye", kwBYE},
    {NULL, NULL, "clr", kwCLR},
    {NULL, NULL, "cls", kwCLS},
    {NULL, NULL, "cont", kwCONT},
    {NULL, NULL, "data", kwDATA},
    {NULL, NULL, "deg", kwDEG},
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
    {NULL, NULL, "rad", kwRAD},
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
    {NULL, NULL, "str$", kwSTR},
    {NULL, NULL, "peek", kwPEEK}
};
/// @brief The current token; its value buffer holds maxStringLen + 1 characters.
static symbolType *scanCurrent = NULL;
/// @brief The keywordTable entry for each keyword, for scanGetKeyword().
static symbolType *idToKeywordTable[kwNumKeywords];
/// @brief Where the current token starts in its input line, for error messages.
static long tokenColumn = -1;

/// @brief Text to scan before the rest of the input.
/// @details When a keyword is found at the start of a longer name (PRINTX is
/// PRINT, then X), the rest of the name is put back here. pendingPos is the
/// next character to scan and pendingLen how many there are.
static char pending[MAX_PENDING];
static size_t pendingLen = 0;
static size_t pendingPos = 0;
/// @brief Whether the last name scanned was followed by a space, so splitName()
/// can put the space back after the rest of the name.
static int nameSpaced = 0;

/// @brief Atari BASIC's statement table, in its order.
/// @details An abbreviation (PR. for PRINT) means the first statement here
/// that starts with it, which gives exactly the abbreviations in the BNF: R.
/// is REM, REA. READ, CL. CLOSE, E. ENTER. Hardware and I/O statements this
/// interpreter doesn't have are kept (id -1) so their abbreviations aren't
/// taken by something else. CLS isn't an Atari statement, so it goes last.
static const struct {
    const char *name;
    int id;
} statementTable[] = {
    {"rem", kwREM}, {"data", kwDATA}, {"input", kwINPUT}, {"color", -1},
    {"list", kwLIST}, {"enter", -1}, {"let", kwLET}, {"if", kwIF},
    {"for", kwFOR}, {"next", kwNEXT}, {"goto", kwGOTO}, {"gosub", kwGOSUB},
    {"trap", kwTRAP}, {"bye", kwBYE}, {"cont", kwCONT}, {"com", -1},
    {"close", -1}, {"clr", kwCLR}, {"deg", kwDEG}, {"dim", kwDIM},
    {"end", kwEND}, {"new", kwNEW}, {"open", -1}, {"load", kwLOAD},
    {"save", kwSAVE}, {"status", -1}, {"note", -1}, {"point", -1},
    {"xio", -1}, {"on", kwON}, {"poke", -1}, {"print", kwPRINT},
    {"rad", kwRAD}, {"read", kwREAD}, {"restore", kwRESTORE},
    {"return", kwRETURN}, {"run", kwRUN}, {"stop", kwSTOP}, {"pop", kwPOP},
    {"get", -1}, {"put", -1}, {"graphics", -1}, {"plot", -1},
    {"position", -1}, {"dos", -1}, {"drawto", -1}, {"setcolor", -1},
    {"locate", -1}, {"sound", -1}, {"lprint", -1}, {"csave", -1},
    {"cload", -1}, {"cls", kwCLS}
};

static int cmpKeywords(const void *a, const void *b);
static char nextChar(void);
static char peekChar(void);
static void splitName(keywords k, size_t n);
static char *scanGetLOp(void);
static char *scanGetName(void);
static char *scanGetNum(void);
static char *scanGetOp(void);
static char *scanGetString(void);
static symbolType *insertSymbol(symbolType *p, long s, long e);
static int islop(char c);
static int isop(char c);
static int lookup(symbolType *t, char *s);
static int lookupAbbreviation(const char *s);
static void scanExit(void);
static int scanToken(void);
static void skipWhite(void);


/// @brief Orders keywordTable entries by name, for qsort().
static int cmpKeywords(const void *a, const void *b) {
    return(strcmp(((symbolType *)a)->value, ((symbolType *)b)->value));
}


/// @brief Links p[s..e], which is sorted, into a balanced binary tree and
/// returns its root.
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


/// @brief Tells whether c is an arithmetic operator.
static int isop(char c) {
    return(c == '+' || c == '-' || c == '*' || c == '/' || c == '^');
}


/// @brief Tells whether c starts a comparison operator.
static int islop(char c) {
    return(c == '<' || c == '>' || c == '=');
}


/// @brief Returns the keyword named s (case doesn't matter) in tree t, or -1.
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


/// @brief Returns the statement keyword that abbreviation s (without its
/// period) stands for.
/// @details Reports an error and returns -1 if there is none, or if it is an
/// Atari statement this interpreter doesn't have.
static int lookupAbbreviation(const char *s) {
    size_t i;
    size_t len = strlen(s);
    for (i = 0; i < sizeof(statementTable) / sizeof(statementTable[0]); i++) {
        if (!strncasecmp(statementTable[i].name, s, len)) {
            if (statementTable[i].id == -1) {
                utilError("%s. is %s, which this BASIC doesn't have", s, statementTable[i].name);
            }
            return(statementTable[i].id);
        }
    }
    utilError("unknown abbreviation '%s.'", s);
    return(-1);
}


long scanTokenColumn(void) {
    return(tokenColumn);
}


/// @brief Moves to the next character: the next pending one, or the next of
/// the input. Returns the new current character.
static char nextChar(void) {
    if (pendingPos < pendingLen) {
        pendingPos++;
        return(peekChar());
    }
    return(ioNext());
}


/// @brief Returns the current character: pending text first, then the input.
static char peekChar(void) {
    return(pendingPos < pendingLen ? pending[pendingPos] : ioPeek());
}


/// @brief Makes the current token, an identifier, the keyword k whose name is
/// its first n characters, and puts the rest of the name back to be scanned.
static void splitName(keywords k, size_t n) {
    char buf[MAX_PENDING];
    const char *rest = scanCurrent->value + n;
    size_t restLen = strlen(rest);
    size_t oldLen = pendingLen - pendingPos;
    size_t space = nameSpaced ? 1 : 0;
    // Names are at most maxStringLen characters, so this always fits; refuse
    // rather than overflow if it ever didn't.
    if (restLen + space + oldLen > sizeof(buf)) {
        return;
    }
    memcpy(buf, rest, restLen);
    if (space) {
        buf[restLen] = ' ';
    }
    memcpy(buf + restLen + space, pending + pendingPos, oldLen);
    pendingLen = restLen + space + oldLen;
    pendingPos = 0;
    memcpy(pending, buf, pendingLen);
    scanCurrent->id = k;
    scanCurrent->value[0] = 0;
}


int scanSplitKeyword(keywords k) {
    const char *name = scanGetKeyword(k);
    size_t n;
    if (name == NULL || scanCurrent->id != kwIdentifier) {
        return(0);
    }
    n = strlen(name);
    if (strlen(scanCurrent->value) <= n || strncasecmp(scanCurrent->value, name, n)) {
        return(0);
    }
    splitName(k, n);
    return(1);
}


int scanSplitStatement(void) {
    size_t i;
    size_t n;
    if (scanCurrent->id != kwIdentifier) {
        return(0);
    }
    for (i = 0; i < sizeof(statementTable) / sizeof(statementTable[0]); i++) {
        n = strlen(statementTable[i].name);
        if (strlen(scanCurrent->value) > n && !strncasecmp(scanCurrent->value, statementTable[i].name, n)) {
            if (statementTable[i].id == -1) {
                utilError("%s starts with %s, which this BASIC doesn't have", scanCurrent->value, statementTable[i].name);
                return(-1);
            }
            splitName(statementTable[i].id, n);
            return(1);
        }
    }
    return(0);
}


int scanConsumeEol(void) {
    // A new line starts here, so it may report its own error.
    utilErrorReset();
    pendingLen = pendingPos = 0;
    nextChar();
    return(scanNext());
}


const char *scanDescribe(symbolType *s) {
    static char text[64];
    const char *t = NULL;
    switch (s->id) {
        case kwEol:
            return("end of line");
        case kwEof:
            return("end of input");
        case kwColon:
            t = ":";
            break;
        case kwSemicolon:
            t = ";";
            break;
        case kwComma:
            t = ",";
            break;
        case kwParenthesesOpen:
            t = "(";
            break;
        case kwParenthesesClose:
            t = ")";
            break;
        case kwOpAdd:
            t = "+";
            break;
        case kwOpSub:
            t = "-";
            break;
        case kwOpMul:
            t = "*";
            break;
        case kwOpDiv:
            t = "/";
            break;
        case kwOpExp:
            t = "^";
            break;
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
        case kwString:
            snprintf(text, sizeof(text), "\"%s\"", s->value);
            return(text);
        default:
            if (s->id < kwNumKeywords) {
                t = scanGetKeyword(s->id);
            } else if (s->value != NULL && s->value[0] != 0) {
                t = s->value;
            } else {
                return("input");
            }
            break;
    }
    snprintf(text, sizeof(text), "'%s'", t);
    return(text);
}


char *scanGetKeyword(keywords k) {
    if (k >= kwNumKeywords) {
        return(NULL);
    }
    return(idToKeywordTable[k]->value);
}


/// @brief Reads a comparison operator; returns it newly allocated, or NULL.
static char *scanGetLOp(void) {
    char *s = malloc(3);
    char *t = s;
    if (s == NULL) {
        aborts("memory allocation error");
    }
    switch (peekChar()) {
        case '<':
            *t++ = '<';
            nextChar();
            if (peekChar() == '=' || peekChar() == '>') {
                *t++ = peekChar();
                nextChar();
            }
            *t = 0;
            skipWhite();
            return(s);
        case '>':
            *t++ = '>';
            nextChar();
            if (peekChar() == '=') {
                *t++ = peekChar();
                nextChar();
            }
            *t = 0;
            skipWhite();
            return(s);
        case '=':
            *t++ = '=';
            *t = 0;
            nextChar();
            skipWhite();
            return(s);
    }
    if (s) {
        free(s);
    }
    return(NULL);
}


/// @brief Reads a name (a keyword or identifier); returns it newly allocated,
/// or NULL after reporting an error.
static char *scanGetName(void) {
    char *s = malloc(maxStringLen + 1);
    char *t = s;
    int i = maxStringLen;
    if (s == NULL) {
        aborts("memory allocation error");
    }
    if (!isalpha((unsigned char)peekChar())) {
        utilError("expected alphabetic character");
        free(s);
        return(NULL);
    }
    // The buffer holds maxStringLen + 1 bytes, so i reaching -1 means too long.
    while (i >= 0 && (isalnum((unsigned char)peekChar()) || peekChar() == '$')) {
        *t++ = peekChar();
        nextChar();
        i--;
    }
    if (i < 0) {
        utilError("name exceeds maximum length of %i", maxStringLen);
        free(s);
        return(NULL);
    }
    *t = 0;
    nameSpaced = isspace((unsigned char)peekChar()) && peekChar() != '\n';
    skipWhite();
    return(s);
}


/// @brief Reads a number (digits, a decimal point and an exponent); returns it
/// newly allocated, or NULL after reporting an error.
static char *scanGetNum(void) {
    char *s = malloc(maxStringLen + 1);
    char *t = s;
    int i = maxStringLen;
    if (s == NULL) {
        aborts("memory allocation error");
    }
    if (!isdigit((unsigned char)peekChar()) && peekChar() != '.') {
        utilError("expected digit");
        free(s);
        return(NULL);
    }
    int digits = 0;
    while (i >= 0 && (isdigit((unsigned char)peekChar()) || peekChar() == '.')) {
        digits += isdigit((unsigned char)peekChar()) != 0;
        *t++ = peekChar();
        nextChar();
        i--;
    }
    // An optional exponent (E, an optional sign, digits), only after a digit, so ". END" stays a REM.
    if (digits > 0 && i >= 0 && (peekChar() == 'e' || peekChar() == 'E')) {
        *t++ = peekChar();
        nextChar();
        i--;
        if (i >= 0 && (peekChar() == '+' || peekChar() == '-')) {
            *t++ = peekChar();
            nextChar();
            i--;
        }
        while (i >= 0 && isdigit((unsigned char)peekChar())) {
            *t++ = peekChar();
            nextChar();
            i--;
        }
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


/// @brief Reads an arithmetic operator; returns it newly allocated, or NULL
/// after reporting an error.
static char *scanGetOp(void) {
    char *s = malloc(2);
    char *t = s;
    if (s == NULL) {
        aborts("memory allocation error");
    }
    if (!isop(peekChar())) {
        utilError("expected operator");
        free(s);
        return(NULL);
    }
    // One character only, so 2*-3 is '*' then '-'.
    *t++ = peekChar();
    nextChar();
    *t = 0;
    skipWhite();
    return(s);
}


/// @brief Reads a string literal, without its quotes; returns it newly
/// allocated, or NULL after reporting an error.
static char *scanGetString(void) {
    char *s = malloc(maxStringLen + 1);
    char *t = s;
    int i = maxStringLen;
    if (s == NULL) {
        utilError("memory allocation error");
        return(NULL);
    }
    if (peekChar() != '"') {
        utilError("expected quote");
        free(s);
        return(NULL);
    }
    nextChar();
    while (i >= 0 && peekChar() != '"' && peekChar() != '\n' && peekChar() != IO_EOF) {
        *t++ = peekChar();
        nextChar();
        i--;
    }
    if (i < 0) {
        utilError("string exceeds maximum length of %i", maxStringLen);
        free(s);
        return(NULL);
    }
    // "The closing quotation can be omitted if it is the last character on a logical line."
    if (peekChar() == '\n' || peekChar() == IO_EOF) {
        *t = 0;
        return(s);
    }
    *t = 0;
    nextChar();
    skipWhite();
    return(s);
}


symbolType *scanGetText(void) {
    char *t = scanCurrent->value;
    int i = maxStringLen;
    while (i >= 0 && peekChar() != '\n' && peekChar() != IO_EOF) {
        *t++ = peekChar();
        nextChar();
        i--;
    }
    if (i < 0) {
        utilError("line exceeds maximum length of %i", maxStringLen);
        return(NULL);
    }
    *t = 0;
    scanCurrent->id = kwText;
    return(scanCurrent);
}


/// @brief Frees the current token, at exit.
static void scanExit(void) {
    if (scanCurrent != NULL) {
        free(scanCurrent);
    }
}


int scanInit(void) {
    long n = (long)(sizeof(keywordTable) / sizeof(symbolType));
    long l = n;
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
        for (l = 0; l < n; l++) {
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
    size_t l = 0;
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
    if (scanToken()) {
        scanCurrent->id = kwUnknown;
        return(1);
    }
    return(0);
}


void scanSkipLine(void) {
    pendingLen = pendingPos = 0;
    while (peekChar() != '\n' && peekChar() != IO_EOF) {
        nextChar();
    }
    scanCurrent->id = (peekChar() == IO_EOF) ? kwEof : kwEol;
    scanCurrent->value[0] = 0;
}


/// @brief Reads the next token into scanCurrent; returns 0, or 1 if no token
/// can be read (with the offending character, if any, as the value).
static int scanToken(void) {
    char *s = NULL;
    int i;
    skipWhite();
    tokenColumn = ioColumn();
    scanCurrent->value[0] = 0;
    if (peekChar() == IO_EOF) {
        scanCurrent->id = kwEof;
        scanCurrent->value[0] = 0;
        return(0);
    } else if (peekChar() == '\n') {
        scanCurrent->id = kwEol;
        scanCurrent->value[0] = 0;
        skipWhite();
        return(0);
    } else if (peekChar() == '(') {
        scanCurrent->id = kwParenthesesOpen;
        scanCurrent->value[0] = 0;
        nextChar();
        skipWhite();
        return(0);
    } else if (peekChar() == ')') {
        scanCurrent->id = kwParenthesesClose;
        scanCurrent->value[0] = 0;
        nextChar();
        skipWhite();
        return(0);
    } else if (peekChar() == ',') {
        scanCurrent->id = kwComma;
        scanCurrent->value[0] = 0;
        nextChar();
        skipWhite();
        return(0);
    } else if (peekChar() == ';') {
        scanCurrent->id = kwSemicolon;
        scanCurrent->value[0] = 0;
        nextChar();
        skipWhite();
        return(0);
    } else if (peekChar() == ':') {
        scanCurrent->id = kwColon;
        scanCurrent->value[0] = 0;
        nextChar();
        skipWhite();
        return(0);
    } else if (peekChar() == '?') {
        // ? is short for PRINT.
        scanCurrent->id = kwPRINT;
        nextChar();
        skipWhite();
        return(0);
    } else if (isalpha((unsigned char)peekChar())) {
        if ((s = scanGetName()) != NULL) {
            if (peekChar() == '.') {
                // An abbreviated statement, e.g. PR. for PRINT.
                nextChar();
                skipWhite();
                i = lookupAbbreviation(s);
                if (i == -1) {
                    free(s);
                    return(1);
                }
                scanCurrent->id = i;
                free(s);
                return(0);
            }
            i = lookup(keywordTree, s);
            if (i == -1 && strlen(s) > MAX_NAME_LEN) {
                utilError("variable names can be at most %d characters", MAX_NAME_LEN);
                free(s);
                return(1);
            }
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
    } else if (isdigit((unsigned char)peekChar()) || peekChar() == '.') {
        if ((s = scanGetNum()) != NULL) {
            if (!strcmp(s, ".")) {
                // A lone period isn't a number; at the start of a statement it means REM.
                scanCurrent->id = kwREM;
                free(s);
                return(0);
            }
            scanCurrent->id = kwNumeric;
            strcpy(scanCurrent->value, s);
            free(s);
            return(0);
        }
        return(1);
    } else if (isop(peekChar())) {
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
    } else if (islop(peekChar())) {
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
    } else if (peekChar() == '"') {
        if ((s = scanGetString()) != NULL) {
            scanCurrent->id = kwString;
            strcpy(scanCurrent->value, s);
            free(s);
            return(0);
        }
        return(1);
    }
    // Not the start of any token: leave it for the error message.
    scanCurrent->value[0] = peekChar();
    scanCurrent->value[1] = 0;
    return(1);
}


symbolType *scanPeek(void) {
    return(scanCurrent);
}


/// @brief Skips spaces and tabs, but not the end of the line.
static void skipWhite(void) {
    while (isspace((unsigned char)peekChar()) && peekChar() != '\n') {
        nextChar();
    }
}
