#include <stdio.h>  /* printf */
#include <string.h> /* memcpy */
#include <stdlib.h> /* malloc, free */

#include "../../utf8.h"
#include "../../utf8.c"

void print_res(const char *str, const char *find, const char *res, size_t idx) {
	printf("Find '%s' in '%s'\n", find, str);

	if (res == NULL) {
		printf("\tNot found\n");
		return;
	}

	printf("\tFound at %i\n", (int)idx);

	size_t size_bytes = u8_str_bytes(find);
	char  *found      = (char*)malloc(size_bytes + 1);
	memcpy(found, res, size_bytes);
	found[size_bytes] = '\0';

	printf("\tFound: '%s'\n", found);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: %s <STRING> <FIND>\n", argv[0]);
		return 0;
	}

	const char *a = argv[1], *b = argv[2];

	printf("len(a):   %i\n", (int)u8_str_len(a));
	printf("len(b):   %i\n", (int)u8_str_len(b));
	printf("bytes(a): %i\n", (int)u8_str_bytes(a));
	printf("bytes(b): %i\n", (int)u8_str_bytes(b));

	size_t cs_at, ci_at;
	const char *res_cs = u8_str_find_str   (a, b, &cs_at);
	const char *res_ci = u8_str_find_str_ci(a, b, &ci_at);

	printf("\nCS: ");
	print_res(a, b, res_cs, cs_at);

	printf("CI: ");
	print_res(a, b, res_ci, ci_at);

	return 0;
}
