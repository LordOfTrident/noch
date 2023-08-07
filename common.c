#ifdef __cplusplus
extern "C" {
#endif

#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "common.h"

NOCH_DEF char *xstrdup(const char *str) {
	NOCH_ASSERT(str != NULL);

	char *ptr = (char*)NOCH_ALLOC(strlen(str) + 1);
	if (ptr == NULL) {
		NOCH_OUT_OF_MEM();
		return NULL;
	}

	strcpy(ptr, str);
	return ptr;
}

#ifdef __cplusplus
}
#endif
