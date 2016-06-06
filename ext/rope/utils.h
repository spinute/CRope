#pragma once

#include <stddef.h>
#include <stdio.h>

#define elog(str) printf("elog(%s): %s\n", __func__, (str))

void *palloc(size_t size);
void pfree(void *ptr);
