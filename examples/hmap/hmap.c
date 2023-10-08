#include <stdio.h>

#include "../../hmap.h"
#include "../../hmap.c"

void print_val(hmap_cstr_t *hmap, const char *key) {
	const char **ref = HMAP_GET(hmap, key);
	if (ref == NULL)
		printf("%s: EMPTY\n", key);
	else
		printf("%s: \"%s\"\n", key, *ref);
}

int main(void) {
	hmap_cstr_t map;
	HMAP_INIT(&map);

	HMAP_SET(&map, "greeting",      "Hello, world!");
	HMAP_SET(&map, "test",          "foo bar");
	HMAP_SET(&map, "to-be-removed", "everything");

	HMAP_REMOVE(&map, "to-be-removed");

	HMAP_SET(&map, "greeting", "Hello there!");

	/* It is also possible to set a key like this, but the key has to already exist */
	*HMAP_GET(&map, "test") = "foo baz";

	print_val(&map, "greeting");
	print_val(&map, "test");
	print_val(&map, "to-be-removed");
	print_val(&map, "non-existant-key");

	HMAP_DEINIT(&map);
	return 0;
}
