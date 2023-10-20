#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"
#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "mexpr.h"

#define mexpr_new_num            NOCH_PRIV(mexpr_new_num)
#define mexpr_new_unary          NOCH_PRIV(mexpr_new_unary)
#define mexpr_new_binary         NOCH_PRIV(mexpr_new_binary)
#define mexpr_new_id             NOCH_PRIV(mexpr_new_id)
#define mexpr_new_fn             NOCH_PRIV(mexpr_new_fn)
#define mexpr_tok_t              NOCH_PRIV(mexpr_tok_t)
#define mparser_t                NOCH_PRIV(mparser_t)
#define mparser_data_clear       NOCH_PRIV(mparser_data_clear)
#define mparser_data_add         NOCH_PRIV(mparser_data_add)
#define mparser_skip_ws          NOCH_PRIV(mparser_skip_ws)
#define mparser_tok_start_here   NOCH_PRIV(mparser_tok_start_here)
#define mparser_tok_single       NOCH_PRIV(mparser_tok_single)
#define mparser_tok_num          NOCH_PRIV(mparser_tok_num)
#define mparser_tok_id           NOCH_PRIV(mparser_tok_id)
#define mparser_advance          NOCH_PRIV(mparser_advance)
#define mexpr_tok_to_op          NOCH_PRIV(mexpr_tok_to_op)
#define mparser_parse_arith      NOCH_PRIV(mparser_parse_arith)
#define mparser_parse_term       NOCH_PRIV(mparser_parse_term)
#define mparser_parse_pow        NOCH_PRIV(mparser_parse_pow)
#define mparser_parse_factor     NOCH_PRIV(mparser_parse_factor)
#define mparser_parse_unary      NOCH_PRIV(mparser_parse_unary)
#define mparser_parse_paren      NOCH_PRIV(mparser_parse_paren)
#define mparser_parse_pipe       NOCH_PRIV(mparser_parse_pipe)
#define mparser_parse_id         NOCH_PRIV(mparser_parse_id)
#define mparser_parse_num        NOCH_PRIV(mparser_parse_num)
#define mparser_expect_input_end NOCH_PRIV(mparser_expect_input_end)
#define mparser_init             NOCH_PRIV(mparser_init)
#define mparser_deinit           NOCH_PRIV(mparser_deinit)
#define mparser_err              NOCH_PRIV(mparser_err)
#define mparser_col              NOCH_PRIV(mparser_col)
#define mexpr_fprint_float       NOCH_PRIV(mexpr_fprint_float)
#define mexpr_mod                NOCH_PRIV(mexpr_mod)
#define mexpr_div                NOCH_PRIV(mexpr_div)

#define MEXPR_ALLOC(TO, TYPE)         \
	do {                              \
		NOCH_MUST_ALLOC(TYPE, TO, 1); \
		memset(TO, 0, sizeof(TYPE));  \
	} while (0)

static mexpr_num_t *NOCH_PRIV(mexpr_new_num)(double val) {
	mexpr_num_t *num;
	MEXPR_ALLOC(num, mexpr_num_t);
	num->_.type = MEXPR_NUM;

	num->val = val;
	return num;
}

static mexpr_unary_t *NOCH_PRIV(mexpr_new_unary)(char op, mexpr_t *expr) {
	mexpr_unary_t *un;
	MEXPR_ALLOC(un, mexpr_unary_t);
	un->_.type = MEXPR_UNARY;

	un->op   = op;
	un->expr = expr;
	return un;
}

static mexpr_binary_t *NOCH_PRIV(mexpr_new_binary)(char op, mexpr_t *a, mexpr_t *b) {
	mexpr_binary_t *bin;
	MEXPR_ALLOC(bin, mexpr_binary_t);
	bin->_.type = MEXPR_BINARY;

	bin->op = op;
	bin->a  = a;
	bin->b  = b;
	return bin;
}

static mexpr_id_t *NOCH_PRIV(mexpr_new_id)(const char *val) {
	mexpr_id_t *id;
	MEXPR_ALLOC(id, mexpr_id_t);
	id->_.type = MEXPR_ID;

	NOCH_ASSERT(strlen(val) < MEXPR_TOK_CAP);
	strcpy(id->val, val);
	return id;
}

