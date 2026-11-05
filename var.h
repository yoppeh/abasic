/*
 * var.h
 *
 * Handles variable allocations, assignments, setting, retrieving, and the
 * DATA list.
 *
 * Variables follow Atari BASIC (Reference Manual, sections 7 and 8):
 *  - A numeric variable used without a subscript is a scalar. It needn't be
 *    declared, and reads as 0 until set.
 *  - A numeric array must be DIMensioned first. DIM A(10) gives subscripts 0
 *    to 10; DIM M(6,3) gives 0-6 by 0-3.
 *  - A string variable (name ending in '$') must be DIMensioned first. DIM
 *    A$(20) makes one string of up to 20 characters (substrings are handled
 *    by the caller).
 *  - A name can't be DIMensioned twice (CLR first).
 * Errors are reported with utilErrorN and the Atari error number.
 *
 * Subscripts are passed as 0-based indexes; -1 means "no subscript".
 */

#ifndef VAR_H
#define VAR_H


/*
 * GLOBAL FUNCTIONS
 */

extern int varAppendData(char **d, long int lineNum);

/* forget all variables and the DATA list (RUN) */
extern void varClearAll(void);

/* forget all variables, keeping the DATA list (CLR) */
extern void varClearVariables(void);

/*
 * DIM name(dim1[,dim2]): the largest subscripts (dim2 -1 if not given), or
 * for a string the maximum length (dim2 must be -1).
 */
extern int varDim(const char *name, long dim1, long dim2);

extern char *varGetValue(const char *name, long i, long j);

/* the DIM size of string variable name, or -1 (after an error) if it has none */
extern long varStringMax(const char *name);

extern int varInit(void);

extern char *varReadData(void);

extern int varRestoreData(long int lineNum);

extern int varSetValue(const char *name, const char *value, long i, long j);


#endif /* VAR_H */
