#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"
#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "mexpr.h"

#define mexpr_strdup             NOCH_PRIVATE(mexpr_strdup)
#define mexpr_new_num            NOCH_PRIVATE(mexpr_new_num)
#define mexpr_new_unary          NOCH_PRIVATE(mexpr_new_unary)
#define mexpr_new_binary         NOCH_PRIVATE(mexpr_new_binary)
#define mexpr_new_id             NOCH_PRIVATE(mexpr_new_id)
#define mexpr_new_fn             NOCH_PRIVATE(mexpr_new_fn)
#define mexpr_tok_t              NOCH_PRIVATE(mexpr_tok_t)
#define mparser_t                NOCH_PRIVATE(mparser_t)
#define mparser_data_clear       NOCH_PRIVATE(mparser_data_clear)
#define mparser_data_add         NOCH_PRIVATE(mparser_data_add)
#define mparser_skip_ws          NOCH_PRIVATE(mparser_skip_ws)
#define mparser_tok_start_here   NOCH_PRIVATE(mparser_tok_start_here)
#define mparser_tok_single       NOCH_PRIVATE(mparser_tok_single)
#define mparser_tok_num          NOCH_PRIVATE(mparser_tok_num)
#define mparser_tok_id           NOCH_PRIVATE(mparser_tok_id)
#define mparser_advance          NOCH_PRIVATE(mparser_advance)
#define mexpr_tok_to_op          NOCH_PRIVATE(mexpr_tok_to_op)
#define mparser_parse_arith      NOCH_PRIVATE(mparser_parse_arith)
#define mparser_parse_term       NOCH_PRIVATE(mparser_parse_term)
#define mparser_parse_pow        NOCH_PRIVATE(mparser_parse_pow)
#define mparser_parse_factor     NOCH_PRIVATE(mparser_parse_factor)
#define mparser_parse_unary      NOCH_PRIVATE(mparser_parse_unary)
#define mparser_parse_paren      NOCH_PRIVATE(mparser_parse_paren)
#define mparser_expect_input_end NOCH_PRIVATE(mparser_expect_input_end)
#define mparser_init             NOCH_PRIVATE(mparser_init)
#define mparser_deinit           NOCH_PRIVATE(mparser_deinit)
#define mparser_err              NOCH_PRIVATE(mparser_err)
#define mparser_col              NOCH_PRIVATE(mparser_col)
#define mexpr_fprint_float       NOCH_PRIVATE(mexpr_fprint_float)
#define mexpr_mod                NOCH_PRIVATE(mexpr_mod)

static char *mexpr_strdup(const char *str) {
	char *duped;
	NOCH_MUST_ALLOC(char, duped, strlen(str) + 1);

	strcpy(duped, str);
	return duped;
}

#define MEXPR_ALLOC(TO, TYPE)         \
	do {                              \
		NOCH_MUST_ALLOC(TYPE, TO, 1); \
		memset(TO, 0, sizeof(TYPE));  \
	} while (0)

static mexpr_num_t *mexpr_new_num(double val) {
	mexpr_num_t *num;
	MEXPR_ALLOC(num, mexpr_num_t);
	num->_.type = MEXPR_NUM;

	num->val = val;
	return num;
}

static mexpr_unary_t *mexpr_new_unary(char op, mexpr_t *expr) {
	mexpr_unary_t *un;
	MEXPR_ALLOC(un, mexpr_unary_t);
	un->_.type = MEXPR_UNARY;

	un->op   = op;
	un->expr = expr;
	return un;
}

static mexpr_binary_t *mexpr_new_binary(char op, mexpr_t *a, mexpr_t *b) {
	mexpr_binary_t *bin;
	MEXPR_ALLOC(bin, mexpr_binary_t);
	bin->_.type = MEXPR_BINARY;

	bin->op = op;
	bin->a  = a;
	bin->b  = b;
	return bin;
}

static mexpr_id_t *mexpr_new_id(const char *val) {
	mexpr_id_t *id;
	MEXPR_ALLOC(id, mexpr_id_t);
	id->_.type = MEXPR_ID;

	id->val = mexpr_strdup(val);
	return id;
}