static mexpr_fn_t *NOCH_PRIV(mexpr_new_fn)(const char *name) {
	mexpr_fn_t *fn;
	MEXPR_ALLOC(fn, mexpr_fn_t);
	fn->_.type = MEXPR_FN;

	NOCH_ASSERT(strlen(name) < MEXPR_TOK_CAP);
	strcpy(fn->name, name);
	return fn;
}

NOCH_DEF void mexpr_destroy(mexpr_t *this) {
	NOCH_ASSERT(this != NULL);

	switch (this->type) {
	case MEXPR_NUM: break;

	case MEXPR_UNARY:
		mexpr_destroy(((mexpr_unary_t*)this)->expr);
		break;

	case MEXPR_BINARY: {
		mexpr_binary_t *bin = (mexpr_binary_t*)this;
		mexpr_destroy(bin->a);
		mexpr_destroy(bin->b);
	} break;

	case MEXPR_ID: break;

	case MEXPR_FN: {
		mexpr_fn_t *fn = (mexpr_fn_t*)this;
		for (size_t i = 0; i < fn->argc; ++ i)
			mexpr_destroy(fn->args[i]);
	} break;

	default: NOCH_ASSERT(0 && "Unknown mexpr type");
	}

	NOCH_FREE(this);
}

typedef enum {
	MEXPR_TOK_EOI = 0,

	MEXPR_TOK_NUM,
	MEXPR_TOK_ID,

	MEXPR_TOK_PIPE,

	MEXPR_TOK_LPAREN,
	MEXPR_TOK_RPAREN,

	MEXPR_TOK_LSQUARE,
	MEXPR_TOK_RSQUARE,

	MEXPR_TOK_COMMA,

	MEXPR_TOK_ADD,
	MEXPR_TOK_SUB,
	MEXPR_TOK_MUL,
	MEXPR_TOK_DIV,
	MEXPR_TOK_MOD,
	MEXPR_TOK_POW,
} NOCH_PRIV(mexpr_tok_t);

/* mexpr parser structure */
typedef struct {
	const char *in, *it, *bol; /* input, iterator, beginning of line */
	size_t      row;

	mexpr_tok_t tok;
	size_t      tok_row, tok_col;

	bool   prefixed_with_ws;
	char   data[MEXPR_TOK_CAP];
	size_t data_size;

	size_t err_row, err_col;
} NOCH_PRIV(mparser_t);

static void NOCH_PRIV(mparser_init)(mparser_t *this, const char *in) {
	memset(this, 0, sizeof(*this));

	this->in  = in;
	this->it  = in;
	this->bol = this->it;
	this->row = 1;
}

#define MEXPR_STR(X)   __MEXPR_STR(X)
#define __MEXPR_STR(X) #X

inline static int NOCH_PRIV(mparser_err)(mparser_t *this, const char *msg, size_t row, size_t col) {
	this->err_row = row;
	this->err_col = col;
	NOCH_PARSER_ERR(msg);
	return -1;
}

inline static size_t NOCH_PRIV(mparser_col)(mparser_t *this) {
	return this->it - this->bol + 1;
}

static void NOCH_PRIV(mparser_data_clear)(mparser_t *this) {
	this->data[0]   = '\0';
	this->data_size = 0;
}

static int NOCH_PRIV(mparser_data_add)(mparser_t *this, char ch) {
	if (this->data_size + 1 >= MEXPR_TOK_CAP)
		return mparser_err(this, "Token exceeded maximum length of " MEXPR_STR(MEXPR_TOK_CAP - 1),
		                   this->row, mparser_col(this));

	this->data[this->data_size ++] = ch;
	this->data[this->data_size]    = '\0';
	return 0;
}

static void NOCH_PRIV(mparser_tok_start_here)(mparser_t *this) {
	this->tok_row = this->row;
	this->tok_col = mparser_col(this);
}

static int NOCH_PRIV(mparser_tok_single)(mparser_t *this, mexpr_tok_t tok) {
	mparser_data_clear(this);
	mparser_tok_start_here(this);
	this->tok = tok;

	++ this->it;
	return 0;
}

