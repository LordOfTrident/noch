#ifndef NOCH_MATHEXPR_H_HEADER_GUARD
#define NOCH_MATHEXPR_H_HEADER_GUARD
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
#include "internal/error.h"

#ifndef NAN
#	define NAN (0.0 / 0.0)
#endif

#ifndef ME_TOKEN_CAPACITY
#	define ME_TOKEN_CAPACITY 64
#endif

#ifndef ME_MAX_ARGS
#	define ME_MAX_ARGS 8
#endif

enum {
	ME_NUMBER = 0,
	ME_UNARY,
	ME_BINARY,
	ME_ID,
	ME_FUNC,
};

typedef struct {
	int    type;
	bool   parens;
	size_t pos;
} MeExpr;

typedef struct {
	MeExpr base;

	double value;
} MeNumber;

enum {
	ME_OP_ADD = '+',
	ME_OP_SUB = '-',
	ME_OP_MUL = '*',
	ME_OP_DIV = '/',
	ME_OP_MOD = '%',
	ME_OP_POW = '^',
	ME_OP_ABS,
};

typedef struct {
	MeExpr base;

	char    op;
	MeExpr *expr;
} MeUnary;

typedef struct {
	MeExpr base;

	char    op;
	MeExpr *left, *right;
} MeBinary;

typedef struct {
	MeExpr base;

	char value[ME_TOKEN_CAPACITY];
} MeId;

typedef struct {
	MeExpr base;

	char    name[ME_TOKEN_CAPACITY];
	MeExpr *args[ME_MAX_ARGS];
	size_t  argsCount;
} MeFunc;

#define ME_NUMBER(EXPR) (NOCH_ASSERT((EXPR)->type == ME_NUMBER), (MeNumber*)(EXPR))
#define ME_UNARY(EXPR)  (NOCH_ASSERT((EXPR)->type == ME_UNARY),  (MeUnary*) (EXPR))
#define ME_BINARY(EXPR) (NOCH_ASSERT((EXPR)->type == ME_BINARY), (MeBinary*)(EXPR))
#define ME_ID(EXPR)     (NOCH_ASSERT((EXPR)->type == ME_ID),     (MeId*)    (EXPR))
#define ME_FUNC(EXPR)   (NOCH_ASSERT((EXPR)->type == ME_FUNC),   (MeFunc*)  (EXPR))

enum {
	ME_INCLUDE_DEFAULT_FUNCS  = 1 << 2,
	ME_INCLUDE_DEFAULT_CONSTS = 1 << 3,

	ME_DEF_CONST = 0,
	ME_DEF_FUNC,
};

typedef double (*MeNative)(MeFunc*, double*, size_t);

typedef struct {
	int  type;
	char name[ME_TOKEN_CAPACITY];
	union {
		MeNative func;
		double   num;
	} u;
} MeDef;

NOCH_DEF MeDef meInclude(int what);
NOCH_DEF MeDef meDefFunc (const char *name, MeNative native);
NOCH_DEF MeDef meDefConst(const char *name, double value);

NOCH_DEF double  meEval        (MeExpr *this, MeDef *defs, size_t size);
NOCH_DEF MeExpr *meEvalLiterals(MeExpr *this);
NOCH_DEF void    mePrintF      (MeExpr *this, FILE *file, bool redundantParens);
NOCH_DEF void    meDestroy     (MeExpr *this);

NOCH_DEF double  meInterp(const char *start, const char *end, MeDef *defs, size_t size);
NOCH_DEF MeExpr *meParse (const char *start, const char *end);

NOCH_DEF double meError(size_t pos, const char *fmt, ...);
NOCH_DEF double meWrongAmountOfArgs(MeFunc *func, size_t expected);

#define meFuncError(FUNC, ...) meError((FUNC)->base.pos, __VA_ARGS__)

/* Default native functions */

#define ME_DECL_NATIVE(NAME) NOCH_DEF double NAME(MeFunc *this, double *args, size_t count)

ME_DECL_NATIVE(meSqrt);
ME_DECL_NATIVE(meCbrt);
ME_DECL_NATIVE(meHypot);
ME_DECL_NATIVE(meSin);
ME_DECL_NATIVE(meCos);
ME_DECL_NATIVE(meTan);
ME_DECL_NATIVE(meLog);
ME_DECL_NATIVE(meFloor);
ME_DECL_NATIVE(meCeil);
ME_DECL_NATIVE(meRound);
ME_DECL_NATIVE(meAtan);
ME_DECL_NATIVE(meAtan2);
ME_DECL_NATIVE(meAbs);
ME_DECL_NATIVE(mePow);
ME_DECL_NATIVE(meRoot);

#undef ME_DECL_NATIVE

#endif
