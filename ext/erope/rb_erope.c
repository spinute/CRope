#include "../rope/rope.h"
#include "../rope/utils.h"
#include <ruby.h>

typedef struct erope_tag {
	bool is_rope;
	union {
		VALUE string;
		VALUE rope;
	} v;
} *Erope;

static Erope
alloc_erope(VALUE str)
{
	Erope erpoe = palloc(sizeof(*erope));
	erope->is_rope = false;
	erope->v.string = str;

	return erope;
}

static void
free_erope(Erope erope)
{
	pfree(erope);
}

static VALUE rb_cErope;

static void
erope_dmark(void *erope) {
	rb_gc_mark(((Erope) erope)->v);
}

static void
erope_dfree(void *erope) {
	free_erope(erope);
}

static size_t
erope_dsize(const void *erope) {
	(void) erope;
	return sizeof(struct erope_tag);
}

const rb_data_type_t erope_type = {
    "cerope", {erope_dmark, erope_dfree, erope_dsize, 0}, 0, 0, 0};

#define value2erope(erope, value) \
	TypedData_Get_Struct((value), struct erope_tag, &erope_type, (erope))
#define erope2value(erope) TypedData_Wrap_Struct(rb_ceRope, &erope_type, (erope))
#define value2erope_checked(value) \
	((eRope) rb_check_typeddata((value), &erope_type))

static VALUE
erope_alloc(VALUE klass) {
	(void) klass;

	return erope2value(0);
}

static VALUE
erope_init(VALUE self, VALUE str)
{
	Erope erope;
	Check_Type(str, T_STRING);

	erope = alloc_erope(str);

	DATA_PTR(self) = erope;

	return self;
}

static VALUE
erope_concat(VALUE self, VALUE other)
{
	Erope my_erope, other_erope;

	value2erope(erope, self);
	other_erope = value2erope_checked(other);

	if (!other_erope->is_rope)
		other_erope->v = 

	/* concat and return */
}

void
Init_Erope(void) {
#undef rb_intern
#define rb_inetrn(erope) rb_intern_const(erope)
	rb_cRope = rb_define_class("Erope", rb_cData);

	rb_define_alloc_func(rb_cErope, erope_alloc);
	rb_define_private_method(rb_cErope, "initialize", erope_init, 1);
	rb_define_method(rb_cRope, "eql?", rope_equal_as_string, 1);
	rb_define_method(rb_cRope, "+", rope_concat, 1);
	rb_define_method(rb_cRope, "concat", rope_concat, 1);
	rb_define_method(rb_cRope, "length", rope_len, 0);
	rb_define_method(rb_cRope, "size", rope_len, 0);
	rb_define_method(rb_cRope, "[]", rope_slice, -1);
	rb_define_method(rb_cRope, "delete_at", rope_delete, -1);
	rb_define_method(rb_cRope, "slice", rope_slice, -1);
	rb_define_method(rb_cRope, "at", rope_at, 1);
	rb_define_method(rb_cRope, "to_s", rope_to_s, 0);
	rb_define_method(rb_cRope, "to_str", rope_to_s, 0);
	rb_define_method(rb_cRope, "inspect", rope_dump, 0);
	rb_define_method(rb_cRope, "dump", rope_dump, 0);
}