static int NOCH_PRIV(mparser_tok_num)(mparser_t *this) {
	NOCH_ASSERT(isdigit(*this->it));

	mparser_data_clear(this);
	mparser_tok_start_here(this);
	this->tok = MEXPR_TOK_NUM;

	bool exponent = false, fpoint = false;

	while (true) {
		if (*this->it == '_') {
			++ this->it;
			continue;
		} else if (*this->it == 'e') {
			if (exponent)
				return mparser_err(this, "Encountered exponent in number twice",
				                   this->row, mparser_col(this));
			else
				exponent = true;

			if (this->it[1] == '+' || this->it[1] == '-') {
				if (mparser_data_add(this, *this->it ++) != 0)
					return -1;
			}

			if (!isdigit(this->it[1]))
				return mparser_err(this, "Expected a number", this->row, mparser_col(this) + 1);
		} else if (*this->it == '.') {
			if (exponent)
				return mparser_err(this, "Unexpected floating point in exponent",
				                   this->row, mparser_col(this));
			else if (fpoint)
				return mparser_err(this, "Encountered a floating point in number twice",
				                   this->row, mparser_col(this));
			else {
				fpoint = true;

				if (!isdigit(this->it[1]))
					return mparser_err(this, "Expected a number", this->row, mparser_col(this) + 1);
			}
		} else if (!isdigit(*this->it))
			break;

		if (mparser_data_add(this, *this->it ++) != 0)
			return -1;
	}

	return 0;
}

static int NOCH_PRIV(mparser_tok_id)(mparser_t *this) {
	NOCH_ASSERT(isalpha(*this->it));

	mparser_data_clear(this);
	mparser_tok_start_here(this);
	this->tok = MEXPR_TOK_ID;

	while (isalnum(*this->it)) {
		if (mparser_data_add(this, *this->it) != 0)
			return -1;

		++ this->it;
	}

	return 0;
}

static void NOCH_PRIV(mparser_skip_ws)(mparser_t *this) {
	this->prefixed_with_ws = false;

	while (*this->it != '\0') {
		if (!isspace(*this->it))
			break;

		if (!this->prefixed_with_ws)
			this->prefixed_with_ws = true;

		if (*this->it == '\n') {
			++ this->row;
			this->bol = this->it + 1;
		}

		++ this->it;
	}
}

static int NOCH_PRIV(mparser_advance)(mparser_t *this) {
	mparser_skip_ws(this);

	if (*this->it == '\0') {
		this->tok     = MEXPR_TOK_EOI;
		this->tok_row = this->row;
		this->tok_col = mparser_col(this);
		return 0;
	}

	switch (*this->it) {
	case '|': return mparser_tok_single(this, MEXPR_TOK_PIPE);
	case '(': return mparser_tok_single(this, MEXPR_TOK_LPAREN);
	case ')': return mparser_tok_single(this, MEXPR_TOK_RPAREN);
	case '[': return mparser_tok_single(this, MEXPR_TOK_LSQUARE);
	case ']': return mparser_tok_single(this, MEXPR_TOK_RSQUARE);
	case ',': return mparser_tok_single(this, MEXPR_TOK_COMMA);
	case '+': return mparser_tok_single(this, MEXPR_TOK_ADD);
	case '-': return mparser_tok_single(this, MEXPR_TOK_SUB);
	case '*': return mparser_tok_single(this, MEXPR_TOK_MUL);
	case '/': return mparser_tok_single(this, MEXPR_TOK_DIV);
	case '%': return mparser_tok_single(this, MEXPR_TOK_MOD);
	case '^': return mparser_tok_single(this, MEXPR_TOK_POW);

	default:
		if (isdigit(*this->it))
			return mparser_tok_num(this);
		else if (isalpha(*this->it))
			return mparser_tok_id(this);
		else
			return mparser_err(this, "Unexpected character", this->row, mparser_col(this));
	}
}

