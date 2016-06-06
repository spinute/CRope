#include <ruby.h>

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static VALUE rb_cRope;
#define MAX_STR_SIZE 1000

/* Utils */

#define elog(str) printf("elog(%s): %s\n", __func__, (str))

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

/* Rope implementation in C */

typedef struct rope_tag *Rope;
typedef struct rope_scan_leaf_tag *RopeScanLeaf;
typedef struct rope_scan_char_tag *RopeScanChar;

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
RopeConcat(Rope left, Rope right) {
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
rope_dump(Rope rope, int level) {
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
RopeDump(Rope rope) {
	assert(rope);
	rope_dump(rope, 0);
}

static int
rope_collect_cstr(Rope rope, char *ret_buf, int i) {
	if (rope->is_leaf) {
		memcpy(ret_buf + i, rope->str, rope->len);
		return i + rope->len;
	}

	i = rope_collect_cstr(rope->left, ret_buf, i);
	return rope_collect_cstr(rope->right, ret_buf, i);
}

int
RopeToString(Rope rope, char *ret_buf, size_t buf_size) {
	int rv;

	if (rope->len > buf_size - 1)
		return -1;

	rv = rope_collect_cstr(rope, ret_buf, 0);
	ret_buf[rope->len] = '\0';

	return rv;
}

size_t
RopeGetLen(Rope rope) {
	assert(rope);
	return rope->len;
}

static Rope
rope_concat_without_rec_ref(Rope left, Rope right) {
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
rope_get_substr(Rope rope, size_t i, size_t n) {
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
RopeSubstr(Rope rope, size_t i, size_t n) {
	assert(rope);
	assert(n != 0);
	assert(i + n <= rope->len);

	return rope_get_substr(rope, i, n);
}

char
RopeIndex(Rope rope, size_t i) {
	assert(rope);
	assert(i < rope->len);

	for (;;) {
		if (rope->is_leaf)
			return rope->str[i];
		else {
			size_t llen = rope->left->len;

			if (i < llen)
				rope = rope->left;
			else {
				rope = rope->right;
				i -= llen;
			}
		}
	}
}

#define ROPE_SCAN_MAX_DEPTH \
	64 /* TODO: It may be possible to fix this value by wordsize */

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
RopeScanLeafInit(Rope rope) {
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
RopeScanCharInit(Rope rope) {
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
/* Ruby C extension wrapper */

#define ROPE_LEN(rope) (((Rope) (rope))->len)
#define ROPE_STR(rope) (((Rope) (rope))->str)

static void
rope_dmark(void *rope)
{
	(void) rope;
}

static void
rope_dfree(void *rope)
{
	RopeDestroy(rope);
}

static size_t
rope_dsize(const void *rope)
{
	return offsetof(struct rope_tag, str) + ROPE_LEN(rope);
}

const rb_data_type_t rope_type = {"crope", {rope_dmark, rope_dfree, rope_dsize, 0}, 0, 0, 0};
#define value2rope(rope, value) TypedData_Get_Struct((value), struct rope_tag, &rope_type, rope)
#define rope2value(rope) TypedData_Wrap_Struct(rb_cRope, &rope_type, rope)

static VALUE
rope_alloc(VALUE klass)
{
	elog("rope_alloc called");
	return rope2value(0);
}

static VALUE
rope_init(int argc, VALUE *argv, VALUE self)
{
	VALUE str = 0;
	Rope rope;

	rb_scan_args(argc, argv, "01", &str);

	Check_Type(str, T_STRING);

	rope = RopeCreate(rb_string_value_cstr(&str), RSTRING_LEN(str));

	(void) self; /* XXX: how to use self? */

	elog("rope_init called");
	RopeDump(rope);

	DATA_PTR(self) = rope;

	return self;
}

static VALUE
rope_plus(VALUE self, VALUE other)
{
	Rope r1, r2;

	value2rope(r1, self);
	value2rope(r2, other);
	RopeDump(r2);

	return rope2value(RopeConcat(r1, r2));
}

static VALUE
rope_to_s(VALUE self)
{
	Rope rope;
	char buf[MAX_STR_SIZE];

	value2rope(rope, self);
	RopeDump(rope);

	if (RopeToString(rope, buf, MAX_STR_SIZE) < 0)
	{
		elog("rope_to_s: buf too small");
		return 0;
	}

	return rb_str_new(buf, ROPE_LEN(rope));
}

void
Init_Rope(void)
{
#undef rb_intern
#define rb_inetrn(rope) rb_intern_const(rope)
	rb_cRope = rb_define_class("Rope", rb_cData);

	rb_define_private_method(rb_cRope, "initialize", rope_init, -1);
	rb_define_alloc_func(rb_cRope, rope_alloc);
	rb_define_method(rb_cRope, "+", rope_plus, 1);
    rb_define_method(rb_cRope, "to_s", rope_to_s, 0);
    rb_define_method(rb_cRope, "to_str", rope_to_s, 0);
}
