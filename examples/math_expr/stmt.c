#include <stdio.h>  /* fprintf, printf */
#include <stdlib.h> /* exit, EXIT_FAILURE */

#include <noch/math_expr.h>
#include <noch/math_expr.c>

void eval(me_ctx_t *ctx, const char *in) {
	printf("> %s\n", in);

	size_t row, col;
	me_stmt_t *stmt = me_parse_stmt(in, &row, &col);
	if (stmt == NULL) {
		fprintf(stderr, "Error: %i:%i: %s\n", (int)row, (int)col, noch_get_err_msg());
		exit(EXIT_FAILURE);
	}

	me_result_t result = me_ctx_eval_stmt(ctx, stmt);
	if (result.err != NULL) {
		fprintf(stderr, "Error: %s: %s\n", result.origin, result.err);
		exit(EXIT_FAILURE);
	}

	me_stmt_fprint(stmt, stdout);
	if (!result.none)
		printf(" = %f", result.val);

	printf("\n");
	me_stmt_destroy(stmt);
}

int main(void) {
	me_ctx_t ctx_, *ctx = &ctx_;
	me_ctx_init(ctx);

	me_ctx_register_basic_fns(ctx);
	me_ctx_set_id(ctx, "pi", 3.1415926535, true);

	eval(ctx, "-2cos(pi)");
	eval(ctx, "x = 2pi / 6");
	eval(ctx, "2x");
	eval(ctx, "x = 5");
	eval(ctx, "2x");
	eval(ctx, "pi = 6");

	me_ctx_deinit(ctx);
	return 0;
}
