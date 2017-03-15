/*
 * main.c
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


/*
 * LOCAL FUNCTIONS
 */

static int init(void);


int main(int ac, char **av) {
	utilReady();
	init();
	ioOpenInput(NULL);
	ioNext();
	parseProgram();
	return(0);
}


static int init(void) {
	int rc;
	rc = ioInit();
	rc |= progInit();
	rc |= scanInit();
	return(rc);
}
