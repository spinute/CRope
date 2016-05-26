#pragma once

#include <stddef.h>

void elog(char *str);
void *palloc(size_t size);
void pfree(void *ptr);
