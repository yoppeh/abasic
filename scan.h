/*
 * scan.h
 */

#ifndef SCAN_H
#define SCAN_H


/*
 * GLOBAL DATA TYPES
 */

typedef enum {
    kwBYE,
    kwCLR,
    kwCLS,
    kwCONT,
    kwDATA,
    kwDEG,
    kwDIM,
    kwEND,
    kwFOR,
    kwGOSUB,
    kwGOTO,
    kwIF,
    kwTHEN,
    kwINPUT,
    kwLET,
    kwLIST,
    kwLOAD,
    kwNEW,
    kwNEXT,
    kwON,
    kwPOP,
    kwPRINT,
    kwRAD,
    kwREAD,
    kwREM,
    kwRESTORE,
    kwRETURN,
    kwRUN,
    kwSAVE,
    kwSTEP,
    kwSTOP,
    kwTO,
    kwTRAP,
    kwABS,
    kwASC,
    kwATN,
    kwCLOG,
    kwCOS,
    kwEXP,
    kwINT,
    kwLEN,
    kwLOG,
    kwRND,
    kwSGN,
    kwSIN,
    kwSQR,
    kwVAL,
    kwCHR,
    kwSTR,
    kwERL,
    kwERR,
    kwPEEK,
    kwOR,
    kwAND,
    kwNOT,
    kwNumKeywords,
    kwEol,
    kwEof,
    kwColon,
    kwSemicolon,
    kwIdentifier,
    kwNumeric,
    kwOpAdd,
    kwOpSub,
    kwOpMul,
    kwOpDiv,
    kwOpExp,
    kwString,
    kwComma,
    kwParenthesesOpen,
    kwParenthesesClose,
    kwLogicalLT,
    kwLogicalLTE,
    kwLogicalEQ,
    kwLogicalNE,
    kwLogicalGT,
    kwLogicalGTE,
    kwText,
    kwDataString,
    kwAssignment,
    kwSignPlus,
    kwSignMinus,
    kwSubExpression,
    kwUnknown           /* input the scanner couldn't make a token of */
} keywords;

typedef struct symbolType {
    struct symbolType *l;
    struct symbolType *r;
    char *value;
    keywords id;
} symbolType;


/*
 * GLOBAL FUNCTIONS
 */

extern int scanConsumeEol(void);

/*
 * A token as the user would recognize it in an error message: ')', 'then',
 * "abc", end of line, ... Returns a static buffer.
 */
extern const char *scanDescribe(symbolType *s);

extern char *scanGetKeyword(keywords k);

extern symbolType *scanGetText(void);

extern int scanInit(void);

extern symbolType *scanNewSymbol(keywords k, char *v);

extern int scanNext(void);

extern symbolType *scanPeek(void);

/* discard the rest of the current line (after a syntax error) */
extern void scanSkipLine(void);

/* column (0-based) in the current input line where the current token starts */
extern long scanTokenColumn(void);


#endif /* SCAN_H */
