#include <stdio.h>  /* fprintf, printf */
#include <stdlib.h> /* EXIT_FAILURE */

#include <noch/mexpr.h>
#include <noch/mexpr.c>

int main(void) {
	const char *in = "5(10 / [1 * (1 + 1)]) - 2^4 * 2";
	printf("Input: %s\n\n", in);

	size_t row, col;
	mexpr_t *expr = mexpr_parse(in, &row, &col);
	if (expr == NULL) {
		fprintf(stderr, "Error: %i:%i: %s\n", (int)row, (int)col, noch_get_err_msg());
		return EXIT_FAILURE;
	}

	mexpr_fprint(expr, stdout);
	printf(" = %f\n", mexpr_eval(expr, NULL));

	mexpr_destroy(expr);
	return 0;
}
