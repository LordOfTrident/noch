#ifdef __cplusplus
extern "C" {
#endif

#include "internal/alloc.h"
#include "internal/assert.h"

#include "common.h"

NOCH_DEF char *stringDup(const char *str) {
	nochAssert(str != NULL);

	char *ptr = (char*)nochAlloc(strlen(str) + 1);
	if (ptr == NULL)
		NOCH_OUT_OF_MEM();

	strcpy(ptr, str);
	return ptr;
}

NOCH_DEF void printNumF(FILE *file, double num) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%.13f", num);

	bool   found = false;
	size_t i;
	for (i = 0; buf[i] != '\0'; ++ i) {
		if (buf[i] == '.' && !found)
			found = true;
	}

	if (found) {
		-- i;
		while (true) {
			if (buf[i] != '0') {
				if (buf[i] == '.')
					buf[i] = '\0';

				break;
			}

			buf[i] = '\0';
			-- i;
		}
	}

	fprintf(file, "%s", buf);
}

#ifdef __cplusplus
}
#endif
