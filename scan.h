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
	kwSubExpression
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

extern char *scanGetKeyword(keywords k);

extern symbolType *scanGetText(void);

extern int scanInit(void);

extern symbolType *scanNewSymbol(keywords k, char *v);

extern int scanNext(void);

extern symbolType *scanPeek(void);


#endif /* SCAN_H */
