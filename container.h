/*
 * container.h
 */

#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdlib.h>


/* STACK */

#define stackType(maxElems) \
struct { \
	void *stackData[maxElems]; \
	long int stackElems; \
	long int stackIndex; \
}
#define stackClear(stack) {(stack).stackIndex = -1;}
#define stackEmpty(stack) ((stack).stackIndex == -1)
#define stackFull(stack) ((stack).stackIndex == (stack).stackElems - 1)
#define stackInit(stack) {(stack).stackElems = sizeof((stack).stackData) / sizeof(void *); (stack).stackIndex = -1L;}
#define stackPeek(stack) ((stack).stackData[(stack).stackIndex])
#define stackPop(stack) {(stack).stackIndex--;}
#define stackPush(stack, v) {(stack).stackData[++(stack).stackIndex] = (v);}


#endif /* CONTAINER_H */
