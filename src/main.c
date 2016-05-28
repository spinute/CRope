#include "rope.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void
test_to_string(Rope rope, char expected[]) {
	char buf[100];
	memset(buf, 0, sizeof(buf));
	RopeToString(rope, (char *) buf, sizeof(buf));
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
	RopeDestroy(lrope);
	RopeDestroy(rrope);

	RopeDump(concat);

	{
		RopeScanLeaf scan = RopeScanLeafInit(concat);
		char * str = RopeScanLeafGetNext(scan);
		puts(str);
		assert(strcmp(str, left) == 0);

		str = RopeScanLeafGetNext(scan);
		puts(str);
		assert(strcmp(str, right) == 0);

		str = RopeScanLeafGetNext(scan);
		assert(str == NULL);

		RopeScanLeafFini(scan);
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
