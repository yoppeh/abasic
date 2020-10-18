/*
 * var.h
 *
 * Handles variable allocations, assignments, setting, retrieving.
 */

#ifndef VAR_H
#define VAR_H

#include <stdarg.h>

#include "scan.h"


/*
 * GLOBAL FUNCTIONS
 */

extern int varAppendData(char **d, int lineNum);

extern void varClearAll(void);

extern int varDim(const char *name, long dim1, long dim2);

extern char *varGetValue(const char *name, long dim1, long dim2);

extern int varInit(void);

extern char *varReadData(void);

extern int varRestoreData(long int lineNum);

extern int varSetValue(const char *name, const char *value, long dim1, long dim2);


#endif /* VAR_H */
