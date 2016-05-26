#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

void
elog(char *str)
{
	printf("elog: %s\n", str);
	exit(-1);
}

void *
palloc(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr)
		elog("malloc failed");

	return ptr;
}

void
pfree(void *ptr)
{
	if (!ptr)
		elog("free: empty ptr");
	free(ptr);
}
