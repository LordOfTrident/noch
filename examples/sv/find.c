#include <stdio.h>

#include <noch/sv.h>
#include <noch/sv.c>

void a(void) {
	StringView sv     = svFromString("Hello, world!");
	StringView substr = svSubstring(sv, 7, 5);
	printf("Inspecting '" SV_FMT "':\n", SV_ARG(sv));

	bool contains = svContainsSubstring(sv, substr);
	printf("  contains '" SV_FMT "': %i\n", SV_ARG(substr), contains);
	printf("    -> at %zu\n", svFindSubstring(sv, substr));
}

void b(void) {
	StringView sv = svFromString("foo bar baz");
	char ch = 'b';
	printf("Inspecting '" SV_FMT "':\n", SV_ARG(sv));

	bool contains = svContains(sv, ch);
	printf("  contains '%c': %i\n", ch, contains);
	printf("    -> first at %zu\n",     svFindFirst(    sv, ch));
	printf("    -> last  at %zu\n\n",   svFindLast(     sv, ch));
	printf("    -> first not at %zu\n", svFindFirstNot(sv, ch));
	printf("    -> last  not at %zu\n", svFindLastNot( sv, ch));
	printf("  contains '%s' %zu times\n", "ba", svCountSubstrings(sv, svFromString("ba")));
}

int main(void) {
	a();
	b();

	return 0;
}
