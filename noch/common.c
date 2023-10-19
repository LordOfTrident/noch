#ifdef __cplusplus
extern "C" {
#endif

#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "common.h"

NOCH_DEF char *xstrdup(const char *str) {
	NOCH_ASSERT(str != NULL);

	char *ptr;
	NOCH_MUST_ALLOC(char, ptr, strlen(str) + 1);

	strcpy(ptr, str);
	return ptr;
}

NOCH_DEF void fputn(double num, FILE *file) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%.13f", num);

	bool   found = false;
	size_t i;
	for (i = 0; buf[i] != '\0'; ++ i) {
		if (buf[i] == '.' && !found)
			found = true;
	}
	if (!found) {
		fprintf(file, "%s", buf);
		return;
	} else
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

	fprintf(file, "%s", buf);
}

NOCH_DEF void putn(double num) {
	fputn(num, stdout);
}

#ifdef __cplusplus
}
#endif
