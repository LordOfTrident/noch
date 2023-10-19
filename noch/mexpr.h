#ifndef NOCH_MEXPR_H_HEADER_GUARD
#define NOCH_MEXPR_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdio.h>  /* fprintf, FILE */
#include <string.h> /* strcmp, memset, strlen, strcpy */
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
	size_t   argc;
} mexpr_fn_t;

typedef struct {
	double val;

	/* Error info */
	const char *origin, *err;
	/* WARNING: origin is the name of an identifier/function that caused the error and WILL be
	   invalid after the expr tree is destroyed */
} mctx_result_t;

typedef mctx_result_t (*mctx_native_t)(double*, size_t);

#define MCTX_NATIVE_VARGS (size_t)-1

typedef struct {
	const char   *name;
	mctx_native_t fn;
} mctx_fn_t;

typedef struct {
	const char *name;
	double      val;
} mctx_const_t;

#ifndef MCTX_FNS_CHUNK_SIZE
#	define MCTX_FNS_CHUNK_SIZE 16
#endif

#ifndef MCTX_CONSTS_CHUNK_SIZE
#	define MCTX_CONSTS_CHUNK_SIZE 16
#endif

typedef struct {
	mctx_fn_t *fns;
	size_t     fns_count, fns_cap;

	mctx_const_t *consts;
	size_t        consts_count, consts_cap;
} mctx_t;

NOCH_DEF void mctx_init  (mctx_t *this);
NOCH_DEF void mctx_deinit(mctx_t *this);

NOCH_DEF void mctx_register_fn   (mctx_t *this, const char *name, mctx_native_t fn);
NOCH_DEF void mctx_register_const(mctx_t *this, const char *name, double val);

NOCH_DEF mctx_result_t mctx_ok(double val);
NOCH_DEF mctx_result_t mctx_err(const char *origin, const char *err);

#define MCTX_INVALID_AMOUNT_OF_ARGS mctx_err(NULL, "Invalid amount of arguments")
#define MCTX_DIV_BY_ZERO            mctx_err(NULL, "Division by zero")

NOCH_DEF mctx_result_t mctx_eval(mctx_t *this, mexpr_t *expr);

NOCH_DEF mexpr_t *mexpr_parse(const char *in, size_t *row, size_t *col);
NOCH_DEF void     mexpr_fprint(mexpr_t *this, FILE *file);
NOCH_DEF void     mexpr_destroy(mexpr_t *this);

/* Pre-defined native functions for mctx */

#define DECL_MCTX_NATIVE(NAME) NOCH_DEF mctx_result_t NAME(double *args, size_t argc)

DECL_MCTX_NATIVE(mctx_fn_sqrt);
DECL_MCTX_NATIVE(mctx_fn_cbrt);
DECL_MCTX_NATIVE(mctx_fn_hypot);
DECL_MCTX_NATIVE(mctx_fn_sin);
DECL_MCTX_NATIVE(mctx_fn_cos);
DECL_MCTX_NATIVE(mctx_fn_tan);
DECL_MCTX_NATIVE(mctx_fn_log);
DECL_MCTX_NATIVE(mctx_fn_floor);
DECL_MCTX_NATIVE(mctx_fn_ceil);
DECL_MCTX_NATIVE(mctx_fn_round);
DECL_MCTX_NATIVE(mctx_fn_atan);
DECL_MCTX_NATIVE(mctx_fn_atan2);
DECL_MCTX_NATIVE(mctx_fn_root);

NOCH_DEF void mctx_register_basic_fns(mctx_t *this);

#undef DECL_MCTX_NATIVE

#ifdef __cplusplus
}
#endif
#endif
