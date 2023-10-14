#include "../../json.h"
#include "../../json.c"

int main(void) {
	json_obj_t *json = json_new_obj();
	assert(json != NULL);

#define MUST(X) assert((X) == 0)

	MUST(json_obj_add(json, "key",      (json_t*)json_new_str("value")));
	MUST(json_obj_add(json, "pi",       (json_t*)json_new_float(3.1415926535)));
	MUST(json_obj_add(json, "my-int",   (json_t*)json_new_int(1024)));
	MUST(json_obj_add(json, "my-float", (json_t*)json_new_float(5.0)));

	json_list_t *fruits = json_new_list();
	assert(fruits != NULL);

	MUST(json_list_add(fruits, (json_t*)json_new_str("apple")));
	MUST(json_list_add(fruits, (json_t*)json_new_str("orange")));
	MUST(json_list_add(fruits, (json_t*)json_new_str("banana")));
	MUST(json_list_add(fruits, (json_t*)json_new_str("pear")));

	MUST(json_obj_add(json, "fruits", (json_t*)fruits));

	json_fprint((json_t*)json, stdout);
	json_destroy((json_t*)json);
	return 0;
}
