#include "rope.h"
#include "utils.h"

#include <ruby.h>

static VALUE rb_cRope;
#define MAX_STR_SIZE 65536

static void
rope_dmark(void *rope) {
	(void) rope;
}

static void
rope_dfree(void *rope) {
	RopeDestroy(rope);
}

static size_t
rope_dsize(const void *rope) {
	return RopeGetSize((Rope) rope);
}

const rb_data_type_t rope_type = {
    "crope", {rope_dmark, rope_dfree, rope_dsize, 0}, 0, 0, 0};

#define value2rope(rope, value) \
	TypedData_Get_Struct((value), struct rope_tag, &rope_type, (rope))
#define rope2value(rope) TypedData_Wrap_Struct(rb_cRope, &rope_type, (rope))
#define value2rope_checked(value) \
	((Rope) rb_check_typeddata((value), &rope_type))

static VALUE
rope_alloc(VALUE klass) {
	return rope2value(0);
}

static VALUE
rope_init(int argc, VALUE *argv, VALUE self) {
	VALUE str = 0;
	Rope rope;

	rb_scan_args(argc, argv, "01", &str);

	/* XXX: empty rope is valid?*/
	Check_Type(str, T_STRING);

	rope = RopeCreate(rb_string_value_cstr(&str), RSTRING_LEN(str));

	DATA_PTR(self) = rope;

	return self;
}

static int
get_fixnum_checked(VALUE v) {
	switch (TYPE(v)) {
		case T_FIXNUM:
			break;
		case T_BIGNUM:
			elog("rope_at: too lange index specified");
		default:
			rb_raise(rb_eTypeError, "%s: invalid type argument", __func__);
			break;
	}

	return FIX2INT(v);
}

static VALUE
rope_at(VALUE self, VALUE vi) {
	Rope rope;
	int i, len;
	char c[2];

	value2rope(rope, self);
	i = get_fixnum_checked(vi);
	len = (int) RopeGetLen(rope);

	if (len <= i)
		return Qnil;

	if (i >= 0)
		c[0] = RopeIndex(rope, i);
	else {
		int ri = len + i;

		if (ri < 0)
			return Qnil;

		c[0] = RopeIndex(rope, ri);
	}

	c[1] = '\0';
	return rb_str_new_cstr(c);
}

static VALUE
rope_substr(VALUE self, VALUE vi, VALUE vn) {
	Rope rope, ret_rope;
	int i = get_fixnum_checked(vi), n = get_fixnum_checked(vn), len;

	value2rope(rope, self);
	len = (int) RopeGetLen(rope);

	if (len <= i || n < 0)
		return Qnil;

	if (i >= 0) {
		if (i + n > len)
			n = len - i;
		ret_rope = RopeSubstr(rope, i, n);
	} else {
		int ri = len + i;

		if (ri < 0)
			return Qnil;

		if (ri + n > len)
			n = len - ri;

		ret_rope = RopeSubstr(rope, ri, n);
	}

	return rope2value(ret_rope);
}

static VALUE
rope_slice(int argc, VALUE *argv, VALUE self) {
	VALUE vi, vn;
	int n_arg = rb_scan_args(argc, argv, "11", &vi, &vn);

	if (n_arg == 1)
		return rope_at(self, vi);
	else if (n_arg == 2)
		return rope_substr(self, vi, vn);
	else
		rb_raise(rb_eArgError, "%s: unexpected n_arg", __func__);
}

static VALUE
rope_len(VALUE self) {
	Rope r;
	value2rope(r, self);

	return INT2NUM(RopeGetLen(r));
}

static VALUE
rope_concat(VALUE self, VALUE other) {
	Rope r1, r2;

	value2rope(r1, self);
	r2 = value2rope_checked(other);

	return rope2value(RopeConcat(r1, r2));
}

static VALUE
rope_to_s(VALUE self) {
	Rope rope;
	char buf[MAX_STR_SIZE];

	value2rope(rope, self);

	if (RopeToString(rope, buf, MAX_STR_SIZE) < 0) {
		elog("WARNING(rope_to_s): buf too small");
		return Qnil;
	}

	return rb_str_new(buf, RopeGetLen(rope));
}

static VALUE
rope_dump(VALUE self) {
	Rope rope;
	value2rope(rope, self);

	RopeDump(rope);

	return self;
}

static VALUE
rope_equal_as_string(VALUE self, VALUE other) {
	Rope my_rope, other_rope;
	char my_buf[MAX_STR_SIZE], other_buf[MAX_STR_SIZE];

	value2rope(my_rope, self);
	other_rope = value2rope_checked(other);

	if (RopeToString(my_rope, my_buf, MAX_STR_SIZE) < 0) {
		elog("WARNING(rope_to_s): buf too small");
		return Qnil;
	}
	if (RopeToString(other_rope, other_buf, MAX_STR_SIZE) < 0) {
		elog("WARNING(rope_to_s): buf too small");
		return Qnil;
	}

	return strcmp(my_buf, other_buf) == 0 ? Qtrue : Qfalse;
}

void
Init_Rope(void) {
#undef rb_intern
#define rb_inetrn(rope) rb_intern_const(rope)
	rb_cRope = rb_define_class("Rope", rb_cData);

	rb_define_alloc_func(rb_cRope, rope_alloc);
	rb_define_private_method(rb_cRope, "initialize", rope_init, -1);
	rb_define_method(rb_cRope, "eql?", rope_equal_as_string, 1);
	rb_define_method(rb_cRope, "+", rope_concat, 1);
	rb_define_method(rb_cRope, "concat", rope_concat, 1);
	rb_define_method(rb_cRope, "length", rope_len, 0);
	rb_define_method(rb_cRope, "size", rope_len, 0);
	rb_define_method(rb_cRope, "[]", rope_slice, -1);
	rb_define_method(rb_cRope, "slice", rope_slice, -1);
	rb_define_method(rb_cRope, "at", rope_at, 1);
	rb_define_method(rb_cRope, "to_s", rope_to_s, 0);
	rb_define_method(rb_cRope, "to_str", rope_to_s, 0);
	rb_define_method(rb_cRope, "inspect", rope_dump, 0);
	rb_define_method(rb_cRope, "dump", rope_dump, 0);
}
