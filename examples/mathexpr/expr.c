#include <stdio.h>  /* fprintf */
#include <stdlib.h> /* exit, EXIT_FAILURE */

#include <noch/mathexpr.h>
#include <noch/mathexpr.c>

double sum(MeFunc *this, double *args, size_t count) {
	(void)this;

	double sum = 0;
	for (size_t i = 0; i < count; ++ i)
		sum += args[i];

	return sum;
}

void eval(const char *in) {
	MeExpr *expr = meParse(in, NULL);
	if (expr == NULL) {
		fprintf(stderr, "Error: %s\n", nochGetError());
		exit(EXIT_FAILURE);
	}

	expr = meEvalLiterals(expr);
	if (expr == NULL) {
		fprintf(stderr, "Error: %s\n", nochGetError());
		exit(EXIT_FAILURE);
	}

	MeDef defs[] = {
		/* The *first* element of the defs array can be a meInclude */
		meInclude(ME_INCLUDE_DEFAULT_FUNCS | ME_INCLUDE_DEFAULT_CONSTS),

		meDefFunc("sum", sum),
		meDefConst("a", 5),
		meDefConst("b", 10),
		meDefConst("x", 0.75),
		meDefConst("y", 2.25),
	};
	double result = meEval(expr, defs, sizeof(defs) / sizeof(*defs));
	if (isnan(result)) {
		fprintf(stderr, "Error: %s\n", nochGetError());
		exit(EXIT_FAILURE);
	}

	mePrintF(expr, stdout, false);
	fprintf(stdout, " = %f\n", result);
	meDestroy(expr);
}

int main(void) {
	eval("|5(10 / [1 * (1 + 5e+5)]) - 2^4 * 2 + 0.5 + 0.25 * 2|");
	eval("5(a + b) - atan2(x, y) * 2a");
	eval("|2cos(0) * -0.5| + a/(0.5b)");
	eval("root(25, 2) + root(256, 8)");
	eval("5 x 5 + 5 * 5");
	eval("x x x + x * x");
	eval("sum(1, 5, 2, 4)");
	return 0;
}
