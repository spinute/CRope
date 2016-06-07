#pragma once

#include <stddef.h>

typedef struct rope_tag *Rope;

Rope RopeCreate(char str[], size_t size);
void RopeDestroy(Rope rope);

/* return the size of a written string, or -1 if buf_size is not sufficient */
int RopeToString(const Rope rope, char *ret_buf, size_t buf_size);
void RopeDump(const Rope rope);
size_t RopeGetLen(const Rope rope);
size_t RopeGetSize(const Rope rope);

Rope RopeConcat(const Rope left, const Rope right);
Rope RopeSubstr(const Rope rope, size_t i, size_t n);
Rope RopeDelete(const Rope rope, size_t i, size_t n);
char RopeIndex(const Rope rope, size_t i);

typedef struct rope_scan_leaf_tag *RopeScanLeaf;
RopeScanLeaf RopeScanLeafInit(const Rope rope);
char *RopeScanLeafGetNext(RopeScanLeaf scan);
void RopeScanLeafFini(RopeScanLeaf scan);

typedef struct rope_scan_char_tag *RopeScanChar;
RopeScanChar RopeScanCharInit(const Rope rope);
RopeScanChar RopeScanCharInitIndex(const Rope rope, size_t i);
char RopeScanCharGetNext(RopeScanChar scan);
void RopeScanCharFini(RopeScanChar scan);
