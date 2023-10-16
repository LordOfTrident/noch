#include <noch/json.h>
#include <noch/json.c>

int main(void) {
	json_obj_t *json = json_new_obj();
	assert(json != NULL);

#define MUST(X) assert((X) == 0)

	MUST(JSON_OBJ_ADD(json, "key",      json_new_str("value")));
	MUST(JSON_OBJ_ADD(json, "pi",       json_new_float(3.1415926535)));
	MUST(JSON_OBJ_ADD(json, "my-int",   json_new_int(1024)));
	MUST(JSON_OBJ_ADD(json, "my-float", json_new_float(5.0)));

	json_list_t *fruits = json_new_list();
	assert(fruits != NULL);

	MUST(JSON_LIST_ADD(fruits, json_new_str("apple")));
	MUST(JSON_LIST_ADD(fruits, json_new_str("orange")));
	MUST(JSON_LIST_ADD(fruits, json_new_str("banana")));
	MUST(JSON_LIST_ADD(fruits, json_new_str("pear")));

	MUST(JSON_OBJ_ADD(json, "fruits", (json_t*)fruits));

	JSON_FPRINT(json, stdout);
	JSON_DESTROY(json);
	return 0;
}
