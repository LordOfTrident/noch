#include <stdio.h>  /* fprintf, printf */
#include <stdlib.h> /* exit, EXIT_FAILURE */

#include <noch/math_expr.h>
#include <noch/math_expr.c>

/* #include <noch/common.h> */
/* #include <noch/common.c> */

void eval(me_ctx_t *ctx, const char *in) {
	printf("Input: %s\n\n", in);

	size_t row, col;
	me_expr_t *expr = me_parse_expr(in, &row, &col);
	if (expr == NULL) {
		fprintf(stderr, "Error: %i:%i: %s\n", (int)row, (int)col, noch_get_err_msg());
		exit(EXIT_FAILURE);
	}

	me_result_t result = me_ctx_eval_expr(ctx, expr);
	if (result.err != NULL) {
		fprintf(stderr, "Error: %s: %s\n", result.origin, result.err);
		exit(EXIT_FAILURE);
	}

	me_expr_fprint(expr, stdout);
	printf(" = %f\n", result.val);

	/* fputn/putn is a nicer and more precise way to print floats, from noch/common
	printf(" = ");
	putn(result.val);
	printf("\n");
	*/

	me_expr_destroy(expr);
}

void set_const(me_ctx_t *ctx, const char *name, double val) {
	me_ctx_set_id(ctx, name, val, true);
	printf("%s = %f\n", name, val);
}

int main(void) {
	me_ctx_t ctx_, *ctx = &ctx_;
	me_ctx_init(ctx);

	eval(ctx, "|5(10 / [1 * (1 + 5e+5)]) - 2^4 * 2 + 0.5 + 0.25 * 2|");
	printf("\n--------\n");

	me_ctx_register_basic_fns(ctx);
	set_const(ctx, "a", 5);
	set_const(ctx, "b", 10);
	set_const(ctx, "x", 0.75);
	set_const(ctx, "y", 2.25);
	printf("\n");
	eval(ctx, "5(a + b) - atan2(x, y) * 2a");
	printf("\n--------\n");

	eval(ctx, "|2cos(0) * -0.5| + a/(0.5b)");
	printf("\n--------\n");

	eval(ctx, "root(25, 2) + root(256, 8)");
	printf("\n--------\n");

	eval(ctx, "5 x 5 + 5 * 5");
	printf("\n--------\n");

	set_const(ctx, "x", 5);
	eval(ctx, "x x x + x * x");

	me_ctx_deinit(ctx);
	return 0;
}
