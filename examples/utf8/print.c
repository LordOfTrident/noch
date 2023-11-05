#include <stdio.h>

#include <noch/utf8.h>
#include <noch/utf8.c>

int main(void) {
	const char *str = "hello π \u2588 lol";

	printf("Length: %i\n", (int)stringU8Length(str));
	printf("Bytes: %i\n",  (int)stringU8Size(str));

	printf("Normal printing:\n\t");

	const char *it = str;
	while (*it != '\0') {
		size_t size;
		Rune   rune = runeFromU8(it, &size);
		it += size;

		char buf[sizeof(Rune) + 1] = {0};
		runeToU8(rune, buf);
		printf("%s", buf);
	}

	printf("\nReverse printing:\n\t");

	it = str + stringU8Size(str);
	while ((it = stringU8Prev(it, str)) != NULL) {
		Rune rune = runeFromU8(it, NULL);

		char buf[sizeof(Rune) + 1] = {0};
		runeToU8(rune, buf);
		printf("%s", buf);
	}

	printf("\n");
	return 0;
}
