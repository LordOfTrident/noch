#include <stdio.h>

#include "../../sv.h"
#include "../../sv.c"

void a(void) {
	sv_t sv     = sv_cstr("Hello, world!");
	sv_t substr = sv_substr(sv, 7, 5);
	printf("Inspecting '" SV_FMT "':\n", SV_ARG(sv));

	bool contains = sv_contains_substr(sv, substr);
	printf("  contains '" SV_FMT "': %i\n", SV_ARG(substr), contains);
	printf("    -> at %zu\n", sv_find_substr(sv, substr));
}

void b(void) {
	sv_t sv = sv_cstr("foo bar baz");
	char ch = 'b';
	printf("Inspecting '" SV_FMT "':\n", SV_ARG(sv));

	bool contains = sv_contains(sv, ch);
	printf("  contains '%c': %i\n", ch, contains);
	printf("    -> first at %zu\n",     sv_find_first(    sv, ch));
	printf("    -> last  at %zu\n\n",   sv_find_last(     sv, ch));
	printf("    -> first not at %zu\n", sv_find_first_not(sv, ch));
	printf("    -> last  not at %zu\n", sv_find_last_not( sv, ch));
	printf("  contains '%s' %zu times\n", "ba", sv_count_substr(sv, sv_cstr("ba")));
}

int main(void) {
	a();
	b();

	return 0;
}
