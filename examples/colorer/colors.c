#include <stdio.h>

#include <noch/colorer.h>
#include <noch/colorer.c>

int main(void) {
	printf("Colored output: ");
	colorSet(COLOR_RED, COLOR_BRIGHT_WHITE);
	printf("Hello, world!");
	colorReset();
	printf("\n");

	printf("Highlight foreground: ");
	colorHighlight();
	printf("Hello, world!");
	colorReset();
	printf("\n");

	colorPrint("Color format function: [W][*r]H[*y]e[*g]l[*c]l[*b]o[*m],[.] [!]world![.]\n");

	fprintf(stderr, "Stream-specific colored output: ");
	colorSetF(stderr, COLOR_RED, COLOR_BRIGHT_WHITE);
	fprintf(stderr, "Hello, world!");
	colorResetF(stderr);
	fprintf(stderr, "\n");

	return 0;
}
