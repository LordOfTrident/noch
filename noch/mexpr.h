#ifndef NOCH_MEXPR_H_HEADER_GUARD
#define NOCH_MEXPR_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdio.h>  /* fprintf, FILE */
#include <string.h> /* memset, strlen, strcpy */
#include <ctype.h>  /* isspace, isdigit, isalnum, isalpha */
#include <stdlib.h> /* atof */
#include <math.h>   /* pow, floor */

#include "internal/def.h"

/* TODO: Function and variable definition parsing (they will be statements of type mstmt_t*) */

typedef enum {
	MEXPR_NUM = 0,

	MEXPR_UNARY,
	MEXPR_BINARY,

	MEXPR_ID,
	MEXPR_FN,
} mexpr_type_t;

typedef struct {
	mexpr_type_t type;
} mexpr_t;

typedef struct {
	mexpr_t _;

	double val;
} mexpr_num_t;

typedef struct {
	mexpr_t _;

	char     op;
	mexpr_t *expr;
} mexpr_unary_t;

typedef struct {
	mexpr_t _;

	char     op;
	mexpr_t *a, *b;
} mexpr_binary_t;

#ifndef MEXPR_TOK_CAP
#	define MEXPR_TOK_CAP 128
#endif

typedef struct {
	mexpr_t _;

	char val[MEXPR_TOK_CAP];
} mexpr_id_t;

#ifndef MEXPR_MAX_ARGS
#	define MEXPR_MAX_ARGS 8
#endif

typedef struct {
	mexpr_t _;

	char     name[MEXPR_TOK_CAP];
	mexpr_t *args[MEXPR_MAX_ARGS];
	size_t   args_count;
} mexpr_fn_t;

typedef double (*mctx_native_t)(double[MEXPR_MAX_ARGS], size_t);

typedef struct {
	const char   *name;
	mctx_native_t fn;
} mctx_fn_t;

typedef struct {
	mctx_fn_t *fns;
	size_t     fns_count, fns_cap;
} mctx_t;

NOCH_DEF void mctx_init  (mctx_t *this); /* TODO */
NOCH_DEF void mctx_deinit(mctx_t *this); /* TODO */

NOCH_DEF void mctx_add_fn(mctx_t *this, const char *name, mctx_native_t fn); /* TODO */

NOCH_DEF mexpr_t *mexpr_parse(const char *in, size_t *row, size_t *col);
NOCH_DEF double   mexpr_eval   (mexpr_t *this, mctx_t *ctx);
NOCH_DEF void     mexpr_fprint (mexpr_t *this, FILE *file);
NOCH_DEF void     mexpr_destroy(mexpr_t *this);

#ifdef __cplusplus
}
#endif
#endif