static mexpr_fn_t *mexpr_new_fn(const char *name) {
	mexpr_fn_t *fn;
	MEXPR_ALLOC(fn, mexpr_fn_t);
	fn->_.type = MEXPR_FN;

	fn->name = mexpr_strdup(name);
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

	case MEXPR_ID:
		NOCH_FREE(((mexpr_id_t*)this)->val);
		break;

	case MEXPR_FN: {
		mexpr_fn_t *fn = (mexpr_fn_t*)this;
		NOCH_FREE(fn->name);

		for (size_t i = 0; i < fn->args_count; ++ i)
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

	MEXPR_TOK_LPAREN,
	MEXPR_TOK_RPAREN,

	MEXPR_TOK_LSQUARE,
	MEXPR_TOK_RSQUARE,

	MEXPR_TOK_ADD,
	MEXPR_TOK_SUB,
	MEXPR_TOK_MUL,
	MEXPR_TOK_DIV,
	MEXPR_TOK_MOD,
	MEXPR_TOK_POW,
} mexpr_tok_t;

/* mexpr parser structure */
typedef struct {
	const char *in, *it, *bol; /* input, iterator, beginning of line */
	size_t      row;

	mexpr_tok_t tok;
	size_t      tok_row, tok_col;

	bool   prefixed_with_ws;
	char  *data;
	size_t data_cap, data_size;

	size_t err_row, err_col;
} mparser_t;

static void mparser_init(mparser_t *this, const char *in) {
	this->in       = in;
	this->it       = in;
	this->bol      = this->it;
	this->row      = 1;
	this->data_cap = 64;
	NOCH_MUST_ALLOC(char, this->data, this->data_cap);
}

static void mparser_deinit(mparser_t *this) {
	NOCH_FREE(this->data);
}

inline static int mparser_err(mparser_t *this, const char *msg, size_t row, size_t col) {
	this->err_row = row;
	this->err_col = col;
	NOCH_PARSER_ERR(msg);
	return -1;
}

inline static size_t mparser_col(mparser_t *this) {
	return this->it - this->bol + 1;
}

static void mparser_data_clear(mparser_t *this) {
	this->data[0]   = '\0';
	this->data_size = 0;
}

static int mparser_data_add(mparser_t *this, char ch) {
	if (this->data_size + 1 >= this->data_cap) {
		this->data_cap *= 2;
		NOCH_MUST_REALLOC(char, this->data, this->data_cap);
	}

	this->data[this->data_size ++] = ch;
	this->data[this->data_size]    = '\0';
	return 0;
}

static void mparser_tok_start_here(mparser_t *this) {
	this->tok_row = this->row;
	this->tok_col = mparser_col(this);
}

static int mparser_tok_single(mparser_t *this, mexpr_tok_t tok) {
	mparser_data_clear(this);
	mparser_tok_start_here(this);
	this->tok = tok;

	++ this->it;
	return 0;
}

static int mparser_tok_num(mparser_t *this) {
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

			if (this->it[1] == '+' || this->it[1] == '-')
				++ this->it;

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
		} else if (isalpha(*this->it))
			return mparser_err(this, "Unexpected character in number",
			                   this->row, mparser_col(this));
		else if (!isdigit(*this->it))
			break;

		if (mparser_data_add(this, *this->it ++) != 0)
			return -1;
	}

	return 0;
}

