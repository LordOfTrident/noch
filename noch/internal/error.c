#ifndef NOCH_INTERNAL_ERROR_C_IMPLEMENTATION_GUARD
#define NOCH_INTERNAL_ERROR_C_IMPLEMENTATION_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include "error.h"

static char nochErrorMsg[256] = {0};

NOCH_DEF const char *nochGetError(void) {
	return nochErrorMsg;
}

static int nochError(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vsnprintf(nochErrorMsg, sizeof(nochErrorMsg), fmt, args);
	va_end(args);
	return -1;
}

#ifdef __cplusplus
}
#endif
#endif
