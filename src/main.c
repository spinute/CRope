#include "rope.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

char left[] = "test ", right[] = "desu.", left_right[] = "test desu.";

static void
test_to_string(Rope rope, char expected[]) {
	char buf[100];

	memset(buf, 0, sizeof(buf));

	if (RopeToString(rope, (char *) buf, sizeof(buf)) < 0)
		elog("buf_size is too small");

	printf("len_expected=%zu, buflen=%zu\n", strlen(expected), strlen(buf));
	assert(strlen(expected) == strlen(buf));
	assert(strcmp(buf, expected) == 0);
}

static void
test_concat(void) {
	Rope lrope = RopeCreate(left, strlen(left)),
	     rrope = RopeCreate(right, strlen(right)),
	     concat = RopeConcat(lrope, rrope), deep = RopeConcat(lrope, concat),
	     moredeep = RopeConcat(deep, concat);

	test_to_string(lrope, left);
	test_to_string(rrope, right);
	test_to_string(concat, left_right);
	test_to_string(deep, "test test desu.");

	RopeDestroy(concat);
	RopeDestroy(lrope);
	RopeDestroy(rrope);
	RopeDestroy(deep);

	elog("moredeep");
	RopeDump(moredeep);
	test_to_string(moredeep, "test test desu.test desu.");

	RopeDestroy(moredeep);
}

static void
test_scan(void) {
	Rope lrope = RopeCreate(left, strlen(left)),
	     rrope = RopeCreate(right, strlen(right)),
	     concat = RopeConcat(lrope, rrope), deep = RopeConcat(concat, concat),
	     moredeep = RopeConcat(concat, deep);

	{
		elog("scan leaf init");
		RopeScanLeaf scan = RopeScanLeafInit(moredeep);

		assert(strcmp(RopeScanLeafGetNext(scan), left) == 0);
		assert(strcmp(RopeScanLeafGetNext(scan), right) == 0);
		assert(strcmp(RopeScanLeafGetNext(scan), left) == 0);
		assert(strcmp(RopeScanLeafGetNext(scan), right) == 0);
		assert(strcmp(RopeScanLeafGetNext(scan), left) == 0);
		assert(strcmp(RopeScanLeafGetNext(scan), right) == 0);
		assert(RopeScanLeafGetNext(scan) == NULL);

		RopeScanLeafFini(scan);
		elog("scan leaf fini");
	}

	{
		elog("scan char init");
		RopeScanChar scan = RopeScanCharInit(deep);

		for (int i = 0; i < (int) strlen(left_right); i++)
			assert(RopeScanCharGetNext(scan) == left_right[i]);
		for (int i = 0; i < (int) strlen(left_right); i++)
			assert(RopeScanCharGetNext(scan) == left_right[i]);

		assert(RopeScanCharGetNext(scan) == '\0');

		RopeScanCharFini(scan);
		elog("scan char fini");
	}

	RopeDestroy(lrope);
	RopeDestroy(rrope);
	RopeDestroy(concat);
	RopeDestroy(deep);
	RopeDestroy(moredeep);
}

static void
test_substr(void) {
	Rope lrope = RopeCreate(left, strlen(left)),
	     rrope = RopeCreate(right, strlen(right)),
	     concat = RopeConcat(lrope, rrope);
	Rope lsub, rsub, bsub;

	lsub = RopeSubstr(concat, 1, 3);
	rsub = RopeSubstr(concat, 6, 4);
	bsub = RopeSubstr(concat, 1, 7);

	test_to_string(lsub, "est");
	test_to_string(rsub, "esu.");
	test_to_string(bsub, "est des");

	RopeDestroy(lrope);
	RopeDestroy(rrope);
	RopeDestroy(concat);
	RopeDestroy(lsub);
	RopeDestroy(rsub);
	RopeDestroy(bsub);
}

int
main(int argc, char *argv[]) {
	test_concat();
	test_scan();
	test_substr();

	(void) argc;
	(void) argv;

	return 0;
}