static int mparser_tok_id(mparser_t *this) {
	NOCH_ASSERT(0 && "TODO");
	/* TODO: Parse 1 letter variable names and multi-letter function names
	 *
	 * 2abc    is the same as 2 * a * b * c
	 * 2abc(5) is the same as 2 * abc(), where abc is the name of the function
	 *
	 * Make the lexer detect this by reading the whole string of letters and then decide if its a
	 * function name or variables multiplied together based on if the following character is a left
	 * paranthesis (
	 */

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

static void mparser_skip_ws(mparser_t *this) {
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

static int mparser_advance(mparser_t *this) {
	mparser_skip_ws(this);

	if (*this->it == '\0') {
		this->tok     = MEXPR_TOK_EOI;
		this->tok_row = this->row;
		this->tok_col = mparser_col(this);
		return 0;
	}

	switch (*this->it) {
	case '(': return mparser_tok_single(this, MEXPR_TOK_LPAREN);
	case ')': return mparser_tok_single(this, MEXPR_TOK_RPAREN);
	case '[': return mparser_tok_single(this, MEXPR_TOK_LSQUARE);
	case ']': return mparser_tok_single(this, MEXPR_TOK_RSQUARE);
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

static char mexpr_tok_to_op(mexpr_tok_t tok) {
	switch (tok) {
	case MEXPR_TOK_ADD: return '+';
	case MEXPR_TOK_SUB: return '-';
	case MEXPR_TOK_MUL: return '*';
	case MEXPR_TOK_DIV: return '/';
	case MEXPR_TOK_MOD: return '%';
	case MEXPR_TOK_POW: return '^';

	default: NOCH_ASSERT(0 && "Token not an operator");
	}
}
static mexpr_t *mparser_parse_factor(mparser_t *this);
static mexpr_t *mparser_parse_arith (mparser_t *this);

static mexpr_t *mparser_parse_paren(mparser_t *this) {
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
		return NOCH_NULL(mparser_err(this, msg, this->row, mparser_col(this)));
	}

	if (mparser_advance(this) != 0) {
		mexpr_destroy(expr);
		return NULL;
	}

	return expr;
}

static mexpr_t *mparser_parse_unary(mparser_t *this) {
	char op = mexpr_tok_to_op(this->tok);

	if (mparser_advance(this) != 0)
		return NULL;

	mexpr_t *expr = mparser_parse_factor(this);
	if (expr == NULL)
		return NULL;

	return (mexpr_t*)mexpr_new_unary(op, expr);
}

static mexpr_t *mparser_parse_factor(mparser_t *this) {
	if (this->it == this->in) {
		if (mparser_advance(this) != 0)
			return NULL;
	}

	switch (this->tok) {
	case MEXPR_TOK_EOI:
		mparser_err(this, "Unexpected end of input", this->tok_row, this->tok_col);
		break;

	case MEXPR_TOK_NUM: {
		double num = atof(this->data);
		if (mparser_advance(this) != 0)
			return NULL;

		return (mexpr_t*)mexpr_new_num(num);
	}

	case MEXPR_TOK_ADD: case MEXPR_TOK_SUB:
		return mparser_parse_unary(this);

	case MEXPR_TOK_LPAREN: case MEXPR_TOK_LSQUARE:
		return mparser_parse_paren(this);

	case MEXPR_TOK_ID: NOCH_ASSERT(0 && "TODO");  //return mparser_parse_id(this);

	default: mparser_err(this, "Unexpected token", this->tok_row, this->tok_col);
	}

	return NULL;

	mexpr_new_fn("");
	mexpr_new_id("");
}

static mexpr_t *mparser_parse_pow(mparser_t *this) {
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

static mexpr_t *mparser_parse_term(mparser_t *this) {
	mexpr_t *left = mparser_parse_pow(this);
	if (left == NULL)
		return NULL;

	while (this->tok == MEXPR_TOK_MUL || this->tok == MEXPR_TOK_DIV || this->tok == MEXPR_TOK_MOD ||
	       (!this->prefixed_with_ws && this->tok == MEXPR_TOK_LPAREN)) {
		char op;
		if (this->tok == MEXPR_TOK_LPAREN)
			op = '*';
		else {
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

static mexpr_t *mparser_parse_arith(mparser_t *this) {
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

static int mparser_expect_input_end(mparser_t *this) {
	if (this->tok != MEXPR_TOK_EOI)
		return mparser_err(this, "Unexpected trailing token", this->tok_row, this->tok_col);

	return 0;
}

NOCH_DEF mexpr_t *mexpr_parse(const char *in, size_t *row, size_t *col) {
	NOCH_ASSERT(in != NULL);

	mparser_t parser_ = {0}, *parser = &parser_;
	mparser_init(parser, in);

	mexpr_t *expr = mparser_parse_arith(parser);
	if (expr != NULL) {
		if (mparser_expect_input_end(parser) != 0) {
			mexpr_destroy(expr);
			expr = NULL;
		}
	}

	mparser_deinit(parser);
	if (row != NULL) *row = parser->err_row;
	if (col != NULL) *col = parser->err_col;
	return expr;
}

#define MEXPR_NAN (0.0 / 0.0)

static double mexpr_mod(double a, double b) {
	float remainder = a / b;
	return b * (remainder - floor(remainder));
}

NOCH_DEF double mexpr_eval(mexpr_t *this, mctx_t *ctx) {
	switch (this->type) {
	case MEXPR_NUM: return ((mexpr_num_t*)this)->val;

	case MEXPR_UNARY: {
		mexpr_unary_t *un = (mexpr_unary_t*)this;
		double num = mexpr_eval(un->expr, ctx);

		return un->op == '-'? -num : num;
	}

	case MEXPR_BINARY: {
		mexpr_binary_t *bin = (mexpr_binary_t*)this;
		double a = mexpr_eval(bin->a, ctx);
		double b = mexpr_eval(bin->b, ctx);

		switch (bin->op) {
		case '+': return a + b;
		case '-': return a - b;
		case '*': return a * b;
		case '/': return a / b;
		case '%': return mexpr_mod(a, b);
		case '^': return pow(a, b);

		default: NOCH_ASSERT(0 && "Unknown binary operator");
		}
	} break;

	case MEXPR_ID: NOCH_ASSERT(0 && "TODO");
	case MEXPR_FN: NOCH_ASSERT(0 && "TODO");

	default: NOCH_ASSERT(0 && "Unknown expression type");
	}
}

#undef MEXPR_NAN

static void mexpr_fprint_float(float num, FILE *file) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%f", num);

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
	case MEXPR_NUM: mexpr_fprint_float(((mexpr_num_t*)this)->val, file); break;

	case MEXPR_UNARY: {
		mexpr_unary_t *un = (mexpr_unary_t*)this;
		fprintf(file, "(%c", un->op);
		mexpr_fprint(un->expr, file);
		fprintf(file, ")");
	} break;

	case MEXPR_BINARY: {
		mexpr_binary_t *bin = (mexpr_binary_t*)this;
		fprintf(file, "(");
		mexpr_fprint(bin->a, file);
		fprintf(file, " %c ", bin->op);
		mexpr_fprint(bin->b, file);
		fprintf(file, ")");
	} break;

	case MEXPR_ID: NOCH_ASSERT(0 && "TODO");
	case MEXPR_FN: NOCH_ASSERT(0 && "TODO");

	default: NOCH_ASSERT(0 && "Unknown expression type");
	}
}

#undef mexpr_strdup
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
#undef mparser_expect_input_end
#undef mparser_init
#undef mparser_deinit
#undef mparser_err
#undef mparser_col

#ifdef __cplusplus
}
#endif