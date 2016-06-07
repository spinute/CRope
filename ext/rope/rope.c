#include "rope.h"
#include "utils.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* TODO: It may be possible to fix this value by wordsize if balanced */
#define ROPE_SCAN_MAX_DEPTH 64

struct rope_tag {
	size_t len; /* w/o NUL */
	bool is_leaf;
	int ref_count;
	Rope left, right;
	char str[];
};

static Rope
rope_ref(Rope rope) {
	assert(rope->ref_count > 0);

	if (rope->ref_count == INT_MAX) {
		elog("ref_count reaches INT_MAX");
		return NULL;
	}

	rope->ref_count++;

	if (rope->left)
		rope_ref(rope->left);
	if (rope->right)
		rope_ref(rope->right);

	return rope;
}

static void
rope_deref(Rope rope) {
	assert(rope->ref_count > 0);

	rope->ref_count--;
	if (rope->left)
		rope_deref(rope->left);
	if (rope->right)
		rope_deref(rope->right);

	if (rope->ref_count == 0)
		pfree(rope);
}

Rope
RopeConcat(const Rope left, const Rope right) {
	Rope rope = palloc(sizeof(*rope));

	rope->is_leaf = false;
	rope->ref_count = 1;
	rope->len = 0;
	if (left)
		rope->len += left->len;
	if (right)
		rope->len += right->len;
	rope->left = rope_ref(left);
	rope->right = rope_ref(right);

	return rope;
}

Rope
RopeCreate(char *str, size_t len) {
	Rope rope = palloc(sizeof(*rope) + len + 1);

	rope->is_leaf = true;
	rope->ref_count = 1;
	rope->len = len;
	rope->left = rope->right = NULL;
	memcpy(rope->str, str, len);
	rope->str[len] = '\0';

	return rope;
}

void
RopeDestroy(Rope rope) {
	assert(rope);
	rope_deref(rope);
}

static void
rope_dump(const Rope rope, int level) {
	for (int i = 0; i < level; i++)
		printf("  ");
	printf("| ");

	if (rope->is_leaf)
		printf("Leaf: len=%zu, str=%s, refcount=%d\n", rope->len, rope->str,
		       rope->ref_count);
	else {
		printf("Concat: len=%zu, refcount=%d\n", rope->len, rope->ref_count);
		rope_dump(rope->left, level + 1);
		rope_dump(rope->right, level + 1);
	}
}

void
RopeDump(const Rope rope) {
	assert(rope);
	rope_dump(rope, 0);
}

static int
rope_collect_cstr(const Rope rope, char *ret_buf, int i) {
	if (rope->is_leaf) {
		memcpy(ret_buf + i, rope->str, rope->len);
		return i + rope->len;
	}

	i = rope_collect_cstr(rope->left, ret_buf, i);
	return rope_collect_cstr(rope->right, ret_buf, i);
}

int
RopeToString(const Rope rope, char *ret_buf, size_t buf_size) {
	int rv;

	if (rope->len > buf_size - 1)
		return -1;

	rv = rope_collect_cstr(rope, ret_buf, 0);
	ret_buf[rope->len] = '\0';

	return rv;
}

size_t
RopeGetLen(const Rope rope) {
	assert(rope);
	return rope->len;
}

size_t
RopeGetSize(const Rope rope) {
	return offsetof(struct rope_tag, str) + rope->len;
}

static Rope
rope_concat_without_rec_ref(const Rope left, const Rope right) {
	Rope rope = palloc(sizeof(*rope));

	rope->is_leaf = false;
	rope->ref_count = 1;
	rope->len = 0;
	if (left)
		rope->len += left->len;
	if (right)
		rope->len += right->len;
	rope->left = left;
	rope->right = right;

	return rope;
}

static Rope
rope_get_substr(const Rope rope, size_t i, size_t n) {
	if (rope->is_leaf) {
		if (n == rope->len)
			return rope_ref(rope);

		return RopeCreate(((char *) rope->str) + i, n);
	} else {
		size_t llen = rope->left->len;

		if (llen >= i + n)
			return rope_get_substr(rope->left, i, n);
		else if (llen <= i)
			return rope_get_substr(rope->right, i - llen, n);
		else
			return rope_concat_without_rec_ref(
			    rope_get_substr(rope->left, i, llen - i),
			    rope_get_substr(rope->right, 0, n - llen + i));
	}
}

