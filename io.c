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
/* how much of the current line is kept for error messages */
#define MAX_LINE_LEN 1024


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
    /* the current line so far, up to and including peek (for error messages) */
    char line[MAX_LINE_LEN];
    size_t lineLen;
} ioType;


/*
 * LOCAL DATA
 */

static ioType *fileStack = NULL;


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
    ssize_t n;
    if (fileStack == NULL) {
        return(IO_EOF);
    }
    if (fileStack->len == 0) {
        /* read() returns -1 on error; len is unsigned, so test before storing */
        n = read(fileStack->fh, fileStack->buffer, MAX_BUFFER_LEN);
        if (n < 0) {
            return(IO_EOF);
        }
        fileStack->len = (unsigned long int)n;
        if (fileStack->len == 0) {
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
    /* a new line starts after a newline */
    if (fileStack->peek == '\n') {
        fileStack->lineLen = 0;
    }
    fileStack->peek = *fileStack->ptr++;
    fileStack->len--;
    if (fileStack->lineLen < MAX_LINE_LEN) {
        fileStack->line[fileStack->lineLen] = fileStack->peek;
    }
    fileStack->lineLen++;
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
    /*
     * Nothing has been read yet. Act as if the stream were at the end of a
     * line: if another file is opened on top of this one before it is read
     * (abasic file.bas), returning to it then looks like a fresh line rather
     * than end of input.
     */
    iop->peek = '\n';
    iop->lineLen = 0;
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


long ioColumn(void) {
    if (fileStack == NULL) {
        return(-1);
    }
    return((long)fileStack->lineLen - 1);
}


const char *ioLineText(size_t *len) {
    if (fileStack == NULL) {
        *len = 0;
        return(NULL);
    }
    *len = fileStack->lineLen < MAX_LINE_LEN ? fileStack->lineLen : MAX_LINE_LEN;
    return(fileStack->line);
}


char *ioReadLine(void) {
    ioType *iop = fileStack;
    char *line;
    char *bigger;
    char c;
    ssize_t n;
    size_t cap = 128;
    size_t len = 0;
    /*
     * INPUT always reads the terminal, even while a LOADed file is being
     * read, so find standard input in the stack.
     */
    while (iop != NULL && iop->fh != 0) {
        iop = iop->next;
    }
    if (iop == NULL || (line = malloc(cap)) == NULL) {
        return(NULL);
    }
    while (1) {
        if (iop->len == 0) {
            n = read(iop->fh, iop->buffer, MAX_BUFFER_LEN);
            if (n <= 0) {
                if (len == 0) {
                    free(line);
                    return(NULL);
                }
                break;
            }
            iop->len = (unsigned long int)n;
            iop->ptr = iop->buffer;
        }
        /* take characters after the scanner's current one (iop->peek) */
        c = *iop->ptr++;
        iop->len--;
        if (c == '\n') {
            break;
        }
        if (len + 1 >= cap) {
            cap <<= 1;
            if ((bigger = realloc(line, cap)) == NULL) {
                free(line);
                return(NULL);
            }
            line = bigger;
        }
        line[len++] = c;
    }
    line[len] = 0;
    return(line);
}
