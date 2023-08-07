#ifndef NOCH_ARGS_H_HEADER_GUARD
#define NOCH_ARGS_H_HEADER_GUARD
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
#include "internal/err.h"

#ifndef MAX_FLAG_NAME_LEN
#	define MAX_FLAG_NAME_LEN 128
#endif

#ifndef FLAGS_CAPACITY
#	define FLAGS_CAPACITY 128
#endif

NOCH_DEF bool arg_is_flag     (const char *arg); /* -h, --help */
NOCH_DEF bool arg_is_long_flag(const char *arg); /* --help */
NOCH_DEF bool arg_is_flags_end(const char *arg); /* -- */

#define DECL_FLAG_FUNC(POSTFIX, TYPE)                                           \
	NOCH_DEF void flag_##POSTFIX(const char *short_name, const char *long_name, \
	                             const char *desc, TYPE *var)

DECL_FLAG_FUNC(str,  const char*);
DECL_FLAG_FUNC(char, char);
DECL_FLAG_FUNC(int,  int);
DECL_FLAG_FUNC(size, size_t);
DECL_FLAG_FUNC(num,  double);
DECL_FLAG_FUNC(bool, bool);

#undef DECL_FLAG_FUNC

typedef struct {
	size_t       c;
	const char **v;
	char       **base;
} args_t;

NOCH_DEF args_t args_new(int argc, const char **argv);

#define FOREACH_IN_ARGS(ARGS, VAR, BODY)                                  \
	do {                                                                  \
		for (int _foreach_i = 0; _foreach_i < (ARGS)->c; ++ _foreach_i) { \
			const char *VAR = (ARGS)->v[_foreach_i];                      \
			BODY                                                          \
		}                                                                 \
	} while (0)

NOCH_DEF const char *args_shift      (args_t *args);
NOCH_DEF int         args_parse_flags(args_t *args, size_t *where, args_t *stripped, bool *extra);

NOCH_DEF void flags_usage_fprint(FILE *file);
NOCH_DEF void args_usage_fprint (FILE *file, const char *name, const char **usages,
                                 size_t usages_count, const char *desc, bool print_flags);

#ifdef __cplusplus
}
#endif
#endif
