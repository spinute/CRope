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

typedef struct rope_scan_leaf_tag *RopeScanLeaf;
RopeScanLeaf RopeScanLeafInit(Rope rope);
char *RopeScanLeafGetNext(RopeScanLeaf scan);
void RopeScanLeafFini(RopeScanLeaf scan);

typedef struct rope_scan_char_tag *RopeScanChar;
RopeScanChar RopeScanCharInit(Rope rope);
char RopeScanCharGetNext(RopeScanChar scan);
void RopeScanCharFini(RopeScanChar scan);
