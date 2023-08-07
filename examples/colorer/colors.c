#include <stdio.h>

#include "../../colorer.h"
#include "../../colorer.c"

int main(void) {
	init_color();

	printf("Colored output: ");
	set_color(COLOR_RED, COLOR_BRIGHT_WHITE);
	printf("Hello, world!");
	reset_color();
	printf("\n");

	printf("Highlight foreground: ");
	highlight_fg();
	printf("Hello, world!");
	reset_color();
	printf("\n");

	printf_color("Color format function: #fW#brH#bye#bgl#bcl#bbo#bm,#X#! world!#X\n");

	fprintf(stderr, "Stream-specific colored output: ");
	fset_color(stderr, COLOR_RED, COLOR_BRIGHT_WHITE);
	fprintf(stderr, "Hello, world!");
	freset_color(stderr);
	fprintf(stderr, "\n");

	return 0;
}
