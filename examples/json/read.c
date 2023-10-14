#include <stdio.h>  /* fprintf, printf, fopen, fclose */
#include <stdlib.h> /* EXIT_FAILURE */

#include "../../json.h"
#include "../../json.c"

int main(void) {
	const char *path = "examples/json/data.json";
	printf("Reading '%s'\n", path);

	size_t  row, col;
	json_obj_t *obj;
	JSON_EXPECT_OBJ(obj, json_from_file(path, &row, &col), {
		printf("Error: %s: Expected data to be an object, got %s\n",
		       path, json_type_to_str(_recieved_json->type));
		return EXIT_FAILURE;
	});

	if (obj == NULL) {
		if (noch_get_err() == NOCH_ERR_PARSER)
			printf("Error: %s:%i:%i: %s\n", path, (int)row, (int)col, noch_get_err_msg());
		else
			printf("Error: %s: %s\n", path, noch_get_err_msg());

		return EXIT_FAILURE;
	}

	JSON_FPRINT(obj, stdout);
	JSON_DESTROY(obj);
	return 0;
}
