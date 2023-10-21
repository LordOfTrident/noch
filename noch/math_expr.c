#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"
#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "math_expr.h"

#define me_expr_new_num            NOCH_PRIV(me_expr_new_num)
#define me_expr_new_unary          NOCH_PRIV(me_expr_new_unary)
#define me_expr_new_binary         NOCH_PRIV(me_expr_new_binary)
#define me_expr_new_id             NOCH_PRIV(me_expr_new_id)
#define me_expr_new_fn             NOCH_PRIV(me_expr_new_fn)
#define me_expr_tok_t              NOCH_PRIV(me_expr_tok_t)
#define me_parser_t                NOCH_PRIV(me_parser_t)
#define me_parser_data_clear       NOCH_PRIV(me_parser_data_clear)
#define me_parser_data_add         NOCH_PRIV(me_parser_data_add)
#define me_parser_skip_ws          NOCH_PRIV(me_parser_skip_ws)
#define me_parser_tok_start_here   NOCH_PRIV(me_parser_tok_start_here)
#define me_parser_tok_single       NOCH_PRIV(me_parser_tok_single)
#define me_parser_tok_num          NOCH_PRIV(me_parser_tok_num)
#define me_parser_tok_id           NOCH_PRIV(me_parser_tok_id)
#define me_parser_advance          NOCH_PRIV(me_parser_advance)
#define me_expr_tok_to_op          NOCH_PRIV(me_expr_tok_to_op)
#define me_parser_parse_arith      NOCH_PRIV(me_parser_parse_arith)
#define me_parser_parse_term       NOCH_PRIV(me_parser_parse_term)
#define me_parser_parse_pow        NOCH_PRIV(me_parser_parse_pow)
#define me_parser_parse_factor     NOCH_PRIV(me_parser_parse_factor)
#define me_parser_parse_unary      NOCH_PRIV(me_parser_parse_unary)
#define me_parser_parse_paren      NOCH_PRIV(me_parser_parse_paren)
#define me_parser_parse_pipe       NOCH_PRIV(me_parser_parse_pipe)
#define me_parser_parse_id         NOCH_PRIV(me_parser_parse_id)
#define me_parser_parse_num        NOCH_PRIV(me_parser_parse_num)
#define me_parser_expect_input_end NOCH_PRIV(me_parser_expect_input_end)
#define me_parser_init             NOCH_PRIV(me_parser_init)
#define me_parser_deinit           NOCH_PRIV(me_parser_deinit)
#define me_parser_err              NOCH_PRIV(me_parser_err)
#define me_parser_col              NOCH_PRIV(me_parser_col)
#define me_fprint_float            NOCH_PRIV(me_fprint_float)
#define me_mod                     NOCH_PRIV(me_mod)
#define me_div                     NOCH_PRIV(me_div)
#define me_stmt_new_expr           NOCH_PRIV(me_stmt_new_expr)
#define me_stmt_new_var            NOCH_PRIV(me_stmt_new_var)
#define me_stmt_new_def            NOCH_PRIV(me_stmt_new_def)

#define ME_NEW(TO, TYPE)              \
	do {                              \
		NOCH_MUST_ALLOC(TYPE, TO, 1); \
		memset(TO, 0, sizeof(TYPE));  \
	} while (0)

static me_expr_num_t *NOCH_PRIV(me_expr_new_num)(double val) {
	me_expr_num_t *num;
	ME_NEW(num, me_expr_num_t);
	num->_.type = ME_EXPR_NUM;

	num->val = val;
	return num;
}

static me_expr_unary_t *NOCH_PRIV(me_expr_new_unary)(char op, me_expr_t *expr) {
	me_expr_unary_t *un;
	ME_NEW(un, me_expr_unary_t);
	un->_.type = ME_EXPR_UNARY;

	un->op   = op;
	un->expr = expr;
	return un;
}

static me_expr_binary_t *NOCH_PRIV(me_expr_new_binary)(char op, me_expr_t *a, me_expr_t *b) {
	me_expr_binary_t *bin;
	ME_NEW(bin, me_expr_binary_t);
	bin->_.type = ME_EXPR_BINARY;

	bin->op = op;
	bin->a  = a;
	bin->b  = b;
	return bin;
}

static me_expr_id_t *NOCH_PRIV(me_expr_new_id)(const char *val) {
	me_expr_id_t *id;
	ME_NEW(id, me_expr_id_t);
	id->_.type = ME_EXPR_ID;

	NOCH_ASSERT(strlen(val) < ME_TOK_CAP);
	strcpy(id->val, val);
	return id;
}

static me_expr_fn_t *NOCH_PRIV(me_expr_new_fn)(const char *name) {
	me_expr_fn_t *fn;
	ME_NEW(fn, me_expr_fn_t);
	fn->_.type = ME_EXPR_FN;

	NOCH_ASSERT(strlen(name) < ME_TOK_CAP);
	strcpy(fn->name, name);
	return fn;
}

