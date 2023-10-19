#include <stdio.h>

#include <noch/common.h>
#include <noch/common.c>

int main(void) {
	printf("%%f vs %%g vs putn\n");

	printf("\nPrinting 123456789.123456789\n");
	printf("%%f: \t%f\n", 123456789.123456789);
	printf("%%g: \t%g\n", 123456789.123456789);
	printf("%%.13f: \t%.13f\n", 123456789.123456789);
	printf("%%.13g: \t%.13g\n", 123456789.123456789);
	printf("putn: \t");
	putn(123456789.123456789);
	printf("\n");

	printf("\nPrinting 123456789101112.123\n");
	printf("%%f: \t%f\n", 123456789101112.123);
	printf("%%g: \t%g\n", 123456789101112.123);
	printf("%%.13f: \t%.13f\n", 123456789101112.123);
	printf("%%.13g: \t%.13g\n", 123456789101112.123);
	printf("putn: \t");
	putn(123456789101112.123);
	printf("\n");

	return 0;
}
