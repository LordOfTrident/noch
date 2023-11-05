#ifndef NOCH_SV_H_HEADER_GUARD
#define NOCH_SV_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>   /* fprintf, FILE */
#include <stdlib.h>  /* strtol, strtoull, strtod */
#include <stddef.h>  /* size_t */
#include <string.h>  /* strcmp, strlen, strncpy */
#include <ctype.h>   /* tolower */
#include <stdbool.h> /* bool, true, false */

#include "internal/def.h"

typedef struct {
	size_t      length;
	const char *data;
} StringView;

#define SV_FMT     "%.*s"
#define SV_ARG(SV) (int)(SV).length, (SV).data

#define SV_NULL        svNew(NULL, 0)
#define SV_NPOS        (size_t)-1
#define SV_WHITESPACES " \f\n\r\t\v"

NOCH_DEF StringView svNew(const char *data, size_t len);
NOCH_DEF StringView svFromString(const char *str);

NOCH_DEF char svAt    (StringView this, size_t idx);
NOCH_DEF bool svIsNull(StringView this);

NOCH_DEF bool svEquals(StringView this, StringView to);

NOCH_DEF bool svHasPrefix(StringView this, StringView prefix);
NOCH_DEF bool svHasSuffix(StringView this, StringView suffix);

NOCH_DEF StringView svSubstring(StringView this, size_t start, size_t len);
NOCH_DEF StringView svTrimLeft (StringView this, const char *chs);
NOCH_DEF StringView svTrimRight(StringView this, const char *chs);
NOCH_DEF StringView svTrim     (StringView this, const char *chs);

NOCH_DEF bool   svContains    (StringView this, char ch);
NOCH_DEF size_t svCount       (StringView this, char ch);
NOCH_DEF size_t svFindFirst   (StringView this, char ch);
NOCH_DEF size_t svFindLast    (StringView this, char ch);
NOCH_DEF size_t svFindFirstNot(StringView this, char ch);
NOCH_DEF size_t svFindLastNot (StringView this, char ch);

NOCH_DEF bool   svContainsSubstring(StringView this, StringView substr);
NOCH_DEF size_t svCountSubstrings  (StringView this, StringView substr);
NOCH_DEF size_t svFindSubstring    (StringView this, StringView substr);

#ifdef __cplusplus
}
#endif
#endif