static char NOCH_PRIV(mexpr_tok_to_op)(mexpr_tok_t tok) {
	switch (tok) {
	case MEXPR_TOK_ADD:  return '+';
	case MEXPR_TOK_SUB:  return '-';
	case MEXPR_TOK_MUL:  return '*';
	case MEXPR_TOK_DIV:  return '/';
	case MEXPR_TOK_MOD:  return '%';
	case MEXPR_TOK_POW:  return '^';
	case MEXPR_TOK_PIPE: return '|';

	default: NOCH_ASSERT(0 && "Token not an operator");
	}
}

static mexpr_t *NOCH_PRIV(mparser_parse_factor)(mparser_t *this);
static mexpr_t *NOCH_PRIV(mparser_parse_arith) (mparser_t *this);

static mexpr_t *NOCH_PRIV(mparser_parse_pipe)(mparser_t *this) {
	char op = mexpr_tok_to_op(this->tok);

	if (mparser_advance(this) != 0)
		return NULL;

	mexpr_t *expr = mparser_parse_arith(this);
	if (expr == NULL)
		return NULL;

	if (this->tok != MEXPR_TOK_PIPE) {
		mexpr_destroy(expr);
		mparser_err(this, "Expected a matching \"|\"", this->row, mparser_col(this));
		return NULL;
	}

	if (mparser_advance(this) != 0) {
		mexpr_destroy(expr);
		return NULL;
	}

	return (mexpr_t*)mexpr_new_unary(op, expr);
}

static mexpr_t *NOCH_PRIV(mparser_parse_paren)(mparser_t *this) {
	mexpr_tok_t end = this->tok + 1;

	if (mparser_advance(this) != 0)
		return NULL;

	mexpr_t *expr = mparser_parse_arith(this);
	if (expr == NULL)
		return NULL;

	if (this->tok != end) {
		mexpr_destroy(expr);
		const char *msg = end == MEXPR_TOK_LPAREN?
		                  "Expected a matching \")\"" : "Expected a matching \"]\"";
		mparser_err(this, msg, this->row, mparser_col(this));
		return NULL;
	}

	if (mparser_advance(this) != 0) {
		mexpr_destroy(expr);
		return NULL;
	}

	return expr;
}

static mexpr_t *NOCH_PRIV(mparser_parse_unary)(mparser_t *this) {
	char op = mexpr_tok_to_op(this->tok);

	if (mparser_advance(this) != 0)
		return NULL;

	mexpr_t *expr = mparser_parse_factor(this);
	if (expr == NULL)
		return NULL;

	return (mexpr_t*)mexpr_new_unary(op, expr);
}

static mexpr_t *NOCH_PRIV(mparser_parse_num)(mparser_t *this) {
	double num = atof(this->data);
	if (mparser_advance(this) != 0)
		return NULL;

	return (mexpr_t*)mexpr_new_num(num);
}

static mexpr_t *NOCH_PRIV(mparser_parse_id)(mparser_t *this) {
	char name[MEXPR_TOK_CAP];
	strcpy(name, this->data);

	if (mparser_advance(this) != 0)
		return NULL;

	if (this->tok == MEXPR_TOK_LPAREN) {
		if (mparser_advance(this) != 0)
			return NULL;

		mexpr_fn_t *fn = mexpr_new_fn(name);

		if (this->tok == MEXPR_TOK_RPAREN) {
			if (mparser_advance(this) != 0) {
				mexpr_destroy((mexpr_t*)fn);
				return NULL;
			}

			return (mexpr_t*)fn;
		}

		while (true) {
			if (fn->argc >= MEXPR_MAX_ARGS) {
				mparser_err(this, "Exceeded maximum amount of " MEXPR_STR(MEXPR_MAX_ARGS)
				            " function arguments", this->row, mparser_col(this));
				return NULL;
			}

			mexpr_t *expr = mparser_parse_arith(this);
			fn->args[fn->argc ++] = expr;
			if (expr == NULL) {
				mexpr_destroy((mexpr_t*)fn);
				return NULL;
			}

			mexpr_tok_t tok = this->tok;
			if (mparser_advance(this) != 0) {
				mexpr_destroy((mexpr_t*)fn);
				return NULL;
			}

			if (tok == MEXPR_TOK_RPAREN)
				break;
			else if (tok != MEXPR_TOK_COMMA) {
				mparser_err(this, "Expected a \",\"", this->row, mparser_col(this));
				return NULL;
			}
		}

		return (mexpr_t*)fn;
	} else
		return (mexpr_t*)mexpr_new_id(name);
}

