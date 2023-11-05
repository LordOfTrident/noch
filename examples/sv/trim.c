#include <stdio.h> /* printf */

#include <noch/sv.h>
#include <noch/sv.c>

int main(void) {
	StringView sv = svFromString("\r \t   Hello, world!\t    ");

	sv = svTrim(sv, SV_WHITESPACES);
	printf("'" SV_FMT "'\n", SV_ARG(sv));

	return 0;
}
