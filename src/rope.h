#pragma once

#include <stddef.h>

typedef struct rope_tag *Rope;

Rope RopeCreate(char str[], size_t size);
void RopeDestroy(Rope rope);

void RopeToString(Rope rope);

Rope RopeConcat(Rope left, Rope right);