static mexpr_t *NOCH_PRIV(mparser_parse_factor)(mparser_t *this) {
	if (this->it == this->in) {
		if (mparser_advance(this) != 0)
			return NULL;
	}

	switch (this->tok) {
	case MEXPR_TOK_EOI:
		mparser_err(this, "Unexpected end of input", this->tok_row, this->tok_col);
		break;

	case MEXPR_TOK_NUM:  return mparser_parse_num(this);
	case MEXPR_TOK_ID:   return mparser_parse_id(this);
	case MEXPR_TOK_PIPE: return mparser_parse_pipe(this);

	case MEXPR_TOK_ADD:    case MEXPR_TOK_SUB:     return mparser_parse_unary(this);
	case MEXPR_TOK_LPAREN: case MEXPR_TOK_LSQUARE: return mparser_parse_paren(this);

	default: mparser_err(this, "Unexpected token", this->tok_row, this->tok_col);
	}

	return NULL;
}

static mexpr_t *NOCH_PRIV(mparser_parse_pow)(mparser_t *this) {
	mexpr_t *left = mparser_parse_factor(this);
	if (left == NULL)
		return NULL;

	while (this->tok == MEXPR_TOK_POW) {
		char op = mexpr_tok_to_op(this->tok);

		if (mparser_advance(this) != 0)
			goto fail;

		mexpr_t *right = mparser_parse_pow(this);
		if (right == NULL)
			goto fail;

		left = (mexpr_t*)mexpr_new_binary(op, left, right);
	}

	return left;

fail:
	mexpr_destroy(left);
	return NULL;
}

static mexpr_t *NOCH_PRIV(mparser_parse_term)(mparser_t *this) {
	mexpr_t *left = mparser_parse_pow(this);
	if (left == NULL)
		return NULL;

	while (this->tok == MEXPR_TOK_MUL || this->tok == MEXPR_TOK_DIV || this->tok == MEXPR_TOK_MOD ||
	       this->tok == MEXPR_TOK_ID  || this->tok == MEXPR_TOK_LPAREN) {
		char op;
		if (this->tok == MEXPR_TOK_ID || this->tok == MEXPR_TOK_LPAREN) {
			if (this->prefixed_with_ws) {
				if (this->tok == MEXPR_TOK_ID && strcmp(this->data, "x") == 0) {
					if (mparser_advance(this) != 0)
						goto fail;
				} else
					break;
			}

			op = '*';
		} else {
			op = mexpr_tok_to_op(this->tok);
			if (mparser_advance(this) != 0)
				goto fail;
		}

		mexpr_t *right = mparser_parse_pow(this);
		if (right == NULL)
			goto fail;

		left = (mexpr_t*)mexpr_new_binary(op, left, right);
	}

	return left;

fail:
	mexpr_destroy(left);
	return NULL;
}

static mexpr_t *NOCH_PRIV(mparser_parse_arith)(mparser_t *this) {
	mexpr_t *left = mparser_parse_term(this);
	if (left == NULL)
		return NULL;

	while (this->tok == MEXPR_TOK_ADD || this->tok == MEXPR_TOK_SUB) {
		char op = mexpr_tok_to_op(this->tok);

		if (mparser_advance(this) != 0)
			goto fail;

		mexpr_t *right = mparser_parse_term(this);
		if (right == NULL)
			goto fail;

		left = (mexpr_t*)mexpr_new_binary(op, left, right);
	}

	return left;

fail:
	mexpr_destroy(left);
	return NULL;
}

static int NOCH_PRIV(mparser_expect_input_end)(mparser_t *this) {
	if (this->tok != MEXPR_TOK_EOI)
		return mparser_err(this, "Expected an operator between tokens",
		                   this->tok_row, this->tok_col);

	return 0;
}

NOCH_DEF mexpr_t *mexpr_parse(const char *in, size_t *row, size_t *col) {
	NOCH_ASSERT(in != NULL);

	mparser_t parser_, *parser = &parser_;
	mparser_init(parser, in);

	mexpr_t *expr = mparser_parse_arith(parser);
	if (expr != NULL) {
		if (mparser_expect_input_end(parser) != 0) {
			mexpr_destroy(expr);
			expr = NULL;
		}
	}

	if (row != NULL) *row = parser->err_row;
	if (col != NULL) *col = parser->err_col;
	return expr;
}

