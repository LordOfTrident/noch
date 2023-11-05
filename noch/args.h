#ifndef NOCH_ARGS_H_HEADER_GUARD
#define NOCH_ARGS_H_HEADER_GUARD
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
#include "internal/error.h"

#ifndef MAX_FLAG_NAME_LEN
#	define MAX_FLAG_NAME_LEN 128
#endif

#ifndef FLAGS_CAPACITY
#	define FLAGS_CAPACITY 128
#endif

NOCH_DEF bool argIsFlag    (const char *arg); /* -h, --help */
NOCH_DEF bool argIsLongFlag(const char *arg); /* --help */
NOCH_DEF bool argIsFlagsEnd(const char *arg); /* -- */

#define DECL_FLAG_FUNC(POSTFIX, TYPE)                                        \
	NOCH_DEF void flag##POSTFIX(const char *shortName, const char *longName, \
	                             const char *desc, TYPE *var)

DECL_FLAG_FUNC(String, const char*);
DECL_FLAG_FUNC(Char,   char);
DECL_FLAG_FUNC(Int,    int);
DECL_FLAG_FUNC(Size,   size_t);
DECL_FLAG_FUNC(Num,    double);
DECL_FLAG_FUNC(Bool,   bool);

#undef DECL_FLAG_FUNC

typedef struct {
	size_t       c;
	const char **v;
	char       **base;
} Args;

NOCH_DEF Args argsNew(int argc, const char **argv);

#define FOREACH_IN_ARGS(THIS, VAR, BODY)                            \
	do {                                                            \
		for (size_t nochIt_ = 0; nochIt_ < (THIS)->c; ++ nochIt_) { \
			const char *VAR = (THIS)->v[nochIt_];                   \
			BODY                                                    \
		}                                                           \
	} while (0)

NOCH_DEF const char *argsShift     (Args *args);
NOCH_DEF int         argsParseFlags(Args *args, Args *stripped);

NOCH_DEF void flagsUsage(FILE *file);
NOCH_DEF void argsUsage (FILE *file, const char *name, const char **usages,
                         size_t usagesCount, const char *desc, bool printFlags);

#ifdef __cplusplus
}
#endif
#endif
