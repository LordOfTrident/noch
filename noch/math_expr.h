#ifndef NOCH_MATH_EXPR_H_HEADER_GUARD
#define NOCH_MATH_EXPR_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdio.h>  /* fprintf, FILE */
#include <string.h> /* strcmp, memset, memcpy, strlen, strcpy */
#include <ctype.h>  /* isspace, isdigit, isalnum, isalpha */
#include <stdlib.h> /* atof */
#include <math.h>   /* pow, floor */

#include "internal/def.h"

#ifdef NAN
#	define ME_NAN NAN
#else
#	define ME_NAN (0.0 / 0.0)
#endif

NOCH_DEF double me_eval(const char *in, size_t *row, size_t *col);

typedef enum {
	ME_EXPR_NUM = 0,

	ME_EXPR_UNARY,
	ME_EXPR_BINARY,

	ME_EXPR_ID,
	ME_EXPR_FN,
} me_expr_type_t;

typedef struct {
	me_expr_type_t type;
	bool           parens;
} me_expr_t;

typedef struct {
	me_expr_t _;

	double val;
} me_expr_num_t;

typedef struct {
	me_expr_t _;

	char       op;
	me_expr_t *expr;
} me_expr_unary_t;

typedef struct {
	me_expr_t _;

	char       op;
	me_expr_t *a, *b;
} me_expr_binary_t;

#ifndef ME_TOK_CAP
#	define ME_TOK_CAP 128
#endif

typedef struct {
	me_expr_t _;

	char val[ME_TOK_CAP];
} me_expr_id_t;

#ifndef ME_MAX_ARGS
#	define ME_MAX_ARGS 8
#endif

typedef struct {
	me_expr_t _;

	char       name[ME_TOK_CAP];
	me_expr_t *args[ME_MAX_ARGS];
	size_t     argc;
} me_expr_fn_t;

NOCH_DEF me_expr_t *me_parse_expr  (const char *in, size_t *row, size_t *col);
NOCH_DEF void       me_expr_fprint (me_expr_t  *this, FILE *file);
NOCH_DEF void       me_expr_destroy(me_expr_t  *this);

typedef enum {
	ME_STMT_EXPR = 0,

	ME_STMT_VAR,
	ME_STMT_DEF,
} me_stmt_type_t;

typedef struct {
	me_stmt_type_t type;
} me_stmt_t;

typedef struct {
	me_stmt_t _;

	me_expr_t *expr;
} me_stmt_expr_t;

typedef struct {
	me_stmt_t _;

	me_expr_id_t *id;
	me_expr_t    *expr;
} me_stmt_var_t;

typedef struct {
	me_stmt_t _;

	me_expr_fn_t *fn;
	me_expr_t    *expr;
} me_stmt_def_t;

NOCH_DEF me_stmt_t *me_parse_stmt  (const char *in, size_t *row, size_t *col);
NOCH_DEF void       me_stmt_fprint (me_stmt_t  *this, FILE *file);
NOCH_DEF void       me_stmt_destroy(me_stmt_t  *this);

typedef struct {
	double val;
	bool   none;

	/* Error info */
	const char *origin, *err;
	/* WARNING: origin is the name of an identifier/function that caused the error and WILL be
	   invalid after the expr tree is destroyed */
} me_result_t;

typedef me_result_t (*me_native_t)(double*, size_t);

typedef struct {
	char        name[ME_TOK_CAP];
	me_native_t native;
} me_ctx_fn_t;

#ifndef ME_CTX_FNS_CHUNK_SIZE
#	define ME_CTX_FNS_CHUNK_SIZE 16
#endif

typedef struct {
	char   name[ME_TOK_CAP];
	double val;
	bool   is_const;
} me_ctx_id_t;

#ifndef ME_CTX_IDS_CHUNK_SIZE
#	define ME_CTX_IDS_CHUNK_SIZE 16
#endif

typedef struct {
	me_ctx_fn_t *fns;
	size_t       fns_count, fns_cap;

	me_ctx_id_t *ids;
	size_t       ids_count, ids_cap;
} me_ctx_t;

NOCH_DEF void me_ctx_init  (me_ctx_t *this);
NOCH_DEF void me_ctx_deinit(me_ctx_t *this);

NOCH_DEF void me_ctx_set_fn(me_ctx_t *this, const char *name, me_native_t native);
NOCH_DEF void me_ctx_set_id(me_ctx_t *this, const char *name, double val, bool is_const);

NOCH_DEF me_ctx_fn_t *me_ctx_get_fn(me_ctx_t *this, const char *name);
NOCH_DEF me_ctx_id_t *me_ctx_get_id(me_ctx_t *this, const char *name);

NOCH_DEF me_result_t me_result_ok(double val);
NOCH_DEF me_result_t me_result_err(const char *origin, const char *err);
NOCH_DEF me_result_t me_result_none(void);

#define ME_INVALID_AMOUNT_OF_ARGS me_result_err(NULL, "Invalid amount of arguments")
#define ME_DIV_BY_ZERO            me_result_err(NULL, "Division by zero")

NOCH_DEF me_result_t me_ctx_eval_expr(me_ctx_t *this, me_expr_t *expr);
NOCH_DEF me_result_t me_ctx_eval_stmt(me_ctx_t *this, me_stmt_t *stmt);

/* Pre-defined native functions for me_ctx */

#define ME_DECL_NATIVE(NAME) NOCH_DEF me_result_t NAME(double *args, size_t argc)

ME_DECL_NATIVE(me_sqrt);
ME_DECL_NATIVE(me_cbrt);
ME_DECL_NATIVE(me_hypot);
ME_DECL_NATIVE(me_sin);
ME_DECL_NATIVE(me_cos);
ME_DECL_NATIVE(me_tan);
ME_DECL_NATIVE(me_log);
ME_DECL_NATIVE(me_floor);
ME_DECL_NATIVE(me_ceil);
ME_DECL_NATIVE(me_round);
ME_DECL_NATIVE(me_atan);
ME_DECL_NATIVE(me_atan2);
ME_DECL_NATIVE(me_root);
ME_DECL_NATIVE(me_abs);
ME_DECL_NATIVE(me_pow);
ME_DECL_NATIVE(me_sum);

#undef ME_DECL_NATIVE

NOCH_DEF void me_ctx_register_basic_fns(me_ctx_t *this);

#ifdef __cplusplus
}
#endif
#endif