static mctx_result_t NOCH_PRIV(mexpr_div)(double a, double b) {
	if (b == 0)
		return mctx_err("0", "Division by zero");

	return mctx_ok(a / b);
}

static mctx_result_t NOCH_PRIV(mexpr_mod)(double a, double b) {
	if (b == 0)
		return mctx_err("0", "Division by zero");

	float remainder = a / b;
	return mctx_ok(b * (remainder - floor(remainder)));
}

NOCH_DEF mctx_result_t mctx_ok(double val) {
	mctx_result_t result = {0};
	result.val = val;
	return result;
}

NOCH_DEF mctx_result_t mctx_err(const char *origin, const char *err) {
	mctx_result_t result = {0};
	result.origin = origin;
	result.err    = err;
	return result;
}

#define MCTX_MUST_EVAL(THIS, EXPR, TO)                 \
	do {                                               \
		mctx_result_t _result = mctx_eval(THIS, EXPR); \
		if (_result.err != NULL)                       \
			return _result;                            \
		TO = _result.val;                              \
	} while (0)

NOCH_DEF mctx_result_t mctx_eval(mctx_t *this, mexpr_t *expr) {
	switch (expr->type) {
	case MEXPR_NUM: return mctx_ok(((mexpr_num_t*)expr)->val);

	case MEXPR_UNARY: {
		mexpr_unary_t *un = (mexpr_unary_t*)expr;
		double num;
		MCTX_MUST_EVAL(this, un->expr, num);

		switch (un->op) {
		case '+': return mctx_ok(num);
		case '-': return mctx_ok(-num);
		case '|': return mctx_ok(fabs(num));

		default: NOCH_ASSERT(0 && "Unknown unary operator");
		}
	}

	case MEXPR_BINARY: {
		mexpr_binary_t *bin = (mexpr_binary_t*)expr;
		double a, b;
		MCTX_MUST_EVAL(this, bin->a, a);
		MCTX_MUST_EVAL(this, bin->b, b);

		switch (bin->op) {
		case '+': return mctx_ok(a + b);
		case '-': return mctx_ok(a - b);
		case '*': return mctx_ok(a * b);
		case '/': return mexpr_div(a, b);
		case '%': return mexpr_mod(a, b);
		case '^': return mctx_ok(pow(a, b));

		default: NOCH_ASSERT(0 && "Unknown binary operator");
		}
	} break;

	case MEXPR_ID: {
		mexpr_id_t *id = (mexpr_id_t*)expr;

		/* TODO: Maybe a hash map? */
		for (size_t i = 0; i < this->consts_count; ++ i) {
			if (strcmp(this->consts[i].name, id->val) == 0)
				return mctx_ok(this->consts[i].val);
		}

		return mctx_err(id->val, "Undefined identifier");
	}


	case MEXPR_FN: {
		mexpr_fn_t *fn = (mexpr_fn_t*)expr;
		for (size_t i = 0; i < this->fns_count; ++ i) {
			if (strcmp(this->fns[i].name, fn->name) == 0) {
				double args[MEXPR_MAX_ARGS];
				for (size_t i = 0; i < fn->argc; ++ i)
					MCTX_MUST_EVAL(this, fn->args[i], args[i]);

				mctx_result_t result = this->fns[i].fn(args, fn->argc);
				if (result.origin == NULL)
					result.origin = fn->name;

				return result;
			}
		}

		return mctx_err(fn->name, "Undefined function");
	}

	default: NOCH_ASSERT(0 && "Unknown expression type");
	}
}

#undef MCTX_MUST_EVAL

static void NOCH_PRIV(mexpr_fprint_float)(double num, FILE *file) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%.13f", num);

	bool   found = false;
	size_t i;
	for (i = 0; buf[i] != '\0'; ++ i) {
		if (buf[i] == '.' && !found)
			found = true;
	}
	if (!found) {
		fprintf(file, "%s", buf);
		return;
	} else
		-- i;

	while (true) {
		if (buf[i] != '0') {
			if (buf[i] == '.')
				buf[i] = '\0';

			break;
		}

		buf[i] = '\0';
		-- i;
	}

	fprintf(file, "%s", buf);
}

