#include <stdio.h>

#include <noch/hashmap.h>
#include <noch/hashmap.c>

void printValue(HashmapString *map, const char *key) {
	const char **ref = hashmapGet(map, key);
	if (ref == NULL)
		printf("%s: EMPTY\n", key);
	else
		printf("%s: \"%s\"\n", key, *ref);
}

int main(void) {
	HashmapString map;
	hashmapInit(&map);

	hashmapSet(&map, "greeting",      "Hello, world!");
	hashmapSet(&map, "test",          "foo bar");
	hashmapSet(&map, "to-be-removed", "everything");

	hashmapRemove(&map, "to-be-removed");

	hashmapSet(&map, "greeting", "Hello there!");

	/* It is also possible to set a key like this, but the key has to already exist */
	*hashmapGet(&map, "test") = "foo baz";

	printValue(&map, "greeting");
	printValue(&map, "test");
	printValue(&map, "to-be-removed");
	printValue(&map, "non-existant-key");

	printf("\nFOREACH_IN_HASHMAP:\n");
	FOREACH_IN_HASHMAP(&map, ref, key, {
		printf("%s: \"%s\"\n", key, *(const char**)ref);
	});

	hashmapDeinit(&map);
	return 0;
}
