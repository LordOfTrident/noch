#ifndef NOCH_SV_H_HEADER_GUARD
#define NOCH_SV_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdio.h>  /* fprintf, FILE */
#include <stdlib.h> /* strtol, strtoull, strtod */
#include <stddef.h> /* size_t */
#include <string.h> /* strcmp, strlen, strncpy */
#include <ctype.h>  /* tolower */

#include "internal/def.h"

typedef struct {
	size_t      len;
	const char *ptr;
} sv_t;

#define SV_FMT     "%.*s"
#define SV_ARG(SV) (int)(SV).len, (SV).ptr

#ifdef __cplusplus
#	define SV_NULL sv_t({0, NULL})
#else
#	define SV_NULL (sv_t){0, NULL}
#endif

#define SV_NPOS        (size_t)-1
#define SV_WHITESPACES " \f\n\r\t\v"

NOCH_DEF sv_t sv_new(const char *cstr, size_t len);
NOCH_DEF sv_t sv_cstr(const char *cstr);

NOCH_DEF char sv_at(sv_t this, size_t idx);
NOCH_DEF bool sv_is_null(sv_t this);

NOCH_DEF bool sv_equals(sv_t this, sv_t to);

NOCH_DEF bool sv_has_prefix(sv_t this, sv_t prefix);
NOCH_DEF bool sv_has_suffix(sv_t this, sv_t suffix);

NOCH_DEF sv_t sv_substr    (sv_t this, size_t start, size_t len);
NOCH_DEF sv_t sv_trim_front(sv_t this, const char *chs);
NOCH_DEF sv_t sv_trim_back (sv_t this, const char *chs);
NOCH_DEF sv_t sv_trim      (sv_t this, const char *chs);

NOCH_DEF bool   sv_contains      (sv_t this, char ch);
NOCH_DEF size_t sv_count         (sv_t this, char ch);
NOCH_DEF size_t sv_find_first    (sv_t this, char ch);
NOCH_DEF size_t sv_find_last     (sv_t this, char ch);
NOCH_DEF size_t sv_find_first_not(sv_t this, char ch);
NOCH_DEF size_t sv_find_last_not (sv_t this, char ch);

NOCH_DEF bool   sv_contains_substr(sv_t this, sv_t substr);
NOCH_DEF size_t sv_count_substr   (sv_t this, sv_t substr);
NOCH_DEF size_t sv_find_substr    (sv_t this, sv_t substr);

#ifdef __cplusplus
}
#endif
#endif