/* TODO: Do not print redundant paranthesis */
NOCH_DEF void mexpr_fprint(mexpr_t *this, FILE *file) {
	switch (this->type) {
	case MEXPR_NUM:
		mexpr_fprint_float(((mexpr_num_t*)this)->val, file);
		break;

	case MEXPR_ID:
		fprintf(file, "%s", ((mexpr_id_t*)this)->val);
		break;

	case MEXPR_UNARY: {
		mexpr_unary_t *un = (mexpr_unary_t*)this;

		if (un->op == '|') {
			fprintf(file, "|");
			mexpr_fprint(un->expr, file);
			fprintf(file, "|");
		} else {
			fprintf(file, "(%c", un->op);
			mexpr_fprint(un->expr, file);
			fprintf(file, ")");
		}
	} break;

	case MEXPR_BINARY: {
		mexpr_binary_t *bin = (mexpr_binary_t*)this;
		fprintf(file, "(");
		mexpr_fprint(bin->a, file);
		fprintf(file, " %c ", bin->op);
		mexpr_fprint(bin->b, file);
		fprintf(file, ")");
	} break;

	case MEXPR_FN: {
		mexpr_fn_t *fn = (mexpr_fn_t*)this;
		fprintf(file, "%s(", fn->name);
		for (size_t i = 0; i < fn->argc; ++ i) {
			if (i > 0)
				fprintf(file, ", ");

			mexpr_fprint(fn->args[i], file);
		}
		fprintf(file, ")");
	} break;

	default: NOCH_ASSERT(0 && "Unknown expression type");
	}
}

#undef MEXPR_STR
#undef __MEXPR_STR

NOCH_DEF void mctx_init(mctx_t *this) {
	memset(this, 0, sizeof(*this));

	this->fns_cap = MCTX_FNS_CHUNK_SIZE;
	NOCH_MUST_ALLOC(mctx_fn_t, this->fns, this->fns_cap);

	this->consts_cap = MCTX_CONSTS_CHUNK_SIZE;
	NOCH_MUST_ALLOC(mctx_const_t, this->consts, this->consts_cap);
}

NOCH_DEF void mctx_deinit(mctx_t *this) {
	NOCH_FREE(this->fns);
	NOCH_FREE(this->consts);
}

NOCH_DEF void mctx_register_fn(mctx_t *this, const char *name, mctx_native_t fn) {
	for (size_t i = 0; i < this->fns_count; ++ i) {
		if (strcmp(this->fns[i].name, name) == 0) {
			this->fns[i].name = name;
			this->fns[i].fn   = fn;
			return;
		}
	}

	if (this->fns_count >= this->fns_cap) {
		this->fns_cap *= 2;
		NOCH_MUST_REALLOC(mctx_fn_t, this->fns, this->fns_cap);
	}

	this->fns[this->fns_count].name  = name;
	this->fns[this->fns_count ++].fn = fn;
}

NOCH_DEF void mctx_register_const(mctx_t *this, const char *name, double val) {
	for (size_t i = 0; i < this->consts_count; ++ i) {
		if (strcmp(this->consts[i].name, name) == 0) {
			this->consts[i].name = name;
			this->consts[i].val  = val;
			return;
		}
	}

	if (this->consts_count >= this->consts_cap) {
		this->consts_cap *= 2;
		NOCH_MUST_REALLOC(mctx_const_t, this->consts, this->consts_cap);
	}

	this->consts[this->consts_count].name   = name;
	this->consts[this->consts_count ++].val = val;
}

#define BIND_MCTX_NATIVE(NAME, CFN, ARGC, ...)               \
	NOCH_DEF mctx_result_t NAME(double *args, size_t argc) { \
		if (argc != ARGC)                                    \
			return MCTX_INVALID_AMOUNT_OF_ARGS;              \
		return mctx_ok(CFN(__VA_ARGS__));                    \
	}

