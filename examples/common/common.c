#include <stdio.h>

#include <noch/common.h>
#include <noch/common.c>

int main(int argc, char **argv) {
	UNUSED(argc); UNUSED(argv);

	char str_buf[] = "Hello, world!";
	printf("String: '%s'\n", str_buf);
	printf("Buffer length: '%i'\n", (int)ARRAY_LEN(str_buf));

	char *dup = stringDup(str_buf);
	printf("Copied to heap: '%s'\n", dup);

	TODO("zeroInit example");
}
