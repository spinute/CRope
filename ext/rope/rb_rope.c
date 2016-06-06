#include "rope.h"
#include "utils.h"

#include <ruby.h>

static VALUE rb_cRope;
#define MAX_STR_SIZE 1000

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
	TypedData_Get_Struct((value), struct rope_tag, &rope_type, rope)
#define rope2value(rope) TypedData_Wrap_Struct(rb_cRope, &rope_type, rope)

static VALUE
rope_alloc(VALUE klass) {
	elog("rope_alloc called");
	return rope2value(0);
}

static VALUE
rope_init(int argc, VALUE *argv, VALUE self) {
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
rope_plus(VALUE self, VALUE other) {
	Rope r1, r2;

	value2rope(r1, self);
	value2rope(r2, other);
	RopeDump(r2);

	return rope2value(RopeConcat(r1, r2));
}

static VALUE
rope_to_s(VALUE self) {
	Rope rope;
	char buf[MAX_STR_SIZE];

	value2rope(rope, self);
	RopeDump(rope);

	if (RopeToString(rope, buf, MAX_STR_SIZE) < 0) {
		elog("rope_to_s: buf too small");
		return 0;
	}

	return rb_str_new(buf, RopeGetLen(rope));
}

void
Init_Rope(void) {
#undef rb_intern
#define rb_inetrn(rope) rb_intern_const(rope)
	rb_cRope = rb_define_class("Rope", rb_cData);

	rb_define_private_method(rb_cRope, "initialize", rope_init, -1);
	rb_define_alloc_func(rb_cRope, rope_alloc);
	rb_define_method(rb_cRope, "+", rope_plus, 1);
	rb_define_method(rb_cRope, "to_s", rope_to_s, 0);
	rb_define_method(rb_cRope, "to_str", rope_to_s, 0);
}
