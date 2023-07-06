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

#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/def.h"

#ifndef NOCH_MAX_FLAG_NAME_LEN
#	define NOCH_MAX_FLAG_NAME_LEN 128
#endif

NOCH_DEF bool noch_is_arg_flag     (const char *arg); /* -h, --help */
NOCH_DEF bool noch_is_arg_long_flag(const char *arg); /* --help */
NOCH_DEF bool noch_is_arg_flags_end(const char *arg); /* -- */

#define NOCH__FLAG_FUNC_DECL(POSTFIX, TYPE)                                          \
	NOCH_DEF void noch_flag_##POSTFIX(const char *short_name, const char *long_name, \
	                                  const char *desc, TYPE *var)

NOCH__FLAG_FUNC_DECL(str,  const char*);
NOCH__FLAG_FUNC_DECL(char, char);
NOCH__FLAG_FUNC_DECL(int,  int);
NOCH__FLAG_FUNC_DECL(size, size_t);
NOCH__FLAG_FUNC_DECL(num,  double);
NOCH__FLAG_FUNC_DECL(bool, bool);

typedef enum {
	NOCH_ARGS_OK = 0,

	NOCH_ARGS_OUT_OF_MEM,
	NOCH_ARGS_UNKNOWN_FLAG,
	NOCH_ARGS_MISSING,
	NOCH_ARGS_EXTRA,
	NOCH_ARGS_EXPECTED_STR,
	NOCH_ARGS_EXPECTED_CHAR,
	NOCH_ARGS_EXPECTED_INT,
	NOCH_ARGS_EXPECTED_SIZE,
	NOCH_ARGS_EXPECTED_NUM,
	NOCH_ARGS_EXPECTED_BOOL,

	NOCH_ARGS_ERROR_COUNT,
} noch_args_err_t;

NOCH_DEF const char *noch_args_err_to_str(noch_args_err_t err);

typedef struct {
	size_t       c;
	const char **v;
	char       **base;
} noch_args_t;

NOCH_DEF noch_args_t noch_args_new(int argc, const char **argv);

#define NOCH_FOREACH_IN_ARGS(ARGS, ARG_VAR, BODY)                                        \
	do {                                                                                 \
		for (int noch__foreach_i = 0; chol__foreach_i < (ARGS)->c; ++ chol__foreach_i) { \
			const char *ARG_VAR = (ARGS)->v[chol__foreach_i];                            \
			BODY                                                                         \
		}                                                                                \
	} while (0)

NOCH_DEF const char     *noch_args_shift      (noch_args_t *args);
NOCH_DEF noch_args_err_t noch_args_parse_flags(noch_args_t *args, size_t *where, size_t *end,
                                               noch_args_t *stripped);

NOCH_DEF void noch_fprint_flags_usage(FILE *file);
NOCH_DEF void noch_fprint_usage      (FILE *file, const char *name, const char **usages,
                                      size_t usages_count, const char *desc, bool print_flags);

#ifdef __cplusplus
}
#endif
#endif