NOCH_DEF void me_expr_destroy(me_expr_t *this) {
	NOCH_ASSERT(this != NULL);

	switch (this->type) {
	case ME_EXPR_NUM: break;

	case ME_EXPR_UNARY:
		me_expr_destroy(((me_expr_unary_t*)this)->expr);
		break;

	case ME_EXPR_BINARY: {
		me_expr_binary_t *bin = (me_expr_binary_t*)this;
		me_expr_destroy(bin->a);
		me_expr_destroy(bin->b);
	} break;

	case ME_EXPR_ID: break;

	case ME_EXPR_FN: {
		me_expr_fn_t *fn = (me_expr_fn_t*)this;
		for (size_t i = 0; i < fn->argc; ++ i)
			me_expr_destroy(fn->args[i]);
	} break;

	default: NOCH_ASSERT(0 && "Unknown expression type");
	}

	NOCH_FREE(this);
}

typedef enum {
	ME_TOK_EOI = 0,

	ME_TOK_NUM,
	ME_TOK_ID,

	ME_TOK_PIPE,

	ME_TOK_LPAREN,
	ME_TOK_RPAREN,

	ME_TOK_LSQUARE,
	ME_TOK_RSQUARE,

	ME_TOK_COMMA,

	ME_TOK_ADD,
	ME_TOK_SUB,
	ME_TOK_MUL,
	ME_TOK_DIV,
	ME_TOK_MOD,
	ME_TOK_POW,
} NOCH_PRIV(me_expr_tok_t);

/* me_expr parser structure */
typedef struct {
	const char *in, *it, *bol; /* input, iterator, beginning of line */
	size_t      row;

	me_expr_tok_t tok;
	size_t      tok_row, tok_col;

	bool   prefixed_with_ws;
	char   data[ME_TOK_CAP];
	size_t data_size;

	size_t err_row, err_col;
} NOCH_PRIV(me_parser_t);

static void NOCH_PRIV(me_parser_init)(me_parser_t *this, const char *in) {
	memset(this, 0, sizeof(*this));

	this->in  = in;
	this->it  = in;
	this->bol = this->it;
	this->row = 1;
}

#define ME_STR(X)   __ME_STR(X)
#define __ME_STR(X) #X

static int NOCH_PRIV(me_parser_err)(me_parser_t *this, const char *msg, size_t row, size_t col) {
	this->err_row = row;
	this->err_col = col;
	NOCH_PARSER_ERR(msg);
	return -1;
}

static size_t NOCH_PRIV(me_parser_col)(me_parser_t *this) {
	return this->it - this->bol + 1;
}

static void NOCH_PRIV(me_parser_data_clear)(me_parser_t *this) {
	this->data[0]   = '\0';
	this->data_size = 0;
}

static int NOCH_PRIV(me_parser_data_add)(me_parser_t *this, char ch) {
	if (this->data_size + 1 >= ME_TOK_CAP)
		return me_parser_err(this, "Token exceeded maximum length of " ME_STR(ME_TOK_CAP - 1),
		                     this->row, me_parser_col(this));

	this->data[this->data_size ++] = ch;
	this->data[this->data_size]    = '\0';
	return 0;
}

static void NOCH_PRIV(me_parser_tok_start_here)(me_parser_t *this) {
	this->tok_row = this->row;
	this->tok_col = me_parser_col(this);
}

static int NOCH_PRIV(me_parser_tok_single)(me_parser_t *this, me_expr_tok_t tok) {
	me_parser_data_clear(this);
	me_parser_tok_start_here(this);
	this->tok = tok;

	++ this->it;
	return 0;
}

static int NOCH_PRIV(me_parser_tok_num)(me_parser_t *this) {
	NOCH_ASSERT(isdigit(*this->it));

	me_parser_data_clear(this);
	me_parser_tok_start_here(this);
	this->tok = ME_TOK_NUM;

	bool exponent = false, fpoint = false;

	while (true) {
		if (*this->it == '_') {
			++ this->it;
			continue;
		} else if (*this->it == 'e') {
			if (exponent)
				return me_parser_err(this, "Encountered exponent in number twice",
				                     this->row, me_parser_col(this));
			else
				exponent = true;

			if (this->it[1] == '+' || this->it[1] == '-') {
				if (me_parser_data_add(this, *this->it ++) != 0)
					return -1;
			}

			if (!isdigit(this->it[1]))
				return me_parser_err(this, "Expected a number", this->row, me_parser_col(this) + 1);
		} else if (*this->it == '.') {
			if (exponent)
				return me_parser_err(this, "Unexpected floating point in exponent",
				                     this->row, me_parser_col(this));
			else if (fpoint)
				return me_parser_err(this, "Encountered a floating point in number twice",
				                     this->row, me_parser_col(this));
			else {
				fpoint = true;

				if (!isdigit(this->it[1]))
					return me_parser_err(this, "Expected a number",
					                     this->row, me_parser_col(this) + 1);
			}
		} else if (!isdigit(*this->it))
			break;

		if (me_parser_data_add(this, *this->it ++) != 0)
			return -1;
	}

	return 0;
}