BIND_MCTX_NATIVE(mctx_fn_sqrt,  sqrt,  1, args[0])
BIND_MCTX_NATIVE(mctx_fn_cbrt,  cbrt,  1, args[0])
BIND_MCTX_NATIVE(mctx_fn_hypot, hypot, 2, args[0], args[1])
BIND_MCTX_NATIVE(mctx_fn_sin,   sin,   1, args[0])
BIND_MCTX_NATIVE(mctx_fn_cos,   cos,   1, args[0])
BIND_MCTX_NATIVE(mctx_fn_tan,   tan,   1, args[0])
BIND_MCTX_NATIVE(mctx_fn_log,   log,   1, args[0])
BIND_MCTX_NATIVE(mctx_fn_floor, floor, 1, args[0])
BIND_MCTX_NATIVE(mctx_fn_ceil,  ceil,  1, args[0])
BIND_MCTX_NATIVE(mctx_fn_round, round, 1, args[0])
BIND_MCTX_NATIVE(mctx_fn_atan,  atan,  1, args[0])
BIND_MCTX_NATIVE(mctx_fn_atan2, atan2, 2, args[0], args[1])
BIND_MCTX_NATIVE(mctx_fn_abs,   fabs,  1, args[0])
BIND_MCTX_NATIVE(mctx_fn_pow,   pow,   2, args[0], args[1])

NOCH_DEF mctx_result_t mctx_fn_root(double *args, size_t argc) {
	if (argc != 2)
		return MCTX_INVALID_AMOUNT_OF_ARGS;

	if (args[1] == 0)
		return MCTX_DIV_BY_ZERO;

	return mctx_ok(pow(args[0], 1 / args[1]));
}

NOCH_DEF mctx_result_t mctx_fn_sum(double *args, size_t argc) {
	double sum = 0;
	for (size_t i = 0; i < argc; ++ i)
		sum += args[i];

	return mctx_ok(sum);
}

#undef BIND_MCTX_NATIVE

NOCH_DEF void mctx_register_basic_fns(mctx_t *this) {
	mctx_register_fn(this, "sqrt",  mctx_fn_sqrt);
	mctx_register_fn(this, "cbrt",  mctx_fn_cbrt);
	mctx_register_fn(this, "hypot", mctx_fn_hypot);
	mctx_register_fn(this, "sin",   mctx_fn_sin);
	mctx_register_fn(this, "cos",   mctx_fn_cos);
	mctx_register_fn(this, "tan",   mctx_fn_tan);
	mctx_register_fn(this, "log",   mctx_fn_log);
	mctx_register_fn(this, "floor", mctx_fn_floor);
	mctx_register_fn(this, "ceil",  mctx_fn_ceil);
	mctx_register_fn(this, "round", mctx_fn_round);
	mctx_register_fn(this, "root",  mctx_fn_root);
	mctx_register_fn(this, "atan",  mctx_fn_atan);
	mctx_register_fn(this, "atan2", mctx_fn_atan2);
	mctx_register_fn(this, "abs",   mctx_fn_abs);
	mctx_register_fn(this, "pow",   mctx_fn_pow);
	mctx_register_fn(this, "sum",   mctx_fn_sum);
}

#undef mexpr_new_num
#undef mexpr_new_unary
#undef mexpr_new_binary
#undef mexpr_new_id
#undef mexpr_new_fn
#undef mexpr_tok_t
#undef mparser_t
#undef mparser_data_clear
#undef mparser_data_add
#undef mparser_skip_ws
#undef mparser_tok_start_here
#undef mparser_tok_single
#undef mparser_tok_num
#undef mparser_tok_id
#undef mparser_advance
#undef mexpr_tok_to_op
#undef mparser_parse_arith
#undef mparser_parse_term
#undef mparser_parse_pow
#undef mparser_parse_factor
#undef mparser_parse_unary
#undef mparser_parse_paren
#undef mparser_parse_id
#undef mparser_parse_num
#undef mparser_expect_input_end
#undef mparser_init
#undef mparser_deinit
#undef mparser_err
#undef mparser_col
#undef mexpr_fprint_float
#undef mexpr_mod
#undef mexpr_div

#ifdef __cplusplus
}
#endif
