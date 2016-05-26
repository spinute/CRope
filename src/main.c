#include "rope.h"
#include <string.h>

int
main(int argc, char *argv[])
{
	char left[] = "test ",
		 right[] = "desu.";
	Rope lrope = RopeCreate(left, strlen(left));
	Rope rrope = RopeCreate(right, strlen(right));
	Rope concat, lsub, rsub, bsub;

	RopeToString(lrope);
	RopeToString(rrope);

	concat = RopeConcat(lrope, rrope);
	RopeToString(concat);

	lsub = RopeSubstr(concat, 1, 3);
	rsub = RopeSubstr(concat, 6, 4);
	bsub = RopeSubstr(concat, 1, 7);

	RopeToString(lsub);
	RopeToString(rsub);
	RopeToString(bsub);

	RopeDestroy(concat);
	RopeDestroy(lrope);
	RopeDestroy(rrope);
	RopeDestroy(lsub);
	RopeDestroy(rsub);
	RopeDestroy(bsub);

	(void) argc;
	(void) argv;

	return 0;
}
