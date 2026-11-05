/*
 * var.c
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "util.h"
#include "var.h"


/*
 * LOCAL CONSTANTS
 */

/* the largest DIM value (array subscript or string length), as on the Atari */
#define MAX_DIM 32767L

/* Atari error numbers (Reference Manual, Appendix B) */
#define ERR_MEMORY 2
#define ERR_DIM 9


/*
 * LOCAL DATA TYPES
 */

/*
 * A variable. kind says what it is:
 *  - SCALAR: a numeric variable used without a subscript; one value.
 *  - ARRAY: DIM A(dim1-1[,dim2-1]); dim1 x dim2 values, subscripts from 0.
 *  - STRING: DIM A$(maxLen); one value of at most maxLen characters.
 * Values that were never set are NULL and read as "0" (or "" for a string).
 */
typedef enum { SCALAR, ARRAY, STRING } variableKind;

typedef struct variableType {
    struct variableType *next;
    variableKind kind;
    long dim1;
    long dim2;
    long maxLen;
    char *name;
    char **value;
} variableType;

typedef struct dataType {
    struct dataType *next;
    long int lineNum;
    char *value;
} dataType;


/*
 * LOCAL DATA
 */

static variableType *varList = NULL;
static dataType *dataList = NULL;
static dataType *dataPtr = NULL;


/*
 * LOCAL FUNCTIONS
 */

static variableType *varCreate(const char *name, variableKind kind, long dim1, long dim2, long maxLen);
static variableType *varFind(const char *name);
static void varExit(void);
static void varFree(variableType *var);
static char **varSlot(const char *name, long i, long j, int create);


int varAppendData(char **list, long int lineNum) {
    char **p = list;
    dataType *dl = dataList;
    dataType *dt = NULL;
    /* skip lines whose data is already in the list (checking every node, including the last) */
    if (dl) {
        while (1) {
            if (dl->lineNum == lineNum) {
                return(0);
            }
            if (dl->next == NULL) {
                break;
            }
            dl = dl->next;
        }
    }
    while (*p) {
        dt = malloc(sizeof(dataType) + strlen(*p) + 1);
        if (dt == NULL) {
            utilErrorN(ERR_MEMORY, "memory allocation error");
            goto err;
        }
        if (dl == NULL) {
            dl = dataList = dataPtr = dt;
        } else {
            dl->next = dt;
            dl = dt;
        }
        dl->next = NULL;
        dl->value = (char *)dl + sizeof(dataType);
        dl->lineNum = lineNum;
        strcpy(dl->value, *p);
        p++;
    }
    return(0);
err:
    return(1);
}


void varClearAll(void) {
    dataType *dl = dataList;
    dataType *dt;
    varClearVariables();
    while (dl) {
        dt = dl->next;
        free(dl);
        dl = dt;
    }
    dataList = NULL;
    dataPtr = NULL;
}


void varClearVariables(void) {
    variableType *vl = varList;
    variableType *vt;
    while (vl) {
        vt = vl->next;
        varFree(vl);
        vl = vt;
    }
    varList = NULL;
}


/* a new variable with all values unset, added to the list; NULL after an error */
static variableType *varCreate(const char *name, variableKind kind, long dim1, long dim2, long maxLen) {
    variableType *var;
    size_t bytes = sizeof(variableType) + sizeof(char *) * (size_t)(dim1 * dim2);
    if ((var = malloc(bytes)) == NULL) {
        utilErrorN(ERR_MEMORY, "not enough memory for %s", name);
        return(NULL);
    }
    memset(var, 0, bytes);
    if ((var->name = strdup(name)) == NULL) {
        free(var);
        utilErrorN(ERR_MEMORY, "not enough memory for %s", name);
        return(NULL);
    }
    var->kind = kind;
    var->dim1 = dim1;
    var->dim2 = dim2;
    var->maxLen = maxLen;
    var->value = (char **)((char *)var + sizeof(variableType));
    var->next = varList;
    varList = var;
    return(var);
}


int varDim(const char *name, long dim1, long dim2) {
    variableType *var = varFind(name);
    if (var != NULL && var->kind != SCALAR) {
        utilErrorN(ERR_DIM, "%s is already dimensioned (CLR first)", name);
        return(1);
    }
    if (strchr(name, '$') != NULL) {
        /* DIM A$(n) is one string of up to n characters */
        if (dim2 != -1) {
            utilErrorN(ERR_DIM, "string %s has only one dimension, its length", name);
            return(1);
        }
        if (dim1 < 1 || dim1 > MAX_DIM) {
            utilErrorN(ERR_DIM, "string length must be 1 to %ld", MAX_DIM);
            return(1);
        }
        return(varCreate(name, STRING, 1, 1, dim1) == NULL);
    }
    /* DIM A(n) has subscripts 0 to n: n + 1 elements */
    if (dim1 < 0 || dim1 > MAX_DIM || dim2 < -1 || dim2 > MAX_DIM) {
        utilErrorN(ERR_DIM, "DIM size must be 0 to %ld", MAX_DIM);
        return(1);
    }
    if (var != NULL) {
        /* a scalar of the same name is replaced by the array */
        variableType **pp = &varList;
        while (*pp != var) {
            pp = &(*pp)->next;
        }
        *pp = var->next;
        varFree(var);
    }
    /* (32767 + 1)^2 pointers fits in size_t on any machine this builds on */
    return(varCreate(name, ARRAY, dim1 + 1, dim2 == -1 ? 1 : dim2 + 1, 0) == NULL);
}


