#include <noch/json.h>
#include <noch/json.c>

int main(void) {
	JsonObj *json = jsonNewObj();
	assert(json != NULL);

	jsonObjSet(json, "key",      jsonNewString("value"));
	jsonObjSet(json, "pi",       jsonNewFloat(3.1415926535));
	jsonObjSet(json, "my-int",   jsonNewInt(1024));
	jsonObjSet(json, "my-float", jsonNewFloat(5.0));

	JsonList *fruits = jsonNewList();
	assert(fruits != NULL);

	jsonListPush(fruits, jsonNewString("apple"));
	jsonListPush(fruits, jsonNewString("orange"));
	jsonListPush(fruits, jsonNewString("banana"));
	jsonListPush(fruits, jsonNewString("pear"));

	jsonObjSet(json, "fruits", (Json*)fruits);

	jsonPrintF(json, stdout);
	jsonDestroy(json);
	return 0;
}
