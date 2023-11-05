#include <stdio.h>  /* printf, fprintf, FILE, stderr */
#include <stdlib.h> /* exit, EXIT_FAILURE */
#include <string.h> /* strcmp */
#include <assert.h> /* assert */

#include <noch/args.h> /* Declarations */
#include <noch/args.c> /* Implementation */

#define DESC "Draw a line in the terminal"
const char *usages[] = {
	"[OPTIONS]",
	"vertical [OPTIONS]      Draw a vertical line",
	"horizontal [OPTIONS]    Draw a horizontal line",
};

const char *argv0;

bool   fHelp   = false, fVersion = false;
size_t fLength = 6;
char   fChar   = '#';

void usage(FILE *file) {
	argsUsage(file, argv0, usages, sizeof(usages) / sizeof(*usages), DESC, true);
}

void version(FILE *file) {
	fprintf(file, "Version x.y.z\n");
}

void parseOptions(Args *args) {
	if (argsParseFlags(args, NULL) != 0) {
		fprintf(stderr, "Error: %s\n", nochGetError());
		exit(EXIT_FAILURE);
	}

	if (fHelp) {
		usage(stdout);
		exit(0);
	} else if (fVersion) {
		version(stdout);
		exit(0);
	}
}

void drawVertical(char ch, size_t len) {
	if (len == 0)
		return;

	if (ch == ' ') {
		printf("-\n");
		-- len;
	}

	while (len --> 0) {
		if (ch == ' ' && len == 0)
			printf("-\n");
		else
			printf("%c\n", ch);
	}
}

void drawHorizontal(char ch, size_t len) {
	if (len == 0)
		return;

	if (ch == ' ') {
		printf("|");
		-- len;
	}

	while (len --> 0) {
		if (ch == ' ' && len == 0)
			printf("|");
		else
			printf("%c", ch);
	}

	printf("\n");
}

int main(int argc, const char **argv) {
	Args args = argsNew(argc, argv);
	argv0     = argsShift(&args);

	flagBool("h", "help",    "Show the usage",   &fHelp);
	flagBool("v", "version", "Show the version", &fVersion);
	flagSize("l", "length",  "Line length",      &fLength);
	flagChar("c", "char",    "Line character",   &fChar);

	if (args.c > 0) {
		if (strcmp(args.v[0], "vertical") == 0) {
			argsShift(&args);
			parseOptions(&args);

			drawVertical(fChar, fLength);
			return 0;
		} else if (strcmp(args.v[0], "horizontal") == 0) {
			argsShift(&args);
			parseOptions(&args);

			drawHorizontal(fChar, fLength);
			return 0;
		} else
			parseOptions(&args);
	}

	usage(stderr);
	return EXIT_FAILURE;
}
