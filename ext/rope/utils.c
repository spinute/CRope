#include "utils.h"
#include <stddef.h>
#include <stdlib.h>

void *
palloc(size_t size) {
	void *ptr = malloc(size);
	if (!ptr)
		elog("malloc failed");

	return ptr;
}

void
pfree(void *ptr) {
	if (!ptr)
		elog("free: empty ptr");
	free(ptr);
}
