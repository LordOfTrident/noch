#include <stdio.h>  /* fprintf, printf */
#include <stdlib.h> /* exit, EXIT_FAILURE */

#include <noch/mexpr.h>
#include <noch/mexpr.c>

/* #include <noch/common.h> */
/* #include <noch/common.c> */

void eval_example(void) {
	const char *in = "5(10 / [1 * (1 + 5e+5)]) - 2^4 * 2 + 0.5 + 0.25 * 2";
	printf("Input: %s\n\n", in);

	size_t row, col;
	mexpr_t *expr = mexpr_parse(in, &row, &col);
	if (expr == NULL) {
		fprintf(stderr, "Error: %i:%i: %s\n", (int)row, (int)col, noch_get_err_msg());
		exit(EXIT_FAILURE);
	}

	mexpr_fprint(expr, stdout);
	printf(" = %f\n", mexpr_eval(expr, NULL));

	/* fputn/putn is a nicer and more precise way to print floats, from noch/common
	printf(" = ");
	putn(mexpr_eval(expr, NULL));
	printf("\n");
	*/

	mexpr_destroy(expr);
}

void parse_example(void) {
	const char *in = "5(a + b) - atan2(x, y) * 2a";
	printf("Input: %s\n\n", in);

	size_t row, col;
	mexpr_t *expr = mexpr_parse(in, &row, &col);
	if (expr == NULL) {
		fprintf(stderr, "Error: %i:%i: %s\n", (int)row, (int)col, noch_get_err_msg());
		exit(EXIT_FAILURE);
	}

	mexpr_fprint(expr, stdout);
	mexpr_destroy(expr);
}

int main(void) {
	parse_example();
	printf("\n\n");
	eval_example();
	return 0;
}
