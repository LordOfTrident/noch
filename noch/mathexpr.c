#ifdef __cplusplus
extern "C" {
#endif

#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/error.c"

#include "mathexpr.h"

static MeNumber *meNewNumber(size_t pos, double value) {
	MeNumber *this = (MeNumber*)nochAlloc(sizeof(MeNumber));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.parens = false;
	this->base.pos    = pos;
	this->base.type   = ME_NUMBER;
	this->value       = value;
	return this;
}

static MeUnary *meNewUnary(size_t pos, char op, MeExpr *expr) {
	MeUnary *this = (MeUnary*)nochAlloc(sizeof(MeUnary));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.parens = false;
	this->base.pos    = pos;
	this->base.type   = ME_UNARY;
	this->op          = op;
	this->expr        = expr;
	return this;
}

static MeBinary *meNewBinary(size_t pos, char op, MeExpr *left, MeExpr *right) {
	MeBinary *this = (MeBinary*)nochAlloc(sizeof(MeBinary));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.parens = false;
	this->base.pos    = pos;
	this->base.type   = ME_BINARY;
	this->op          = op;
	this->left        = left;
	this->right       = right;
	return this;
}

static MeId *meNewId(size_t pos, const char *value) {
	MeId *this = (MeId*)nochAlloc(sizeof(MeId));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.parens = false;
	this->base.pos    = pos;
	this->base.type   = ME_ID;

	NOCH_ASSERT(strlen(value) < ME_TOKEN_CAPACITY);
	strcpy(this->value, value);
	return this;
}

static MeFunc *meNewFunc(size_t pos, const char *name) {
	MeFunc *this = (MeFunc*)nochAlloc(sizeof(MeFunc));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.parens = false;
	this->base.pos    = pos;
	this->base.type   = ME_FUNC;
	this->argsCount   = 0;

	NOCH_ASSERT(strlen(name) < ME_TOKEN_CAPACITY);
	strcpy(this->name, name);
	return this;
}

NOCH_DEF double meError(size_t pos, const char *fmt, ...) {
	char    str[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	nochError("%lu: %s", (long unsigned)pos, str);
	return NAN;
}

static double meDiv(size_t pos, double a, double b) {
	if (b == 0)
		return meError(pos, "Division by zero");

	return a / b;
}

static double meMod(size_t pos, double a, double b) {
	if (b == 0)
		return meError(pos, "Division by zero in modulus");

	float remainder = a / b;
	return b * (remainder - floor(remainder));
}

static double meDoUnaryOp(char op, double value) {
	switch (op) {
	case ME_OP_ADD: return value;
	case ME_OP_SUB: return -value;
	case ME_OP_ABS: return fabs(value);

	default:
		NOCH_ASSERT(0 && "Unknown MeUnary operator");
		return NAN;
	}
}

static double meDoBinaryOp(char op, double left, double right, size_t pos) {
	switch (op) {
	case ME_OP_ADD: return left + right;
	case ME_OP_SUB: return left - right;
	case ME_OP_MUL: return left * right;
	case ME_OP_DIV: return meDiv(pos, left, right);
	case ME_OP_MOD: return meMod(pos, left, right);
	case ME_OP_POW: return pow(left, right);

	default: NOCH_ASSERT(0 && "Unknown MeUnary operator");
	}
}

static double meEvalUnary(MeUnary *unary, MeDef *defs, size_t size) {
	double value = meEval(unary->expr, defs, size);
	if (isnan(value))
		return NAN;

	return meDoUnaryOp(unary->op, value);
}

static double meEvalBinary(MeBinary *binary, MeDef *defs, size_t size) {
	double left = meEval(binary->left, defs, size);
	if (isnan(left))
		return NAN;

	double right = meEval(binary->right, defs, size);
	if (isnan(right))
		return NAN;

	return meDoBinaryOp(binary->op, left, right, binary->base.pos);
}

static MeDef *meFindDef(int type, const char *name, MeDef *defs, size_t size) {
	for (size_t i = 0; i < size; ++ i) {
		if (defs[i].type != type)
			continue;

		if (strcmp(defs[i].name, name) == 0)
			return defs + i;
	}

	return NULL;
}

#define ME_DEFAULT_DEFS_SIZE (sizeof(meDefaultDefs) / sizeof(*meDefaultDefs))
static MeDef meDefaultDefs[] = {
	{ME_DEF_FUNC, "sqrt",  {.func = meSqrt}},
	{ME_DEF_FUNC, "cbrt",  {.func = meCbrt}},
	{ME_DEF_FUNC, "hypot", {.func = meHypot}},
	{ME_DEF_FUNC, "sin",   {.func = meSin}},
	{ME_DEF_FUNC, "cos",   {.func = meCos}},
	{ME_DEF_FUNC, "tan",   {.func = meTan}},
	{ME_DEF_FUNC, "log",   {.func = meLog}},
	{ME_DEF_FUNC, "floor", {.func = meFloor}},
	{ME_DEF_FUNC, "ceil",  {.func = meCeil}},
	{ME_DEF_FUNC, "round", {.func = meRound}},
	{ME_DEF_FUNC, "atan",  {.func = meAtan}},
	{ME_DEF_FUNC, "atan2", {.func = meAtan2}},
	{ME_DEF_FUNC, "abs",   {.func = meAbs}},
	{ME_DEF_FUNC, "pow",   {.func = mePow}},
	{ME_DEF_FUNC, "root",  {.func = meRoot}},

	{ME_DEF_CONST, "PI", {.num = 3.1415926535}},
};

static double meEvalId(MeId *id, MeDef *defs, size_t size) {
	MeDef *def;
	if (size >= 1) {
		if (defs[0].type & ME_INCLUDE_DEFAULT_CONSTS) {
			def = meFindDef(ME_DEF_CONST, id->value, meDefaultDefs, ME_DEFAULT_DEFS_SIZE);
			if (def != NULL)
				goto found;
		}
	}

	def = meFindDef(ME_DEF_CONST, id->value, defs, size);
	if (def == NULL)
		return meError(id->base.pos, "Undefined identifier \"%s\"", id->value);

found:
	return def->u.num;
}

static double meEvalFunc(MeFunc *func, MeDef *defs, size_t size) {
	double evaled[ME_MAX_ARGS];
	MeDef *def;
	if (size >= 1) {
		if (defs[0].type & ME_INCLUDE_DEFAULT_FUNCS) {
			def = meFindDef(ME_DEF_FUNC, func->name, meDefaultDefs, ME_DEFAULT_DEFS_SIZE);
			if (def != NULL)
				goto found;
		}
	}

	def = meFindDef(ME_DEF_FUNC, func->name, defs, size);
	if (def == NULL)
		return meError(func->base.pos, "Undefined function \"%s\"", func->name);

found:
	for (size_t i = 0; i < func->argsCount; ++ i) {
		evaled[i] = meEval(func->args[i], defs, size);
		if (isnan(evaled[i]))
			return NAN;
	}

	return def->u.func(func, evaled, func->argsCount);
}

#undef ME_DEFAULT_DEFS_SIZE

NOCH_DEF double meEval(MeExpr *this, MeDef *defs, size_t size) {
	switch (this->type) {
	case ME_NUMBER: return ME_NUMBER(this)->value;
	case ME_UNARY:  return meEvalUnary (ME_UNARY(this),  defs, size);
	case ME_BINARY: return meEvalBinary(ME_BINARY(this), defs, size);
	case ME_ID:     return meEvalId    (ME_ID(this),     defs, size);
	case ME_FUNC:   return meEvalFunc  (ME_FUNC(this),   defs, size);

	default: NOCH_ASSERT(0 && "Unknown MeExpr type");
	}
}

static double meEvalLiteralUnary(MeUnary *unary) {
	NOCH_ASSERT(unary->expr->type == ME_NUMBER);
	double value = ME_NUMBER(unary->expr)->value;

	return meDoUnaryOp(unary->op, value);
}

static double meEvalLiteralBinary(MeBinary *binary) {
	NOCH_ASSERT(binary->left->type == ME_NUMBER && binary->right->type == ME_NUMBER);
	double left  = ME_NUMBER(binary->left)->value;
	double right = ME_NUMBER(binary->right)->value;

	return meDoBinaryOp(binary->op, left, right, binary->base.pos);
}

NOCH_DEF MeExpr *meEvalLiterals(MeExpr *this) {
	if (this->type == ME_UNARY) {
		MeUnary *unary = ME_UNARY(this);

		unary->expr = meEvalLiterals(unary->expr);
		if (unary->expr == NULL)
			return NULL;

		if (unary->expr->type != ME_NUMBER)
			return this;

		MeNumber *result = meNewNumber(this->pos, meEvalLiteralUnary(unary));
		meDestroy(this);
		return (MeExpr*)result;
	} else if (this->type == ME_BINARY) {
		MeBinary *binary = ME_BINARY(this);

		binary->left = meEvalLiterals(binary->left);
		if (binary->left == NULL)
			return NULL;

		binary->right = meEvalLiterals(binary->right);
		if (binary->right == NULL)
			return NULL;

		if (binary->left->type != ME_NUMBER || binary->right->type != ME_NUMBER)
			return this;

		double resultValue = meEvalLiteralBinary(binary);
		if (isnan(resultValue))
			return NULL;

		MeNumber *result = meNewNumber(this->pos, resultValue);
		meDestroy(this);
		return (MeExpr*)result;
	}

	return this;
}

static void mePrintFNumber(double num, FILE *file) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%.13f", num);

	bool   found = false;
	size_t i;
	for (i = 0; buf[i] != '\0'; ++ i) {
		if (buf[i] == '.' && !found)
			found = true;
	}

	if (found) {
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
	}

	fprintf(file, "%s", buf);
}

NOCH_DEF void mePrintF(MeExpr *this, FILE *file, bool redundantParens) {
	bool parens = redundantParens && (this->type == ME_UNARY || this->type == ME_BINARY);

	if (this->parens || parens)
		fprintf(file, "(");

	switch (this->type) {
	case ME_NUMBER:
		mePrintFNumber(ME_NUMBER(this)->value, file);
		break;

	case ME_ID:
		fprintf(file, "%s", ME_ID(this)->value);
		break;

	case ME_UNARY:
		if (ME_UNARY(this)->op == ME_OP_ABS) {
			fprintf(file, "|");
			mePrintF(ME_UNARY(this)->expr, file, redundantParens);
			fprintf(file, "|");
		} else {
			fprintf(file, "%c", ME_UNARY(this)->op);
			mePrintF(ME_UNARY(this)->expr, file, redundantParens);
		}
		break;

	case ME_BINARY:
		mePrintF(ME_BINARY(this)->left, file, redundantParens);
		fprintf(file, " %c ", ME_BINARY(this)->op);
		mePrintF(ME_BINARY(this)->right, file, redundantParens);
		break;

	case ME_FUNC:
		fprintf(file, "%s(", ME_FUNC(this)->name);
		for (size_t i = 0; i < ME_FUNC(this)->argsCount; ++ i) {
			if (i > 0)
				fprintf(file, ", ");

			mePrintF(ME_FUNC(this)->args[i], file, redundantParens);
		}
		fprintf(file, ")");
		break;

	default: NOCH_ASSERT(0 && "Unknown MeExpr type");
	}

	if (this->parens || parens)
		fprintf(file, ")");
}

NOCH_DEF void meDestroy(MeExpr *this) {
	NOCH_ASSERT(this != NULL);

	switch (this->type) {
	case ME_NUMBER: case ME_ID: break;

	case ME_UNARY:
		meDestroy(ME_UNARY(this)->expr);
		break;

	case ME_BINARY:
		meDestroy(ME_BINARY(this)->left);
		meDestroy(ME_BINARY(this)->right);
		break;

	case ME_FUNC:
		for (size_t i = 0; i < ME_FUNC(this)->argsCount; ++ i)
			meDestroy(ME_FUNC(this)->args[i]);
		break;

	default: NOCH_ASSERT(0 && "Unknown MeExpr type");
	}

	nochFree(this);
}

NOCH_DEF double meInterp(const char *start, const char *end, MeDef *defs, size_t size) {
	MeExpr *expr = meParse(start, end);
	if (expr == NULL)
		return NAN;

	double result = meEval(expr, defs, size);
	meDestroy(expr);
	return result;
}

typedef struct {
	const char *start, *end, *it;

	char   data[ME_TOKEN_CAPACITY];
	size_t dataSize;
} MeParser;

#define ME_END(THIS)     ((THIS)->it >= (THIS)->end)
#define ME_CHAR(THIS)    (ME_END(THIS)? '\0' : *(THIS)->it)
#define ME_PEEK(THIS, N) ((THIS)->it + (N) >= (THIS)->end? '\0' : (THIS)->it[N])
#define ME_POS(THIS)     (size_t)((THIS)->it - (THIS)->start + 1)

inline static bool meCharIsOneOf(MeParser *this, const char *chs) {
	char ch = ME_CHAR(this);
	for (const char *it = chs; *it != '\0'; ++ it) {
		if (ch == *it)
			return true;
	}
	return false;
}

static int meSkipWhitespaces(MeParser *this) {
	while (isspace(ME_CHAR(this))) {
		if (*this->it == '\n') {
			meError(ME_POS(this), "Unexpected end of line\n");
			return -1;
		}

		++ this->it;
	}
	return 0;
}

static void meTokenDataClear(MeParser *this) {
	*this->data    = '\0';
	this->dataSize = 0;
}

static int meTokenDataAddChar(MeParser *this, char ch) {
	if (this->dataSize + 1 >= ME_TOKEN_CAPACITY) {
		meError(ME_POS(this), "Token exceeded maximum length of %i", ME_TOKEN_CAPACITY - 1);
		return -1;
	}

	this->data[this->dataSize ++] = ch;
	this->data[this->dataSize]    = '\0';
	return 0;
}

static MeExpr *meParseFactor(MeParser *this);
static MeExpr *meParseExpr  (MeParser *this);

static MeExpr *meParseId(MeParser *this) {
	NOCH_ASSERT(isalpha(*this->it));

	size_t pos = ME_POS(this);

	meTokenDataClear(this);
	while (isalnum(ME_CHAR(this))) {
		if (meTokenDataAddChar(this, *this->it ++) != 0)
			return NULL;
	}

	if (ME_CHAR(this) != '(')
		return (MeExpr*)meNewId(pos, this->data);

	MeFunc *func = meNewFunc(pos, this->data);

	++ this->it;
	if (meSkipWhitespaces(this) != 0)
		goto fail;

	if (*this->it == ')')
		return (MeExpr*)func;

	while (true) {
		if (func->argsCount >= ME_MAX_ARGS) {
			meError(ME_POS(this), "Exceeded maximum amount of %i function arguments", ME_MAX_ARGS);
			goto fail;
		}

		MeExpr *expr = meParseExpr(this);
		if (expr == NULL)
			goto fail;

		func->args[func->argsCount ++] = expr;

		if (ME_CHAR(this) == ')')
			break;
		else if (ME_CHAR(this) != ',') {
			meError(ME_POS(this), "Expected a \",\" or a matching \")\"");
			goto fail;
		}

		++ this->it;
	}
	++ this->it;
	return (MeExpr*)func;

fail:
	meDestroy((MeExpr*)func);
	return NULL;
}

static MeExpr *meParseNumber(MeParser *this) {
	NOCH_ASSERT(isdigit(*this->it));

	size_t pos = ME_POS(this);
	bool exponent = false, floatingPoint = false;

	meTokenDataClear(this);
	while (!ME_END(this)) {
		if (*this->it == '_') {
			++ this->it;
			continue;
		} else if (*this->it == 'e') {
			if (exponent) {
				meError(ME_POS(this), "Encountered exponent in number twice");
				return NULL;
			}

			exponent = true;
			if (ME_PEEK(this, 1) == '+' || ME_PEEK(this, 1) == '-') {
				if (meTokenDataAddChar(this, *this->it ++) != 0)
					return NULL;
			}

			if (!isdigit(ME_PEEK(this, 1))) {
				meError(ME_POS(this), "expected a digit after exponent");
				return NULL;
			}
		} else if (*this->it == '.') {
			if (floatingPoint) {
				meError(ME_POS(this), "Encountered floating point in number twice");
				return NULL;
			} else if (exponent) {
				meError(ME_POS(this), "Unexpected floating point in exponent");
				return NULL;
			}

			floatingPoint = true;
			if (!isdigit(ME_PEEK(this, 1))) {
				meError(ME_POS(this), "expected a digit after floating point");
				return NULL;
			}
		} else if (!isdigit(*this->it))
			break;

		if (meTokenDataAddChar(this, *this->it ++) != 0)
			return NULL;
	}

	return (MeExpr*)meNewNumber(pos, atof(this->data));
}

static MeExpr *meParseUnary(MeParser *this) {
	NOCH_ASSERT(*this->it == '+' || *this->it == '-');

	size_t pos = ME_POS(this);
	char   op  = *this->it ++;

	MeExpr *expr = meParseFactor(this);
	if (expr == NULL)
		return NULL;

	return (MeExpr*)meNewUnary(pos, op, expr);
}

static MeExpr *meParseParens(MeParser *this) {
	NOCH_ASSERT(*this->it == '(' || *this->it == '[');

	size_t pos     = ME_POS(this);
	char   closing = *this->it ++ == '('? ')' : ']';

	MeExpr *expr = meParseExpr(this);
	if (expr == NULL)
		return NULL;

	expr->parens = true;

	if (ME_CHAR(this) != closing) {
		meDestroy(expr);
		meError(pos, "Expected a matching \"%c\"", closing);
		return NULL;
	}

	++ this->it;
	return expr;
}

static MeExpr *meParsePipes(MeParser *this) {
	NOCH_ASSERT(*this->it == '|');

	size_t pos = ME_POS(this);
	++ this->it;

	MeExpr *expr = meParseExpr(this);
	if (expr == NULL)
		return NULL;

	if (ME_CHAR(this) != '|') {
		meDestroy(expr);
		meError(pos, "Expected a matching \"|\"");
		return NULL;
	}

	++ this->it;
	return (MeExpr*)meNewUnary(pos, ME_OP_ABS, expr);
}

static MeExpr *meParseFactor(MeParser *this) {
	if (meSkipWhitespaces(this) != 0)
		return NULL;

	MeExpr *parsed;
	switch (ME_CHAR(this)) {
	case '\0':
		meError(ME_POS(this), "Unexpected end of input");
		return NULL;

	case '|':
		parsed = meParsePipes(this);
		break;

	case '+': case '-': parsed = meParseUnary(this);  break;
	case '(': case '[': parsed = meParseParens(this); break;

	default:
		if (isalpha(*this->it))
			parsed = meParseId(this);
		else if (isdigit(*this->it))
			parsed = meParseNumber(this);
		else {
			meError(ME_POS(this), "Unexpected character \"%c\"", *this->it);
			return NULL;
		}
	}

	if (meSkipWhitespaces(this) != 0) {
		meDestroy(parsed);
		return NULL;
	}

	return parsed;
}

static MeExpr *meParseExponent(MeParser *this) {
	MeExpr *left = meParseFactor(this);
	if (left == NULL)
		return NULL;

	while (ME_CHAR(this) == '^') {
		size_t pos = ME_POS(this);
		char   op  = *this->it ++;

		MeExpr *right = meParseExponent(this);
		if (right == NULL) {
			meDestroy(left);
			return NULL;
		}

		left = (MeExpr*)meNewBinary(pos, op, left, right);
	}

	return left;
}

static MeExpr *meParseTerm(MeParser *this) {
	MeExpr *left = meParseExponent(this);
	if (left == NULL)
		return NULL;

	while (meCharIsOneOf(this, "*/%(") || isalpha(ME_CHAR(this))) {
		size_t pos = ME_POS(this);
		char   op;
		if (*this->it == '(' || isalpha(*this->it)) {
			if (isspace(this->it[-1])) {
				if (*this->it == 'x' && isspace(ME_PEEK(this, 1)))
					++ this->it;
				else
					break;
			}

			op = ME_OP_MUL;
		} else
			op = *this->it ++;

		MeExpr *right = meParseExponent(this);
		if (right == NULL) {
			meDestroy(left);
			return NULL;
		}

		left = (MeExpr*)meNewBinary(pos, op, left, right);
	}

	return left;
}

static MeExpr *meParseExpr(MeParser *this) {
	MeExpr *left = meParseTerm(this);
	if (left == NULL)
		return NULL;

	while (meCharIsOneOf(this, "+-")) {
		size_t pos = ME_POS(this);
		char   op  = *this->it ++;

		MeExpr *right = meParseTerm(this);
		if (right == NULL) {
			meDestroy(left);
			return NULL;
		}

		left = (MeExpr*)meNewBinary(pos, op, left, right);
	}

	return left;
}

NOCH_DEF MeExpr *meParse(const char *start, const char *end) {
	NOCH_ASSERT(start != NULL);

	if (end == NULL)
		end = start + strlen(start);
	else {
		NOCH_ASSERT(start <= end);
		for (const char *it = start; it < end; ++ it)
			NOCH_ASSERT(*it != '\0');
	}

	MeParser parser = {0};
	parser.start = start;
	parser.end   = end;
	parser.it    = start;

	MeExpr *expr = meParseExpr(&parser);

	if (!ME_END(&parser)) {
		meError(ME_POS(&parser), "Expected end of input");
		meDestroy(expr);
		return NULL;
	}

	return expr;
}

#undef ME_END
#undef ME_CHAR
#undef ME_PEEK

NOCH_DEF double meWrongAmountOfArgs(MeFunc *func, size_t expected) {
	return meError(func->base.pos, "Function \"%s\" Expected %lu arguments, got %lu",
	               func->name, (long unsigned)expected, (long unsigned)func->argsCount);
}

#define ME_BIND_NATIVE(NAME, C_FUNC, ARGC, ...)                      \
	NOCH_DEF double NAME(MeFunc *this, double *args, size_t count) { \
		if (count != ARGC)                                           \
			return meWrongAmountOfArgs(this, ARGC);                  \
		return C_FUNC(__VA_ARGS__);                                  \
	}

ME_BIND_NATIVE(meSqrt,  sqrt,  1, args[0])
ME_BIND_NATIVE(meCbrt,  cbrt,  1, args[0])
ME_BIND_NATIVE(meHypot, hypot, 2, args[0], args[1])
ME_BIND_NATIVE(meSin,   sin,   1, args[0])
ME_BIND_NATIVE(meCos,   cos,   1, args[0])
ME_BIND_NATIVE(meTan,   tan,   1, args[0])
ME_BIND_NATIVE(meLog,   log,   1, args[0])
ME_BIND_NATIVE(meFloor, floor, 1, args[0])
ME_BIND_NATIVE(meCeil,  ceil,  1, args[0])
ME_BIND_NATIVE(meRound, round, 1, args[0])
ME_BIND_NATIVE(meAtan,  atan,  1, args[0])
ME_BIND_NATIVE(meAtan2, atan2, 2, args[0], args[1])
ME_BIND_NATIVE(meAbs,   fabs,  1, args[0])
ME_BIND_NATIVE(mePow,   pow,   2, args[0], args[1])

NOCH_DEF double meRoot(MeFunc *this, double *args, size_t count) {
	if (count != 2)
		return meWrongAmountOfArgs(this, 2);

	if (args[1] == 0)
		return meFuncError(this, "Cannot get 0th root of %f", args[0]);

	return pow(args[0], 1 / args[1]);
}

#undef ME_BIND_NATIVE

NOCH_DEF MeDef meInclude(int what) {
	MeDef def = {0};
	def.type = what;
	return def;
}

NOCH_DEF MeDef meDefFunc(const char *name, MeNative native) {
	NOCH_ASSERT(strlen(name) < ME_TOKEN_CAPACITY);

	MeDef def  = {0};
	def.type   = ME_DEF_FUNC;
	def.u.func = native;
	strcpy(def.name, name);
	return def;
}

NOCH_DEF MeDef meDefConst(const char *name, double value) {
	NOCH_ASSERT(strlen(name) < ME_TOKEN_CAPACITY);

	MeDef def = {0};
	def.type  = ME_DEF_CONST;
	def.u.num = value;
	strcpy(def.name, name);
	return def;
}

#ifdef __cplusplus
}
#endif
