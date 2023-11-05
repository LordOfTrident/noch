#ifndef NOCH_INTERNAL_ERROR_H_HEADER_GUARD
#define NOCH_INTERNAL_ERROR_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* NULL */
#include <stdarg.h> /* va_list, va_start, va_end, vsnprintf */

#include "def.h"

NOCH_DEF const char *nochGetError(void);

#ifdef __cplusplus
}
#endif
#endif
