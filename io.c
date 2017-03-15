/*
 * io.c
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "io.h"
#include "util.h"


/*
 * LOCAL CONSTANTS
 */

#define MAX_BUFFER_LEN 1024


/*
 * LOCAL DATA TYPES
 */

typedef struct ioType {
	struct ioType *next;
	char *buffer;
	char *ptr;
	unsigned long int len;
	int fh;
	char peek;
} ioType;


/*
 * LOCAL DATA
 */

static ioType *fileStack = NULL;

static int fileHandle = -1;
static char *buffer = NULL;
static char *buffPtr = NULL;
static int buffLen = 0;


/*
 * LOCAL FUNCTIONS
 */

static void ioExit(void);


int ioCloseInput(void) {
	if (fileStack) {
		ioType *iop = fileStack->next;
		if (fileStack->buffer != NULL) {
			free(fileStack->buffer);
		}
		if (fileStack->fh > 1) {
			close(fileStack->fh);
		}
		free(fileStack);
		fileStack = iop;
	}
	return(0);
}


static void ioExit(void) {
	ioType *iop = fileStack;
	ioType *ion;
	while (iop) {
		ion = iop->next;
		if (iop->fh > 1) {
			close(iop->fh);
		}
		if (iop->buffer != NULL) {
			free(iop->buffer);
		}
		free(iop);
		iop = ion;
	}
	fileStack = NULL;
}


int ioInit(void) {
	atexit(ioExit);
	return(0);
}


char ioNext(void) {
	unsigned long int len;
	if (fileStack == NULL) {
		return(IO_EOF);
	}
	if (fileStack->len == 0) {
		fileStack->len = read(fileStack->fh, fileStack->buffer, MAX_BUFFER_LEN);
		if (fileStack->len < 0) {
			return(IO_EOF);
		} else if (fileStack->len == 0) {
			ioCloseInput();
			if (fileStack != NULL) {
				return(ioPeek());
			} else {
				return(IO_EOF);
			}
		} else {
			fileStack->ptr = fileStack->buffer;
		}
	}
	fileStack->peek = *fileStack->ptr++;
	fileStack->len--;
	return(fileStack->peek);
}


int ioOpenInput(char *fn) {
	ioType *iop = NULL;
	char *buffer = NULL;
	int fh = -1;
	if (fn == NULL) {
		fn = "stdin";
		fh = 0;
	} else if ((fh = open(fn, O_RDONLY)) < 0) {
		goto err;
	}
	if ((buffer = malloc(MAX_BUFFER_LEN)) == NULL) {
		goto err;
	}
	if ((iop = malloc(sizeof(ioType))) == NULL) {
		goto err;
	}
	iop->next = fileStack;
	iop->buffer = buffer;
	iop->ptr = buffer;
	iop->len = 0;
	iop->fh = fh;
	iop->peek = IO_EOF;
	fileStack = iop;
	return(0);
err:
	if (iop != NULL) {
		free(iop);
	}
	if (buffer != NULL) {
		free(buffer);
	}
	if (fh > 1) {
		close(fh);
	}
	return(1);
}


char ioPeek(void) {
	if (fileStack == NULL) {
		return(IO_EOF);
	}
	return(fileStack->peek);
}
