/**
 * @file scan.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief The scanner: splits input into tokens.
 * @details The current token is kept in one shared symbolType, which
 * scanNext() overwrites. Tokens are also the nodes of expression trees, made
 * with scanNewSymbol().
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#ifndef SCAN_H
#define SCAN_H

/**
 * @brief Kinds of tokens and of expression tree nodes.
 * @details Up to kwTRAP, the values are statements (and the words THEN, TO
 * and STEP); up to kwNumKeywords, they are keywords, with names from
 * scanGetKeyword(). The rest are other tokens, then node kinds the parser
 * makes. kwUnknown is input the scanner couldn't make a token of.
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
    kwUnknown
} keywords;

/**
 * @brief A token, or a node of an expression tree.
 * @details value is the text of an identifier, number or string. In a tree,
 * l and r are the operands; an identifier's are its subscripts. A comparison
 * between strings has the value "$", so it compares text rather than numbers.
 */
typedef struct symbolType {
    struct symbolType *l;
    struct symbolType *r;
    char *value;
    keywords id;
} symbolType;

/**
 * @brief Moves past the end of the current line and reads the first token of the next.
 * @details Starts a new unit of error reporting (see utilErrorReset()).
 * @return As scanNext().
 */
extern int scanConsumeEol(void);

/**
 * @brief Describes a token as the user would recognize it, for error messages.
 * @details For example ')', 'then', "abc" or end of line.
 * @param s The token.
 * @return A static buffer, overwritten by the next call.
 */
extern const char *scanDescribe(symbolType *s);

/**
 * @brief Returns a keyword's name, in lower case.
 * @param k The keyword.
 * @return The name, or NULL if k isn't a keyword.
 */
extern char *scanGetKeyword(keywords k);

/**
 * @brief Reads the rest of the line as one kwText token, for DATA and REM.
 * @return The current token, or NULL after reporting that the line is too long.
 */
extern symbolType *scanGetText(void);

/**
 * @brief Initializes the module.
 * @details Builds the keyword lookup tree and the current token.
 * @return 0, or 1 if memory runs out.
 */
extern int scanInit(void);

/**
 * @brief Makes a new token or expression tree node.
 * @param k Its kind.
 * @param v Its value, which is copied, or NULL for none.
 * @return The node, or NULL after reporting that memory ran out.
 */
extern symbolType *scanNewSymbol(keywords k, char *v);

/**
 * @brief Reads the next token into the current token.
 * @details If no token can be read, the token is kwUnknown, with the
 * offending character as its value, so the parser reports an error rather
 * than reusing the previous token.
 * @return 0, or 1 for kwUnknown.
 */
extern int scanNext(void);

/**
 * @brief Returns the current token.
 * @return The shared token, which the next scanNext() overwrites.
 */
extern symbolType *scanPeek(void);

/**
 * @brief Discards the rest of the current line, after a syntax error.
 * @details The current token becomes kwEol (or kwEof).
 */
extern void scanSkipLine(void);

/**
 * @brief Splits a keyword off the front of the current token, if it's a name
 * that starts with it.
 * @details Atari BASIC recognizes a keyword wherever the grammar expects one,
 * even run together with what follows, so where a TO is expected, the name
 * TO10 is TO followed by 10. The rest of the name is scanned next.
 * @param k The keyword the parser expects.
 * @return 1 if the current token is now k, 0 if it wasn't split.
 */
extern int scanSplitKeyword(keywords k);

/**
 * @brief Splits a statement keyword off the front of the current token, if
 * it's a name that starts with one.
 * @details At the start of a statement, Atari BASIC takes the first statement
 * in its table whose name starts the text, so PRINTX is PRINT X and LETTER=1
 * is LET TER=1. A name starting with an Atari statement this BASIC doesn't
 * have (COLORS=1 is COLOR S=1) is reported as an error.
 * @return 1 if the current token is now that statement, 0 if it wasn't split,
 * or -1 after reporting an error.
 */
extern int scanSplitStatement(void);

/**
 * @brief Returns where the current token starts in its input line.
 * @return The 0-based column, for showing where a syntax error is.
 */
extern long scanTokenColumn(void);

#endif // SCAN_H
