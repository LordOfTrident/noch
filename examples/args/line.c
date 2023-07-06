#include <stdio.h>  /* printf, fprintf, FILE, stderr */
#include <stdlib.h> /* exit, EXIT_FAILURE */
#include <string.h> /* strcmp */
#include <assert.h> /* assert */

#include <args.h> /* Declarations */
#include <args.c> /* Implementation */

#define DESC "Draw a triangle or a rectangle in the terminal"
const char *usages[] = {
	"[OPTIONS]",
	"vertical [OPTIONS]      Draw a vertical line",
	"horizontal [OPTIONS]    Draw a horizontal line",
};

const char *app_path;

bool   flag_h = false, flag_v = false;
size_t flag_l = 6;
char   flag_c = '#';

void fprint_usage(FILE *file) {
	noch_fprint_usage(file, app_path, usages, sizeof(usages) / sizeof(*usages), DESC, true);
}

void fprint_version(FILE *file) {
	fprintf(file, "Version x.y.z\n");
}

void parse_options(noch_args_t *args) {
	size_t          where;
	noch_args_err_t err = noch_args_parse_flags(args, &where, NULL, NULL);
	if (err != NOCH_ARGS_OK) {
		assert(err != NOCH_ARGS_OUT_OF_MEM);

		fprintf(stderr, "Error: '%s': %s\n", args->v[where], noch_args_err_to_str(err));
		exit(EXIT_FAILURE);
	}

	if (flag_h) {
		fprint_usage(stdout);
		exit(0);
	} else if (flag_v) {
		fprint_version(stdout);
		exit(0);
	}
}

void draw_vertical(char ch, size_t len) {
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

void draw_horizontal(char ch, size_t len) {
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
	noch_args_t args = noch_args_new(argc, argv);
	app_path         = noch_args_shift(&args);

	noch_flag_bool("h", "help",    "Show the usage",   &flag_h);
	noch_flag_bool("v", "version", "Show the version", &flag_v);
	noch_flag_size("l", "length",  "Line length",      &flag_l);
	noch_flag_char("c", "char",    "Line character",   &flag_c);

	if (args.c > 0) {
		if (strcmp(args.v[0], "vertical") == 0) {
			noch_args_shift(&args);
			parse_options(&args);

			draw_vertical(flag_c, flag_l);
			return 0;
		} else if (strcmp(args.v[0], "horizontal") == 0) {
			noch_args_shift(&args);
			parse_options(&args);

			draw_horizontal(flag_c, flag_l);
			return 0;
		} else
			parse_options(&args);
	}

	fprint_usage(stderr);
	return EXIT_FAILURE;
}