Rope
RopeSubstr(const Rope rope, size_t i, size_t n) {
	assert(rope);
	assert(i + n <= rope->len);
	assert(i >= 0);

	return rope_get_substr(rope, i, n);
}

Rope
RopeDelete(const Rope rope, size_t i, size_t n) {
	Rope left, right, ret_rope;
	assert(rope);
	assert(i + n <= rope->len);
	assert(i >= 0);

	if (i == 0 || i + n == rope->len)
		return RopeSubstr(rope, i, n);

	left = RopeSubstr(rope, 0, i);
	right = RopeSubstr(rope, i + n, rope->len - i - n);
	ret_rope = RopeConcat(left, right);

	RopeDestroy(left);
	RopeDestroy(right);

	return ret_rope;
}

char
RopeIndex(const Rope rope, size_t i) {
	Rope this = rope;
	assert(rope);
	assert(i < rope->len);

	for (;;) {
		if (this->is_leaf)
			return this->str[i];
		else {
			size_t llen = this->left->len;

			if (i < llen)
				this = this->left;
			else {
				this = this->right;
				i -= llen;
			}
		}
	}
}

typedef enum {
	LEFT_DOWN,
	RIGHT_DOWN,
} scan_dir;

struct rope_scan_leaf_tag {
	size_t depth;
	bool is_end;
	Rope rope;
	scan_dir dir[ROPE_SCAN_MAX_DEPTH];
	Rope stack[ROPE_SCAN_MAX_DEPTH];
};

RopeScanLeaf
RopeScanLeafInit(const Rope rope) {
	RopeScanLeaf scan = palloc(sizeof(*scan));

	scan->rope = rope;
	scan->depth = 0;
	scan->is_end = false;

	while (!scan->rope->is_leaf) {
		scan->stack[scan->depth] = scan->rope;
		scan->dir[scan->depth] = LEFT_DOWN;
		scan->depth++;
		scan->rope = scan->rope->left;
	}

	return scan;
}

char *
RopeScanLeafGetNext(RopeScanLeaf scan) {
	char *rv;

	if (scan->is_end)
		return NULL;

	rv = scan->rope->str;

	do {
		if (scan->depth == 0) /* End of scan */
		{
			scan->is_end = true;
			return rv;
		}
		scan->depth--;
	} while (scan->dir[scan->depth] == RIGHT_DOWN);

	scan->rope = scan->stack[scan->depth];
	scan->dir[scan->depth] = RIGHT_DOWN;
	scan->depth++;
	scan->rope =
	    scan->rope->right; /* XXX: Assuming non-leaf rope has right child */

	while (!scan->rope->is_leaf) {
		scan->stack[scan->depth] = scan->rope;
		scan->dir[scan->depth] = LEFT_DOWN;
		scan->depth++;
		scan->rope = scan->rope->left;
	}

	return rv;
}

void
RopeScanLeafFini(RopeScanLeaf scan) {
	pfree(scan);
}

struct rope_scan_char_tag {
	RopeScanLeaf scan_leaf;
	char *str;
	size_t pos;
};

RopeScanChar
RopeScanCharInit(const Rope rope) {
	RopeScanChar scan = palloc(sizeof(*scan));

	scan->scan_leaf = RopeScanLeafInit(rope);
	scan->str = RopeScanLeafGetNext(scan->scan_leaf);
	scan->pos = 0;

	return scan;
}

char
RopeScanCharGetNext(RopeScanChar scan) {
	if (scan->str[scan->pos] == '\0') {
		scan->str = RopeScanLeafGetNext(scan->scan_leaf);

		if (!scan->str)
			return 0;

		scan->pos = 0;
	}

	return scan->str[scan->pos++];
}

void
RopeScanCharFini(RopeScanChar scan) {
	RopeScanLeafFini(scan->scan_leaf);
	pfree(scan);
}
