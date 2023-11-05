#include <stdio.h>  /* fprintf, printf */
#include <stdlib.h> /* EXIT_FAILURE */

#include <noch/json.h>
#include <noch/json.c>

int main(void) {
	const char *path = "examples/json/data.json";
	printf("Reading '%s'\n", path);

	Json *json = jsonFromFile(path);
	if (json == NULL) {
		fprintf(stderr, "Error: %s\n", nochGetError());
		return EXIT_FAILURE;
	}

	jsonPrintF(json, stdout);
	jsonDestroy(json);
	return 0;
}
