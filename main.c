/**
 * @file main.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief The abasic interpreter's entry point.
 * @details Usage: abasic [file.bas]. With a file, it is read first, as if
 * LOADed, then the terminal.
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "parse.h"
#include "prog.h"
#include "scan.h"
#include "util.h"
#include "var.h"

static int init(void);

int main(int ac, char **av) {
    if (init()) {
        aborts("initialization failed");
    }
    if (ioOpenInput(NULL)) {
        aborts("couldn't read standard input");
    }
    // abasic file.bas: read the file first (like LOAD), then the terminal.
    if (ac > 1 && ioOpenInput(av[1])) {
        aborts("couldn't open [%s]", av[1]);
    }
    utilReady();
    ioNext();
    parseProgram();
    return(0);
}

/// @brief Initializes every module; returns 0, or nonzero if one failed.
static int init(void) {
    int rc;
    rc = ioInit();
    rc |= progInit();
    rc |= scanInit();
    rc |= varInit();
    return(rc);
}
