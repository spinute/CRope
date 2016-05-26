#include "rope.h"
#include <string.h>

int
main(int argc, char *argv[])
{
	char left[] = "test ",
		 right[] = "desu.";
	Rope lrope = RopeCreate(left, strlen(left));
	Rope rrope = RopeCreate(right, strlen(right));
	Rope concat;

	RopeToString(lrope);
	RopeToString(rrope);

	concat = RopeConcat(lrope, rrope);

	RopeToString(concat);

	RopeDestroy(concat);
	RopeDestroy(lrope);
	RopeDestroy(rrope);

	(void) argc;
	(void) argv;

	return 0;
}
