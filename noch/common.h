#ifndef NOCH_COMMON_H_HEADER_GUARD
#define NOCH_COMMON_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdlib.h> /* exit, EXIT_FAILURE */
#include <stdio.h>  /* fprintf, stderr */
#include <string.h> /* memset, strlen, strcpy */

#include "internal/def.h"
#include "internal/err.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define UNUSED(X)      (void)X
#define ARRAY_LEN(X)   (sizeof(X) / sizeof(*(X)))
#define ZERO_STRUCT(X) memset(X, 0, sizeof(*(X)))

#define SWAP(A, B, T) \
	do {              \
		T _tmp = A;   \
		A = B;        \
		B = _tmp;     \
	} while (0)

#define UNREACHABLE()                                             \
	(fprintf(stderr, "%s:%i: Unreachable\n", __FILE__, __LINE__), \
	 exit(EXIT_FAILURE))

#define TODO(...)                                          \
	(fprintf(stderr, "%s:%i: TODO: ", __FILE__, __LINE__), \
	 fprintf(stderr, __VA_ARGS__),                         \
	 fprintf(stderr, "\n"),                                \
	 exit(EXIT_FAILURE))

NOCH_DEF char *xstrdup(const char *str);

NOCH_DEF void fputn(double num, FILE *file);
NOCH_DEF void putn (double num);

#ifdef __cplusplus
}
#endif
#endif
