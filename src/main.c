#include "rope.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

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

int
main(int argc, char *argv[]) {
	char left[] = "test ", right[] = "desu.", left_right[] = "test desu.";
	Rope lrope = RopeCreate(left, strlen(left));
	Rope rrope = RopeCreate(right, strlen(right));
	Rope concat, lsub, rsub, bsub;

	test_to_string(lrope, left);
	test_to_string(rrope, right);

	concat = RopeConcat(lrope, rrope);

	RopeDump(concat);

	{
		Rope deep, moredeep;
		deep = RopeConcat(lrope, concat);

		RopeDump(deep);

		test_to_string(deep, "test test desu.");

		moredeep = RopeConcat(deep, concat);

		RopeDump(moredeep);

		test_to_string(moredeep, "test test desu.test desu.");

		RopeDestroy(deep);
		RopeDestroy(moredeep);
	}

	RopeDestroy(lrope);
	RopeDestroy(rrope);

	{
		RopeScanLeaf scan = RopeScanLeafInit(concat);

		assert(strcmp(RopeScanLeafGetNext(scan), left) == 0);
		assert(strcmp(RopeScanLeafGetNext(scan), right) == 0);
		assert(RopeScanLeafGetNext(scan) == NULL);

		RopeScanLeafFini(scan);
	}

	{
		RopeScanChar scan = RopeScanCharInit(concat);

		for (int i = 0; i < (int) strlen(left_right); i++)
			assert(RopeScanCharGetNext(scan) == left_right[i]);

		assert(RopeScanCharGetNext(scan) == '\0');

		RopeScanCharFini(scan);
	}

	test_to_string(concat, left_right);

	lsub = RopeSubstr(concat, 1, 3);
	rsub = RopeSubstr(concat, 6, 4);
	bsub = RopeSubstr(concat, 1, 7);

	test_to_string(lsub, "est");
	test_to_string(rsub, "esu.");
	test_to_string(bsub, "est des");

	RopeDestroy(concat);
	RopeDestroy(lsub);
	RopeDestroy(rsub);
	RopeDestroy(bsub);

	(void) argc;
	(void) argv;

	return 0;
}
