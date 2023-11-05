#include <stdio.h>  /* printf */
#include <string.h> /* memcpy */
#include <stdlib.h> /* malloc, free */

#include <noch/utf8.h>
#include <noch/utf8.c>

void printFindResult(const char *str, const char *toFind, const char *foundPtr, size_t idx) {
	printf("Find '%s' in '%s'\n", toFind, str);

	if (foundPtr == NULL) {
		printf("\tNot found\n");
		return;
	}

	printf("\tFound at %i\n", (int)idx);

	size_t size      = stringU8Size(toFind);
	char  *foundCopy = (char*)malloc(size + 1);
	memcpy(foundCopy, foundPtr, size);
	foundCopy[size] = '\0';

	printf("\tFound: '%s'\n", foundCopy);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: %s <STRING> <FIND>\n", argv[0]);
		return 0;
	}

	const char *a = argv[1], *b = argv[2];

	printf("len(a):   %i\n", (int)stringU8Length(a));
	printf("len(b):   %i\n", (int)stringU8Length(b));
	printf("bytes(a): %i\n", (int)stringU8Size(a));
	printf("bytes(b): %i\n", (int)stringU8Size(b));

	size_t csAt, ciAt;
	const char *csPtr = stringU8FindSub(a, b, &csAt, true);  /* Case sensitive find */
	const char *ciPtr = stringU8FindSub(a, b, &ciAt, false); /* Case insensitive find */

	printf("\nCase sensitive: ");
	printFindResult(a, b, csPtr, csAt);

	printf("Case insensitive: ");
	printFindResult(a, b, ciPtr, ciAt);

	return 0;
}
