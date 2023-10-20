#include <stdio.h>  /* fprintf, printf */
#include <stdlib.h> /* exit, EXIT_FAILURE */

#include <noch/mexpr.h>
#include <noch/mexpr.c>

/* #include <noch/common.h> */
/* #include <noch/common.c> */

void eval(mctx_t *ctx, const char *in) {
	printf("Input: %s\n\n", in);

	size_t row, col;
	mexpr_t *expr = mexpr_parse(in, &row, &col);
	if (expr == NULL) {
		fprintf(stderr, "Error: %i:%i: %s\n", (int)row, (int)col, noch_get_err_msg());
		exit(EXIT_FAILURE);
	}

	mctx_result_t result = mctx_eval(ctx, expr);
	if (result.err != NULL) {
		fprintf(stderr, "Error: %s: %s\n", result.origin, result.err);
		exit(EXIT_FAILURE);
	}

	mexpr_fprint(expr, stdout);
	printf(" = %f\n", result.val);

	/* fputn/putn is a nicer and more precise way to print floats, from noch/common
	printf(" = ");
	putn(result.val);
	printf("\n");
	*/

	mexpr_destroy(expr);
}

void register_const(mctx_t *ctx, const char *name, double val) {
	mctx_register_const(ctx, name, val);
	printf("%s = %f\n", name, val);
}

int main(void) {
	mctx_t ctx_, *ctx = &ctx_;
	mctx_init(ctx);

	eval(ctx, "|5(10 / [1 * (1 + 5e+5)]) - 2^4 * 2 + 0.5 + 0.25 * 2|");
	printf("\n--------\n");

	mctx_register_basic_fns(ctx);
	register_const(ctx, "a", 5);
	register_const(ctx, "b", 10);
	register_const(ctx, "x", 0.75);
	register_const(ctx, "y", 2.25);
	printf("\n");
	eval(ctx, "5(a + b) - atan2(x, y) * 2a");
	printf("\n--------\n");

	eval(ctx, "|2cos(0) * -0.5| + a/(0.5b)");
	printf("\n--------\n");

	eval(ctx, "root(25, 2) + root(256, 8)");
	printf("\n--------\n");

	eval(ctx, "5 x 5 + 5 * 5");
	printf("\n--------\n");

	register_const(ctx, "x", 5);
	eval(ctx, "x x x + x * x");

	mctx_deinit(ctx);
	return 0;
}
