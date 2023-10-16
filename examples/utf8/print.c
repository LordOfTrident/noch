#include <stdio.h>

#include <noch/utf8.h>
#include <noch/utf8.c>

int main(void) {
	const char *str = "hello π \uE0B3 lol";

	printf("Length: %i\n", (int)u8_str_len(str));
	printf("Bytes: %i\n",  (int)u8_str_bytes(str));

	printf("Normal printing:\n\t");

	const char *it = str;
	while (*it != '\0') {
		size_t size;
		rune_t r = rune_decode_u8(it, &size);
		it += size;

		char buf[sizeof(rune_t) + 1] = {0};
		rune_encode_u8(r, buf);
		printf("%s", buf);
	}

	printf("\nReverse printing:\n\t");

	it = str + u8_str_bytes(str);
	while ((it = u8_str_prev(it, str)) != NULL) {
		rune_t r = rune_decode_u8(it, NULL);

		char buf[sizeof(rune_t) + 1] = {0};
		rune_encode_u8(r, buf);
		printf("%s", buf);
	}

	printf("\n");
	return 0;
}
