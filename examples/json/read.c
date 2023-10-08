#include <stdio.h>  /* fprintf, printf, fopen, fclose */
#include <stdlib.h> /* EXIT_FAILURE */

#include "../../json.h"
#include "../../json.c"

int main(void) {
	const char *path = "examples/json/data.json";
	printf("Reading '%s'\n", path);

	size_t  row, col;
	json_t *json = json_from_file(path, &row, &col);
	if (json == NULL) {
		if (noch_get_err() == NOCH_ERR_PARSER)
			printf("Error: %s:%i:%i: %s\n", path, (int)row, (int)col, noch_get_err_msg());
		else
			printf("Error: %s: %s\n", path, noch_get_err_msg());

		return EXIT_FAILURE;
	}

	json_fprint(json, stdout);
	json_destroy(json);
	return 0;
}
