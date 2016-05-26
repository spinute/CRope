#pragma once

#include <stddef.h>

typedef struct rope_tag *Rope;

Rope RopeCreate(char str[], size_t size);
void RopeDestroy(Rope rope);

size_t RopeGetLen(Rope rope);
void RopeDump(Rope rope);
/* return the size of a written string, or -1 if buf_size is not sufficient */
int RopeToString(Rope rope, char *ret_buf, size_t buf_size);

Rope RopeConcat(Rope left, Rope right);
Rope RopeSubstr(Rope rope, size_t i, size_t n);
char RopeIndex(Rope rope, size_t i);

typedef struct rope_scan_tag *RopeScan;
RopeScan RopeScanInit(Rope rope);
char RopeScanGetNext(RopeScan scan);
void RopeScanFini(RopeScan scan);