static void varExit(void) {
    varClearAll();
}


static variableType *varFind(const char *name) {
    variableType *var;
    for (var = varList; var != NULL; var = var->next) {
        if (!strcasecmp(var->name, name)) {
            return(var);
        }
    }
    return(NULL);
}


/* free a variable, its name and its values */
static void varFree(variableType *var) {
    long i;
    for (i = 0L; i < var->dim1 * var->dim2; i++) {
        free(var->value[i]);
    }
    free(var->name);
    free(var);
}


char *varGetValue(const char *name, long i, long j) {
    char **slot;
    char *s;
    if (varFind(name) == NULL && strchr(name, '$') == NULL && i == -1) {
        /* a numeric scalar that was never set */
        s = strdup("0");
    } else if ((slot = varSlot(name, i, j, 0)) == NULL) {
        return(NULL);
    } else if (*slot == NULL) {
        s = strdup(strchr(name, '$') != NULL ? "" : "0");
    } else {
        s = strdup(*slot);
    }
    if (s == NULL) {
        utilErrorN(ERR_MEMORY, "memory allocation error");
    }
    return(s);
}


int varInit(void) {
    atexit(varExit);
    return(0);
}


char *varReadData(void) {
    char *s;
    if (dataPtr == NULL) {
        return(NULL);
    }
    s = strdup(dataPtr->value);
    dataPtr = dataPtr->next;
    return(s);
}


int varRestoreData(long int lineNum) {
    if (lineNum == -1) {
        dataPtr = dataList;
        return(0);
    }
    /* the first data item on or after the given line (data is kept in line order) */
    dataType *d = dataList;
    while (d != NULL && d->lineNum < lineNum) {
        d = d->next;
    }
    dataPtr = d;
    return(0);
}


int varSetValue(const char *name, const char *value, long i, long j) {
    char **slot;
    char *s;
    variableType *var;
    if (name == NULL || name[0] == 0) {
        return(1);
    }
    if (value == NULL) {
        value = "";
    }
    if ((slot = varSlot(name, i, j, 1)) == NULL) {
        return(1);
    }
    /* a string is cut to its DIM size */
    var = varFind(name);
    s = var->kind == STRING ? strndup(value, (size_t)var->maxLen) : strdup(value);
    if (s == NULL) {
        utilErrorN(ERR_MEMORY, "memory allocation error");
        return(1);
    }
    free(*slot);
    *slot = s;
    return(0);
}


/*
 * Where the value of name, or of name(i,j), is stored. With create, an
 * unknown numeric scalar is created. Reports an error and returns NULL if the
 * reference isn't valid for the variable.
 */
static char **varSlot(const char *name, long i, long j, int create) {
    variableType *var = varFind(name);
    if (strchr(name, '$') != NULL) {
        if (var == NULL) {
            utilErrorN(ERR_DIM, "%s is not dimensioned", name);
            return(NULL);
        }
        return(&var->value[0]);
    }
    if (i == -1) {
        /* no subscript: a scalar */
        if (var == NULL) {
            if (!create || (var = varCreate(name, SCALAR, 1, 1, 0)) == NULL) {
                return(NULL);
            }
        } else if (var->kind == ARRAY) {
            utilErrorN(ERR_DIM, "%s is an array; it needs a subscript", name);
            return(NULL);
        }
        return(&var->value[0]);
    }
    if (var == NULL || var->kind != ARRAY) {
        utilErrorN(ERR_DIM, "%s is not dimensioned", name);
        return(NULL);
    }
    /* A(i) on a two-dimensional array means A(i,0) */
    if (j == -1) {
        j = 0;
    }
    if (i < 0 || i >= var->dim1 || j < 0 || j >= var->dim2) {
        utilErrorN(ERR_DIM, "subscript out of range for %s", name);
        return(NULL);
    }
    return(&var->value[j * var->dim1 + i]);
}


long varStringMax(const char *name) {
    variableType *var = varFind(name);
    if (var == NULL || var->kind != STRING) {
        utilErrorN(ERR_DIM, "%s is not dimensioned", name);
        return(-1);
    }
    return(var->maxLen);
}
