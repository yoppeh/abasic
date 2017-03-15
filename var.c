/*
 * var.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scan.h"
#include "util.h"
#include "var.h"


/*
 * LOCAL DATA TYPES
 */

typedef struct variableType {
	struct variableType *next;
	long dim1;
	long dim2;
	char *name;
	char **value;
} variableType;

typedef struct dataType {
	struct dataType *next;
	int lineNum;
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

static void varExit(void);


int varAppendData(char **list, int lineNum) {
	char **p = list;
	dataType *dl = dataList;
	dataType *dt = NULL;
	if (dl) {
		while (dl->next) {
			if (dl->lineNum == lineNum) {
				return(0);
			}
			dl = dl->next;
		}
	}
	while (*p) {
		dt = malloc(sizeof(dataType) + strlen(*p) + 1);
		if (dt == NULL) {
			utilError("memory allocation error");
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


static void varExit(void) {
	varClearAll();
}


void varClearAll(void) {
	variableType *vl = varList;
	variableType *vt;
	long i;
	while (vl) {
		vt = vl->next;
		if (vl->name != NULL) {
			free(vl->name);
		}
		if (vl->value != NULL) {
			for (i = 0L; i < vl->dim1 * vl->dim2; i++) {
				if (vl->value[i] != NULL) {
					free(vl->value[i]);
				}
			}
		}
		free(vl);
		vl = vt;
	}
	varList = NULL;
	dataType *dl = dataList;
	dataType *dt;
	while (dl) {
		dt = dl->next;
		free(dl);
		dl = dt;
	}
	dataList = NULL;
	dataPtr = NULL;
}


int varDim(const char *name, long dim1, long dim2) {
	if (dim1 < 1) {
		dim1 = 1;
	}
	if (dim2 < 1) {
		dim2 = 1;
	}
	variableType *var = malloc(sizeof(variableType) + (sizeof(char *) * (dim1 * dim2)));
	if (var == NULL) {
		return(1);
	}
	memset(var, 0, sizeof(variableType) + (sizeof(char *) * (dim1 * dim2)));
	var->next = varList;
	var->name = strdup(name);
	var->value = (char **)((char *)var + sizeof(variableType));
	var->dim1 = dim1;
	var->dim2 = dim2;
	varList = var;
	return(0);
}


char *varGetValue(const char *name, long dim1, long dim2) {
	variableType *var = varList;
	long i;
	dim1--;
	dim2--;
	while (var) {
		if (!strcmp(var->name, name)) {
			if (dim1 >= var->dim1 || dim2 >= var->dim2) {
				utilError("dimensions out of bounds");
				return(NULL);
			}
			if (var->value != NULL) {
				i = dim2 * var->dim1 + dim1;
				return(strdup(var->value[i]));
			} else {
				return(NULL);
			}
		}
		var = var->next;
	}
	return(NULL);
}


int varInit(void) {
	atexit(varExit);
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
	dataType *d = dataList;
	while (d) {
		if (d->lineNum == lineNum) {
			dataPtr = d;
			return(0);
		}
	}
	return(1);
}


int varSetValue(const char *name, const char *value, long dim1, long dim2) {
	variableType *var = varList;
	long i;
	if (name == NULL) {
		return(1);
	}
	if (name[0] == 0) {
		return(1);
	}
	if (value == NULL) {
		value = "";
	}
	dim1--;
	dim2--;
	while (var) {
		if (!strcmp(var->name, name)) {
			if (dim1 >= var->dim1 || dim2 >= var->dim2) {
				utilError("dimensions out of bounds");
				return(1);
			}
			if (var->value) {
				i = dim2 * var->dim1 + dim1;
				if (var->value[i]) {
					free(var->value[i]);
					var->value[i] = NULL;
				}
				var->value[i] = strdup(value);
				if (var->value[i] == NULL) {
					return(1);
				}
				return(0);
			}
		}
		var = var->next;
	}
	if (dim1 > 1 || dim2 > 1) {
		utilError("dimensions out of bounds");
		return(1);
	}
	if ((var = malloc(sizeof(variableType) + sizeof(char *))) == NULL) {
		return(1);
	}
	var->next = varList;
	var->name = strdup(name);
	var->value = (char **)((char *)var + sizeof(variableType)); 
	var->value[0] = strdup(value);
	var->dim1 = 1;
	var->dim2 = 1;
	if (var->name == NULL || var->value[0] == NULL) {
		free(var);
		return(1);
	}
	varList = var;
	return(0);
}