static int NOCH_PRIV(me_parser_tok_id)(me_parser_t *this) {
	NOCH_ASSERT(isalpha(*this->it));

	me_parser_data_clear(this);
	me_parser_tok_start_here(this);
	this->tok = ME_TOK_ID;

	while (isalnum(*this->it)) {
		if (me_parser_data_add(this, *this->it) != 0)
			return -1;

		++ this->it;
	}

	return 0;
}

static void NOCH_PRIV(me_parser_skip_ws)(me_parser_t *this) {
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

static int NOCH_PRIV(me_parser_advance)(me_parser_t *this) {
	me_parser_skip_ws(this);

	if (*this->it == '\0') {
		this->tok     = ME_TOK_EOI;
		this->tok_row = this->row;
		this->tok_col = me_parser_col(this);
		return 0;
	}

	switch (*this->it) {
	case '|': return me_parser_tok_single(this, ME_TOK_PIPE);
	case '(': return me_parser_tok_single(this, ME_TOK_LPAREN);
	case ')': return me_parser_tok_single(this, ME_TOK_RPAREN);
	case '[': return me_parser_tok_single(this, ME_TOK_LSQUARE);
	case ']': return me_parser_tok_single(this, ME_TOK_RSQUARE);
	case ',': return me_parser_tok_single(this, ME_TOK_COMMA);
	case '+': return me_parser_tok_single(this, ME_TOK_ADD);
	case '-': return me_parser_tok_single(this, ME_TOK_SUB);
	case '*': return me_parser_tok_single(this, ME_TOK_MUL);
	case '/': return me_parser_tok_single(this, ME_TOK_DIV);
	case '%': return me_parser_tok_single(this, ME_TOK_MOD);
	case '^': return me_parser_tok_single(this, ME_TOK_POW);

	default:
		if (isdigit(*this->it))
			return me_parser_tok_num(this);
		else if (isalpha(*this->it))
			return me_parser_tok_id(this);
		else
			return me_parser_err(this, "Unexpected character", this->row, me_parser_col(this));
	}
}

static char NOCH_PRIV(me_expr_tok_to_op)(me_expr_tok_t tok) {
	switch (tok) {
	case ME_TOK_ADD:  return '+';
	case ME_TOK_SUB:  return '-';
	case ME_TOK_MUL:  return '*';
	case ME_TOK_DIV:  return '/';
	case ME_TOK_MOD:  return '%';
	case ME_TOK_POW:  return '^';
	case ME_TOK_PIPE: return '|';

	default: NOCH_ASSERT(0 && "Token not an operator");
	}
}

static me_expr_t *NOCH_PRIV(me_parser_parse_factor)(me_parser_t *this);
static me_expr_t *NOCH_PRIV(me_parser_parse_arith) (me_parser_t *this);

static me_expr_t *NOCH_PRIV(me_parser_parse_pipe)(me_parser_t *this) {
	char op = me_expr_tok_to_op(this->tok);

	if (me_parser_advance(this) != 0)
		return NULL;

	me_expr_t *expr = me_parser_parse_arith(this);
	if (expr == NULL)
		return NULL;

	if (this->tok != ME_TOK_PIPE) {
		me_expr_destroy(expr);
		me_parser_err(this, "Expected a matching \"|\"", this->row, me_parser_col(this));
		return NULL;
	}

	if (me_parser_advance(this) != 0) {
		me_expr_destroy(expr);
		return NULL;
	}

	return (me_expr_t*)me_expr_new_unary(op, expr);
}

static me_expr_t *NOCH_PRIV(me_parser_parse_paren)(me_parser_t *this) {
	me_expr_tok_t end = this->tok + 1;

	if (me_parser_advance(this) != 0)
		return NULL;

	me_expr_t *expr = me_parser_parse_arith(this);
	if (expr == NULL)
		return NULL;

	expr->parens = true;

	if (this->tok != end) {
		me_expr_destroy(expr);
		const char *msg = end == ME_TOK_LPAREN?
		                  "Expected a matching \")\"" : "Expected a matching \"]\"";
		me_parser_err(this, msg, this->row, me_parser_col(this));
		return NULL;
	}

	if (me_parser_advance(this) != 0) {
		me_expr_destroy(expr);
		return NULL;
	}

	return expr;
}

static me_expr_t *NOCH_PRIV(me_parser_parse_unary)(me_parser_t *this) {
	char op = me_expr_tok_to_op(this->tok);

	if (me_parser_advance(this) != 0)
		return NULL;

	me_expr_t *expr = me_parser_parse_factor(this);
	if (expr == NULL)
		return NULL;

	return (me_expr_t*)me_expr_new_unary(op, expr);
}

static me_expr_t *NOCH_PRIV(me_parser_parse_num)(me_parser_t *this) {
	double num = atof(this->data);
	if (me_parser_advance(this) != 0)
		return NULL;

	return (me_expr_t*)me_expr_new_num(num);
}

static me_expr_t *NOCH_PRIV(me_parser_parse_id)(me_parser_t *this) {
	char name[ME_TOK_CAP];
	strcpy(name, this->data);

	if (me_parser_advance(this) != 0)
		return NULL;

	if (this->tok == ME_TOK_LPAREN) {
		if (me_parser_advance(this) != 0)
			return NULL;

		me_expr_fn_t *fn = me_expr_new_fn(name);

		if (this->tok == ME_TOK_RPAREN) {
			if (me_parser_advance(this) != 0) {
				me_expr_destroy((me_expr_t*)fn);
				return NULL;
			}

			return (me_expr_t*)fn;
		}

		while (true) {
			if (fn->argc >= ME_MAX_ARGS) {
				me_parser_err(this, "Exceeded maximum amount of " ME_STR(ME_MAX_ARGS)
				              " function arguments", this->row, me_parser_col(this));
				return NULL;
			}

			me_expr_t *expr = me_parser_parse_arith(this);
			fn->args[fn->argc ++] = expr;
			if (expr == NULL) {
				me_expr_destroy((me_expr_t*)fn);
				return NULL;
			}

			me_expr_tok_t tok = this->tok;
			if (me_parser_advance(this) != 0) {
				me_expr_destroy((me_expr_t*)fn);
				return NULL;
			}

			if (tok == ME_TOK_RPAREN)
				break;
			else if (tok != ME_TOK_COMMA) {
				me_parser_err(this, "Expected a \",\"", this->row, me_parser_col(this));
				return NULL;
			}
		}

		return (me_expr_t*)fn;
	} else
		return (me_expr_t*)me_expr_new_id(name);
}

static me_expr_t *NOCH_PRIV(me_parser_parse_factor)(me_parser_t *this) {
	if (this->it == this->in) {
		if (me_parser_advance(this) != 0)
			return NULL;
	}

	switch (this->tok) {
	case ME_TOK_EOI:
		me_parser_err(this, "Unexpected end of input", this->tok_row, this->tok_col);
		break;

	case ME_TOK_NUM:  return me_parser_parse_num(this);
	case ME_TOK_ID:   return me_parser_parse_id(this);
	case ME_TOK_PIPE: return me_parser_parse_pipe(this);

	case ME_TOK_ADD:    case ME_TOK_SUB:     return me_parser_parse_unary(this);
	case ME_TOK_LPAREN: case ME_TOK_LSQUARE: return me_parser_parse_paren(this);

	default: me_parser_err(this, "Unexpected token", this->tok_row, this->tok_col);
	}

	return NULL;
}

static me_expr_t *NOCH_PRIV(me_parser_parse_pow)(me_parser_t *this) {
	me_expr_t *left = me_parser_parse_factor(this);
	if (left == NULL)
		return NULL;

	while (this->tok == ME_TOK_POW) {
		char op = me_expr_tok_to_op(this->tok);

		if (me_parser_advance(this) != 0)
			goto fail;

		me_expr_t *right = me_parser_parse_pow(this);
		if (right == NULL)
			goto fail;

		left = (me_expr_t*)me_expr_new_binary(op, left, right);
	}

	return left;

fail:
	me_expr_destroy(left);
	return NULL;
}

static me_expr_t *NOCH_PRIV(me_parser_parse_term)(me_parser_t *this) {
	me_expr_t *left = me_parser_parse_pow(this);
	if (left == NULL)
		return NULL;

	while (this->tok == ME_TOK_MUL || this->tok == ME_TOK_DIV || this->tok == ME_TOK_MOD ||
	       this->tok == ME_TOK_ID  || this->tok == ME_TOK_LPAREN) {
		char op;
		if (this->tok == ME_TOK_ID || this->tok == ME_TOK_LPAREN) {
			if (this->prefixed_with_ws) {
				if (this->tok == ME_TOK_ID && strcmp(this->data, "x") == 0) {
					if (me_parser_advance(this) != 0)
						goto fail;
				} else
					break;
			}

			op = '*';
		} else {
			op = me_expr_tok_to_op(this->tok);
			if (me_parser_advance(this) != 0)
				goto fail;
		}

		me_expr_t *right = me_parser_parse_pow(this);
		if (right == NULL)
			goto fail;

		left = (me_expr_t*)me_expr_new_binary(op, left, right);
	}

	return left;

fail:
	me_expr_destroy(left);
	return NULL;
}

static me_expr_t *NOCH_PRIV(me_parser_parse_arith)(me_parser_t *this) {
	me_expr_t *left = me_parser_parse_term(this);
	if (left == NULL)
		return NULL;

	while (this->tok == ME_TOK_ADD || this->tok == ME_TOK_SUB) {
		char op = me_expr_tok_to_op(this->tok);

		if (me_parser_advance(this) != 0)
			goto fail;

		me_expr_t *right = me_parser_parse_term(this);
		if (right == NULL)
			goto fail;

		left = (me_expr_t*)me_expr_new_binary(op, left, right);
	}

	return left;

fail:
	me_expr_destroy(left);
	return NULL;
}

static int NOCH_PRIV(me_parser_expect_input_end)(me_parser_t *this) {
	if (this->tok != ME_TOK_EOI)
		return me_parser_err(this, "Expected an operator between tokens",
		                   this->tok_row, this->tok_col);

	return 0;
}

NOCH_DEF me_expr_t *me_parse_expr(const char *in, size_t *row, size_t *col) {
	NOCH_ASSERT(in != NULL);

	me_parser_t parser_, *parser = &parser_;
	me_parser_init(parser, in);

	me_expr_t *expr = me_parser_parse_arith(parser);
	if (expr != NULL) {
		if (me_parser_expect_input_end(parser) != 0) {
			me_expr_destroy(expr);
			expr = NULL;
		}
	}

	if (row != NULL) *row = parser->err_row;
	if (col != NULL) *col = parser->err_col;
	return expr;
}

static me_result_t NOCH_PRIV(me_div)(double a, double b) {
	if (b == 0)
		return me_result_err("0", "Division by zero");

	return me_result_ok(a / b);
}

static me_result_t NOCH_PRIV(me_mod)(double a, double b) {
	if (b == 0)
		return me_result_err("0", "Division by zero");

	float remainder = a / b;
	return me_result_ok(b * (remainder - floor(remainder)));
}

NOCH_DEF me_result_t me_result_ok(double val) {
	me_result_t result = {0};
	result.val = val;
	return result;
}

NOCH_DEF me_result_t me_result_err(const char *origin, const char *err) {
	me_result_t result = {0};
	result.origin = origin;
	result.err    = err;
	return result;
}

NOCH_DEF me_result_t me_result_none(void) {
	me_result_t result = {0};
	result.none = true;
	return result;
}

#define ME_CTX_MUST_EVAL_EXPR(THIS, EXPR, TO)               \
	do {                                                    \
		me_result_t _result = me_ctx_eval_expr(THIS, EXPR); \
		if (_result.err != NULL)                            \
			return _result;                                 \
		TO = _result.val;                                   \
	} while (0)

NOCH_DEF me_result_t me_ctx_eval_expr(me_ctx_t *this, me_expr_t *expr) {
	switch (expr->type) {
	case ME_EXPR_NUM: return me_result_ok(((me_expr_num_t*)expr)->val);

	case ME_EXPR_UNARY: {
		me_expr_unary_t *un = (me_expr_unary_t*)expr;
		double num;
		ME_CTX_MUST_EVAL_EXPR(this, un->expr, num);

		switch (un->op) {
		case '+': return me_result_ok(num);
		case '-': return me_result_ok(-num);
		case '|': return me_result_ok(fabs(num));

		default: NOCH_ASSERT(0 && "Unknown unary operator");
		}
	}

	case ME_EXPR_BINARY: {
		me_expr_binary_t *bin = (me_expr_binary_t*)expr;
		double a, b;
		ME_CTX_MUST_EVAL_EXPR(this, bin->a, a);
		ME_CTX_MUST_EVAL_EXPR(this, bin->b, b);

		switch (bin->op) {
		case '+': return me_result_ok(a + b);
		case '-': return me_result_ok(a - b);
		case '*': return me_result_ok(a * b);
		case '/': return me_div(a, b);
		case '%': return me_mod(a, b);
		case '^': return me_result_ok(pow(a, b));

		default: NOCH_ASSERT(0 && "Unknown binary operator");
		}
	} break;

	case ME_EXPR_ID: {
		me_expr_id_t *id = (me_expr_id_t*)expr;

		/* TODO: Maybe a hash map? */
		for (size_t i = 0; i < this->ids_count; ++ i) {
			if (strcmp(this->ids[i].name, id->val) == 0)
				return me_result_ok(this->ids[i].val);
		}

		return me_result_err(id->val, "Undefined identifier");
	}

	case ME_EXPR_FN: {
		me_expr_fn_t *fn = (me_expr_fn_t*)expr;
		for (size_t i = 0; i < this->fns_count; ++ i) {
			if (strcmp(this->fns[i].name, fn->name) == 0) {
				double args[ME_MAX_ARGS];
				for (size_t i = 0; i < fn->argc; ++ i)
					ME_CTX_MUST_EVAL_EXPR(this, fn->args[i], args[i]);

				me_result_t result = this->fns[i].native(args, fn->argc);
				if (result.origin == NULL)
					result.origin = fn->name;

				return result;
			}
		}

		return me_result_err(fn->name, "Undefined function");
	}

	default: NOCH_ASSERT(0 && "Unknown expression type");
	}
}

static void NOCH_PRIV(me_fprint_float)(double num, FILE *file) {
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

NOCH_DEF void me_expr_fprint(me_expr_t *this, FILE *file) {
	if (this->parens)
		fprintf(file, "(");

	switch (this->type) {
	case ME_EXPR_NUM:
		me_fprint_float(((me_expr_num_t*)this)->val, file);
		break;

	case ME_EXPR_ID:
		fprintf(file, "%s", ((me_expr_id_t*)this)->val);
		break;

	case ME_EXPR_UNARY: {
		me_expr_unary_t *un = (me_expr_unary_t*)this;

		if (un->op == '|') {
			fprintf(file, "|");
			me_expr_fprint(un->expr, file);
			fprintf(file, "|");
		} else {
			fprintf(file, "%c", un->op);
			me_expr_fprint(un->expr, file);
		}
	} break;

	case ME_EXPR_BINARY: {
		me_expr_binary_t *bin = (me_expr_binary_t*)this;
		me_expr_fprint(bin->a, file);
		fprintf(file, " %c ", bin->op);
		me_expr_fprint(bin->b, file);
	} break;

	case ME_EXPR_FN: {
		me_expr_fn_t *fn = (me_expr_fn_t*)this;
		fprintf(file, "%s(", fn->name);
		for (size_t i = 0; i < fn->argc; ++ i) {
			if (i > 0)
				fprintf(file, ", ");

			me_expr_fprint(fn->args[i], file);
		}
		fprintf(file, ")");
	} break;

	default: NOCH_ASSERT(0 && "Unknown expression type");
	}

	if (this->parens)
		fprintf(file, ")");
}

#undef ME_STR
#undef __ME_STR

NOCH_DEF void me_ctx_init(me_ctx_t *this) {
	memset(this, 0, sizeof(*this));

	this->fns_cap = ME_CTX_FNS_CHUNK_SIZE;
	NOCH_MUST_ALLOC(me_ctx_fn_t, this->fns, this->fns_cap);

	this->ids_cap = ME_CTX_IDS_CHUNK_SIZE;
	NOCH_MUST_ALLOC(me_ctx_id_t, this->ids, this->ids_cap);
}

NOCH_DEF void me_ctx_deinit(me_ctx_t *this) {
	NOCH_FREE(this->fns);
	NOCH_FREE(this->ids);
}

NOCH_DEF void me_ctx_set_fn(me_ctx_t *this, const char *name, me_native_t native) {
	NOCH_ASSERT(strlen(name) + 1 < ME_TOK_CAP);

	me_ctx_fn_t *fn = me_ctx_get_fn(this, name);
	if (fn != NULL) {
		fn->native = native;
		return;
	}

	if (this->fns_count >= this->fns_cap) {
		this->fns_cap *= 2;
		NOCH_MUST_REALLOC(me_ctx_fn_t, this->fns, this->fns_cap);
	}

	strcpy(this->fns[this->fns_count].name, name);
	this->fns[this->fns_count ++].native = native;
}

NOCH_DEF void me_ctx_set_id(me_ctx_t *this, const char *name, double val, bool is_const) {
	NOCH_ASSERT(strlen(name) + 1 < ME_TOK_CAP);

	me_ctx_id_t *id = me_ctx_get_id(this, name);
	if (id != NULL) {
		id->is_const = is_const;
		id->val      = val;
		return;
	}

	if (this->ids_count >= this->ids_cap) {
		this->ids_cap *= 2;
		NOCH_MUST_REALLOC(me_ctx_id_t, this->ids, this->ids_cap);
	}

	strcpy(this->ids[this->ids_count].name, name);
	this->ids[this->ids_count].is_const = is_const;
	this->ids[this->ids_count ++].val   = val;
}

NOCH_DEF me_ctx_fn_t *me_ctx_get_fn(me_ctx_t *this, const char *name) {
	NOCH_ASSERT(strlen(name) + 1 < ME_TOK_CAP);

	for (size_t i = 0; i < this->fns_count; ++ i) {
		if (strcmp(this->fns[i].name, name) == 0)
			return this->fns + i;
	}

	return NULL;
}

NOCH_DEF me_ctx_id_t *me_ctx_get_id(me_ctx_t *this, const char *name) {
	NOCH_ASSERT(strlen(name) + 1 < ME_TOK_CAP);

	for (size_t i = 0; i < this->ids_count; ++ i) {
		if (strcmp(this->ids[i].name, name) == 0)
			return this->ids + i;
	}

	return NULL;
}

#define ME_BIND_NATIVE(NAME, CFN, ARGC, ...)               \
	NOCH_DEF me_result_t NAME(double *args, size_t argc) { \
		if (argc != ARGC)                                  \
			return ME_INVALID_AMOUNT_OF_ARGS;              \
		return me_result_ok(CFN(__VA_ARGS__));             \
	}

ME_BIND_NATIVE(me_sqrt,  sqrt,  1, args[0])
ME_BIND_NATIVE(me_cbrt,  cbrt,  1, args[0])
ME_BIND_NATIVE(me_hypot, hypot, 2, args[0], args[1])
ME_BIND_NATIVE(me_sin,   sin,   1, args[0])
ME_BIND_NATIVE(me_cos,   cos,   1, args[0])
ME_BIND_NATIVE(me_tan,   tan,   1, args[0])
ME_BIND_NATIVE(me_log,   log,   1, args[0])
ME_BIND_NATIVE(me_floor, floor, 1, args[0])
ME_BIND_NATIVE(me_ceil,  ceil,  1, args[0])
ME_BIND_NATIVE(me_round, round, 1, args[0])
ME_BIND_NATIVE(me_atan,  atan,  1, args[0])
ME_BIND_NATIVE(me_atan2, atan2, 2, args[0], args[1])
ME_BIND_NATIVE(me_abs,   fabs,  1, args[0])
ME_BIND_NATIVE(me_pow,   pow,   2, args[0], args[1])

NOCH_DEF me_result_t me_root(double *args, size_t argc) {
	if (argc != 2)
		return ME_INVALID_AMOUNT_OF_ARGS;

	if (args[1] == 0)
		return ME_DIV_BY_ZERO;

	return me_result_ok(pow(args[0], 1 / args[1]));
}

NOCH_DEF me_result_t me_sum(double *args, size_t argc) {
	double sum = 0;
	for (size_t i = 0; i < argc; ++ i)
		sum += args[i];

	return me_result_ok(sum);
}

#undef ME_BIND_NATIVE

NOCH_DEF void me_ctx_register_basic_fns(me_ctx_t *this) {
	me_ctx_set_fn(this, "sqrt",  me_sqrt);
	me_ctx_set_fn(this, "cbrt",  me_cbrt);
	me_ctx_set_fn(this, "hypot", me_hypot);
	me_ctx_set_fn(this, "sin",   me_sin);
	me_ctx_set_fn(this, "cos",   me_cos);
	me_ctx_set_fn(this, "tan",   me_tan);
	me_ctx_set_fn(this, "log",   me_log);
	me_ctx_set_fn(this, "floor", me_floor);
	me_ctx_set_fn(this, "ceil",  me_ceil);
	me_ctx_set_fn(this, "round", me_round);
	me_ctx_set_fn(this, "root",  me_root);
	me_ctx_set_fn(this, "atan",  me_atan);
	me_ctx_set_fn(this, "atan2", me_atan2);
	me_ctx_set_fn(this, "abs",   me_abs);
	me_ctx_set_fn(this, "pow",   me_pow);
	me_ctx_set_fn(this, "sum",   me_sum);
}

NOCH_DEF void me_stmt_destroy(me_stmt_t *this) {
	NOCH_ASSERT(this != NULL);

	switch (this->type) {
	case ME_STMT_EXPR:
		me_expr_destroy(((me_stmt_expr_t*)this)->expr);
		break;

	case ME_STMT_VAR: {
		me_stmt_var_t *var = (me_stmt_var_t*)this;
		me_expr_destroy((me_expr_t*)var->id);
		me_expr_destroy(var->expr);
	} break;

	case ME_STMT_DEF: {
		me_stmt_def_t *def = (me_stmt_def_t*)this;
		me_expr_destroy((me_expr_t*)def->fn);
		me_expr_destroy(def->expr);
	} break;

	default: NOCH_ASSERT(0 && "Unknown statement type");
	}

	NOCH_FREE(this);
}

NOCH_DEF void me_stmt_fprint(me_stmt_t *this, FILE *file) {
	switch (this->type) {
	case ME_STMT_EXPR:
		me_expr_fprint(((me_stmt_expr_t*)this)->expr, file);
		break;

	case ME_STMT_VAR: {
		me_stmt_var_t *var = (me_stmt_var_t*)this;
		me_expr_fprint((me_expr_t*)var->id, file);
		fprintf(file, " = ");
		me_expr_fprint(var->expr, file);
	} break;

	case ME_STMT_DEF: {
		me_stmt_def_t *def = (me_stmt_def_t*)this;
		me_expr_fprint((me_expr_t*)def->fn, file);
		fprintf(file, " = ");
		me_expr_fprint(def->expr, file);
	} break;

	default: NOCH_ASSERT(0 && "Unknown statement type");
	}
}

static me_stmt_expr_t *NOCH_PRIV(me_stmt_new_expr)(me_expr_t *expr) {
	me_stmt_expr_t *stmt;
	ME_NEW(stmt, me_stmt_expr_t);
	stmt->_.type = ME_STMT_EXPR;

	stmt->expr = expr;
	return stmt;
}

static me_stmt_var_t *NOCH_PRIV(me_stmt_new_var)(me_expr_id_t *id, me_expr_t *expr) {
	me_stmt_var_t *var;
	ME_NEW(var, me_stmt_var_t);
	var->_.type = ME_STMT_VAR;

	var->id   = id;
	var->expr = expr;
	return var;
}

static me_stmt_def_t *NOCH_PRIV(me_stmt_new_def)(me_expr_fn_t *fn, me_expr_t *expr) {
	me_stmt_def_t *def;
	ME_NEW(def, me_stmt_def_t);
	def->_.type = ME_STMT_DEF;

	def->fn   = fn;
	def->expr = expr;
	return def;
}

NOCH_DEF me_stmt_t *me_parse_stmt(const char *in, size_t *row, size_t *col) {
	NOCH_ASSERT(in != NULL);

	if (row != NULL) *row = 0;
	if (col != NULL) *col = 0;

	for (const char *it = in; *it != '\0'; ++ it) {
		if (*it == '=') {
			size_t len = it - in - 1;
			char  *side1;
			NOCH_MUST_ALLOC(char, side1, len + 1);
			memcpy(side1, in, len);
			side1[len] = '\0';

			const char *side2 = it + 1;

			me_expr_t *expr1 = me_parse_expr(side1, row, col);
			NOCH_FREE(side1);
			if (expr1 == NULL)
				return NULL;

			me_expr_t *expr2 = me_parse_expr(side2, row, col);
			if (expr2 == NULL) {
				me_expr_destroy(expr1);
				return NULL;
			}

			if (expr1->type == ME_EXPR_ID)
				return (me_stmt_t*)me_stmt_new_var((me_expr_id_t*)expr1, expr2);
			else if (expr1->type == ME_EXPR_FN)
				return (me_stmt_t*)me_stmt_new_def((me_expr_fn_t*)expr1, expr2);
			else {
				me_expr_destroy(expr1);
				me_expr_destroy(expr2);
				NOCH_PARSER_ERR("Unexpected expression on the left side of \"=\"");
				return NULL;
			}
		}
	}

	me_expr_t *expr = me_parse_expr(in, row, col);
	if (expr == NULL)
		return NULL;

	return (me_stmt_t*)me_stmt_new_expr(expr);
}

NOCH_DEF me_result_t me_ctx_eval_stmt(me_ctx_t *this, me_stmt_t *stmt) {
	switch (stmt->type) {
	case ME_STMT_EXPR: return me_ctx_eval_expr(this, ((me_stmt_expr_t*)stmt)->expr);

	case ME_STMT_VAR: {
		me_stmt_var_t *var = (me_stmt_var_t*)stmt;
		double         val;
		ME_CTX_MUST_EVAL_EXPR(this, var->expr, val);

		me_ctx_id_t *id = me_ctx_get_id(this, var->id->val);
		if (id == NULL)
			me_ctx_set_id(this, var->id->val, val, false);
		else if (!id->is_const)
			id->val = val;
		else
			return me_result_err(var->id->val, "Cannot assign to a constant");

		return me_result_none();
	}

	case ME_STMT_DEF: NOCH_ASSERT(0 && "TODO");

	default: NOCH_ASSERT(0 && "Unknown statement type");
	}
}

#undef ME_CTX_MUST_EVAL_EXPR

NOCH_DEF double me_eval(const char *in, size_t *row, size_t *col) {
	me_expr_t *expr = me_parse_expr(in, row, col);
	if (expr == NULL)
		return ME_NAN;

	me_ctx_t ctx_, *ctx = &ctx_;
	me_ctx_init(ctx);

	me_result_t result = me_ctx_eval_expr(ctx, expr);
	if (result.err != NULL)
		return ME_NAN;

	me_ctx_deinit(ctx);
	me_expr_destroy(expr);
	return result.val;
}

#undef me_expr_new_num
#undef me_expr_new_unary
#undef me_expr_new_binary
#undef me_expr_new_id
#undef me_expr_new_fn
#undef me_expr_tok_t
#undef me_parser_t
#undef me_parser_data_clear
#undef me_parser_data_add
#undef me_parser_skip_ws
#undef me_parser_tok_start_here
#undef me_parser_tok_single
#undef me_parser_tok_num
#undef me_parser_tok_id
#undef me_parser_advance
#undef me_expr_tok_to_op
#undef me_parser_parse_arith
#undef me_parser_parse_term
#undef me_parser_parse_pow
#undef me_parser_parse_factor
#undef me_parser_parse_unary
#undef me_parser_parse_paren
#undef me_parser_parse_id
#undef me_parser_parse_num
#undef me_parser_expect_input_end
#undef me_parser_init
#undef me_parser_deinit
#undef me_parser_err
#undef me_parser_col
#undef me_fprint_float
#undef me_mod
#undef me_div
#undef me_stmt_new_expr
#undef me_stmt_new_var
#undef me_stmt_new_def

#ifdef __cplusplus
}
#endif
