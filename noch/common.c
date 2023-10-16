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

#ifdef __cplusplus
}
#endif
