#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"
#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "args.h"

#define flag_type_t            NOCH_PRIVATE(flag_type_t)
#define flag_t                 NOCH_PRIVATE(flag_t)
#define flags                  NOCH_PRIVATE(flags)
#define flags_count            NOCH_PRIVATE(flags_count)
#define flag_get_by_short_name NOCH_PRIVATE(flag_get_by_short_name)
#define flag_get_by_long_name  NOCH_PRIVATE(flag_get_by_long_name)
#define arg_to_char            NOCH_PRIVATE(arg_to_char)
#define arg_to_int             NOCH_PRIVATE(arg_to_int)
#define arg_to_size            NOCH_PRIVATE(arg_to_size)
#define arg_to_num             NOCH_PRIVATE(arg_to_num)
#define arg_equals_ci          NOCH_PRIVATE(arg_equals_ci)
#define arg_to_bool            NOCH_PRIVATE(arg_to_bool)
#define flag_set_from_arg      NOCH_PRIVATE(flag_set_from_arg)

NOCH_DEF bool arg_is_flag(const char *arg) {
	NOCH_ASSERT(arg != NULL);

	if (strlen(arg) > 1)
		return arg[0] == '-' && !arg_is_flags_end(arg);
	else
		return false;
}

NOCH_DEF bool arg_is_long_flag(const char *arg) {
	NOCH_ASSERT(arg != NULL);

	if (strlen(arg) > 2)
		return arg[0] == '-' && arg[1] == '-';
	else
		return false;
}

NOCH_DEF bool arg_is_flags_end(const char *arg) {
	NOCH_ASSERT(arg != NULL);

	return strcmp(arg, "--") == 0;
}

typedef enum {
	FLAG_STR = 0,
	FLAG_CHAR,
	FLAG_INT,
	FLAG_SIZE,
	FLAG_NUM,
	FLAG_BOOL,

	FLAG_TYPE_COUNT,
} flag_type_t;

typedef struct {
	flag_type_t type;

	union {
		const char **str;
		char        *ch;
		int         *int_;
		size_t      *size;
		double      *num;
		bool        *bool_;
	} var;

	union {
		const char *str;
		char        ch;
		int         int_;
		size_t      size;
		double      num;
		bool        bool_;
	} default_val;

	const char *short_name, *long_name, *desc;
} flag_t;

flag_t flags[FLAGS_CAPACITY];
size_t flags_count = 0;

static flag_t *flag_get_by_short_name(const char *short_name) {
	if (short_name == NULL)
		return NULL;

	for (size_t i = 0; i < flags_count; ++ i) {
		if (flags[i].short_name == NULL)
			continue;

		if (strcmp(flags[i].short_name, short_name) == 0)
			return &flags[i];
	}

	return NULL;
}

static flag_t *flag_get_by_long_name(const char *long_name) {
	if (long_name == NULL)
		return NULL;

	for (size_t i = 0; i < flags_count; ++ i) {
		if (flags[i].long_name == NULL)
			continue;

		if (strcmp(flags[i].long_name, long_name) == 0)
			return &flags[i];
	}

	return NULL;
}

static int arg_to_char(const char *arg, char *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	if (strlen(arg) != 1)
		return -1;

	*var = arg[0];
	return 0;
}

static int arg_to_int(const char *arg, int *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	char *ptr;
	*var = (int)strtol(arg, &ptr, 10);
	return *ptr == '\0'? 0 : -1;
}

static int arg_to_size(const char *arg, size_t *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	char *ptr;
	*var = (size_t)strtoull(arg, &ptr, 10);
	return *ptr == '\0'? 0 : -1;
}

static int arg_to_num(const char *arg, double *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	char *ptr;
	*var = (double)strtod(arg, &ptr);
	return *ptr == '\0'? 0 : -1;
}

/* Case insensitive equality function */
static bool arg_equals_ci(const char *a, const char *b) {
	size_t len = strlen(a);
	if (len != strlen(b))
		return -1;

	for (size_t i = 0; i < len; ++ i) {
		if (tolower(a[i]) != tolower(b[i]))
			return -1;
	}

	return 0;
}

#define ARG_EQUALS_CI_4(STR, A, B, C, D) \
	(arg_equals_ci(STR, A) ||            \
	 arg_equals_ci(STR, B) ||            \
	 arg_equals_ci(STR, C) ||            \
	 arg_equals_ci(STR, D))

static int arg_to_bool(const char *arg, bool *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	if (ARG_EQUALS_CI_4(arg, "true",  "1", "yes", "y"))
		*var = true;
	else if (ARG_EQUALS_CI_4(arg, "false", "0", "no",  "n"))
		*var = false;
	else
		return -1;

	return 0;
}

#undef ARG_EQUALS_CI_4

/* Set the flags value from an arg. Type is automatically assumed from the flag type */
static int flag_set_from_arg(flag_t *flag, const char *arg) {
	NOCH_ASSERT(flag != NULL);
	NOCH_ASSERT(arg  != NULL);

#define FLAG_SET(FIELD, FUNC, ERR)           \
	do {                                     \
		if (FUNC(arg, flag->var.FIELD) != 0) \
			return NOCH_PARSER_ERR(ERR);     \
	} while (0)

	NOCH_ASSERT(flag->type < FLAG_TYPE_COUNT);

	switch (flag->type) {
	case FLAG_STR:  *flag->var.str = arg; break;
	case FLAG_CHAR: FLAG_SET(ch,    arg_to_char, "Expected a character"); break;
	case FLAG_INT:  FLAG_SET(int_,  arg_to_int,  "Expected an integer");  break;
	case FLAG_SIZE: FLAG_SET(size,  arg_to_size, "Expected a size");      break;
	case FLAG_NUM:  FLAG_SET(num,   arg_to_num,  "Expected a number");    break;
	case FLAG_BOOL: FLAG_SET(bool_, arg_to_bool, "Expected a boolean");   break;

	default: NOCH_ASSERT(0 && "Unknown flag type");
	}

#undef FLAG_SET

	return 0;
}

#define IMPL_FLAG_FUNC(POSTFIX, TYPE, FLAG_TYPE, FIELD)                         \
	NOCH_DEF void flag_##POSTFIX(const char *short_name, const char *long_name, \
	                             const char *desc, TYPE *var) {                 \
		NOCH_ASSERT(var != NULL);                                               \
		NOCH_ASSERT(flags_count < FLAGS_CAPACITY);                              \
		if (short_name != NULL)                                                 \
			NOCH_ASSERT(strlen(short_name) <= MAX_FLAG_NAME_LEN);               \
		if (long_name != NULL)                                                  \
			NOCH_ASSERT(strlen(long_name)  <= MAX_FLAG_NAME_LEN);               \
		                                                                        \
		flag_t *flag = &flags[flags_count ++];                                  \
		flag->type              = FLAG_TYPE;                                    \
		flag->var.FIELD         = var;                                          \
		flag->default_val.FIELD = *(var);                                       \
		flag->short_name        = short_name;                                   \
		flag->long_name         = long_name;                                    \
		flag->desc              = desc;                                         \
	}

IMPL_FLAG_FUNC(str,  const char*, FLAG_STR,  str)
IMPL_FLAG_FUNC(char, char,        FLAG_CHAR, ch)
IMPL_FLAG_FUNC(int,  int,         FLAG_INT,  int_)
IMPL_FLAG_FUNC(size, size_t,      FLAG_SIZE, size)
IMPL_FLAG_FUNC(num,  double,      FLAG_NUM,  num)
IMPL_FLAG_FUNC(bool, bool,        FLAG_BOOL, bool_)

#undef IMPL_FLAG_FUNC

NOCH_DEF args_t args_new(int argc, const char **argv) {
	NOCH_ASSERT(argv != NULL);

	args_t a;
	a.c    = (size_t)argc;
	a.v    = argv;
	a.base = (char**)argv;
	return a;
}

NOCH_DEF const char *args_shift(args_t *args) {
	NOCH_ASSERT(args != NULL);

	if (args->c <= 0)
		return NULL;

	const char *arg = *args->v ++;
	-- args->c;
	return arg;
}

NOCH_DEF int args_parse_flags(args_t *args, size_t *where, args_t *stripped, bool *extra) {
	NOCH_ASSERT(args != NULL);

	/* If stripped args are expected to be returned, allocate memory for them
	   (allocate the same size as the original arguments so we dont have to do any reallocs) */
	if (stripped != NULL) {
		stripped->base = (char**)NOCH_ALLOC(sizeof(*stripped->v) * (args->c + 1));
		NOCH_CHECK_ALLOC(stripped->base);

		stripped->v = (const char**)stripped->base;
		stripped->c = 0;
	}

	bool   has_extra   = false;
	size_t extra_where = 0;

	if (extra != NULL)
		*extra = false;

	bool flags_end = false;
	for (size_t i = 0; i < args->c; ++ i) {
		const char *arg = args->v[i];
		if (arg_is_flags_end(arg) && !flags_end) {
			/* If stripped args arent expected to be returned, we can just return from the
			   function after reaching the end of flag args */

			if (stripped == NULL) {
				if (i + 1 < args->c) {
					has_extra   = true;
					extra_where = i + 1;
				}
				break;
			} else {
				flags_end = true;
				continue;
			}
		} else if (!arg_is_flag(arg) || flags_end) {
			/* If stripped args are supposed to be returned, save each non-flag arg */
			if (!has_extra) {
				has_extra   = true;
				extra_where = i;
			}

			if (stripped != NULL)
				stripped->v[stripped->c ++] = arg;

			/* Dont parse non-flag args as flags */
			continue;
		}

		if (where != NULL)
			*where = i;

		bool is_long = arg_is_long_flag(arg);
		arg += is_long + 1;

		/* Find the end of the flag name */
		const char *tmp   = arg;
		size_t      count = 0;
		while (*arg != '\0' && *arg != '=') {
			++ count;
			++ arg;
		}
		if (count > MAX_FLAG_NAME_LEN)
			return NOCH_PARSER_ERR("Unknown flag");

		/* Allocate memory for flag name and copy it there */
		char name[MAX_FLAG_NAME_LEN];
		strncpy(name, tmp, count);
		name[count] = '\0';

		/* Get the flag pointer */
		flag_t *flag = is_long? flag_get_by_long_name(name) : flag_get_by_short_name(name);
		if (flag == NULL)
			return NOCH_PARSER_ERR("Unknown flag");

		/* If the flag has '=', save the value */
		if (*arg == '=') {
			++ arg;

			if (flag_set_from_arg(flag, arg) != 0)
				return -1;
		} else if (flag->type == FLAG_BOOL)
			/* If there was no value in the flag, Set the flag to true if its a boolean flag */
			*flag->var.bool_ = true;
		else {
			/* Otherwise, read the next argument for the value */
			++ i;
			if (i >= args->c) {
				if (flag->type == FLAG_STR)
					/* If there is no value for the flag, Set the flag to
					   an empty string if its a string flag */
					*flag->var.str = "";
				else
					return NOCH_PARSER_ERR("Missing value");
			} else
				if (flag_set_from_arg(flag, (char*)args->v[i]) != 0)
					return -1;
		}
	}

	if (stripped != NULL)
		stripped->v[stripped->c] = NULL;

	if (extra != NULL && has_extra) {
		if (where != NULL)
			*where = extra_where;

		*extra = true;
		return 0;
	} else
		return 0;
}

NOCH_DEF void flags_usage_fprint(FILE *file) {
	NOCH_ASSERT(file != NULL);

	if (flags_count == 0)
		return;

	/* Find the offset of the flag descriptions so all the descriptions are aligned, like so:
		  -h, --help       Show the usage
		  -v, --version    Show the version
		  -r               Foo bar baz
		  -f               Whatever description
	*/
	int longest = 0;
	for (size_t i = 0; i < flags_count; ++ i) {
		flag_t *flag = &flags[i];

		int len;
		if (flag->short_name == NULL)
			len = snprintf(NULL, 0, "  --%s", flag->long_name);
		else if (flag->long_name == NULL)
			len = snprintf(NULL, 0, "  -%s", flag->short_name);
		else
			len = snprintf(NULL, 0, "  -%s, --%s", flag->short_name, flag->long_name);

		if (len > longest)
			longest = len;
	}

	/* Print all flags and align descriptions */
	for (size_t i = 0; i < flags_count; ++ i) {
		flag_t *flag = &flags[i];

		int len;
		if (flag->short_name == NULL)
			len = fprintf(file, "  --%s", flag->long_name);
		else if (flag->long_name == NULL)
			len = fprintf(file, "  -%s", flag->short_name);
		else
			len = fprintf(file, "  -%s, --%s", flag->short_name, flag->long_name);

		for (int i = len; i < longest; ++ i)
			fputc(' ', file);

		fprintf(file, "    %s", flag->desc);

		/* If the default value is a false bool or a NULL string, dont print it */
		if ((flag->type == FLAG_BOOL && !flag->default_val.bool_) ||
		    (flag->type == FLAG_STR  && flag->default_val.str == NULL)) {
			fprintf(file, "\n");
			continue;
		}

		NOCH_ASSERT(flag->type < FLAG_TYPE_COUNT);

		fprintf(file, " (default \"");
		switch (flag->type) {
		case FLAG_STR:  fprintf(file, "%s",  flag->default_val.str);                 break;
		case FLAG_CHAR: fprintf(file, "%c",  flag->default_val.ch);                  break;
		case FLAG_INT:  fprintf(file, "%i",  flag->default_val.int_);                break;
		case FLAG_SIZE: fprintf(file, "%lu", (long unsigned)flag->default_val.size); break;
		case FLAG_NUM:  fprintf(file, "%f",  flag->default_val.num);                 break;
		case FLAG_BOOL: fprintf(file, "true");                                       break;

		default: NOCH_ASSERT(0 && "Unknown flag type");
		}
		fprintf(file, "\")\n");
	}
}

NOCH_DEF void args_usage_fprint(FILE *file, const char *name, const char **usages,
                                size_t usages_count, const char *desc, bool print_flags) {
	NOCH_ASSERT(file != NULL);

	if (usages != NULL && usages_count > 0) {
		NOCH_ASSERT(name != NULL);
		for (size_t i = 0; i < usages_count; ++ i)
			fprintf(file, i == 0? "Usage: %s %s\n" : "       %s %s\n", name, usages[i]);

		if (desc != NULL)
			fprintf(file, "\n");
	}

	if (desc != NULL) {
		fprintf(file, "%s\n", desc);

		if (flags_count > 0 && print_flags)
			fprintf(file, "\n");
	}

	if (print_flags) {
		fprintf(file, "Options:\n");
		flags_usage_fprint(file);
	}
}

#undef flag_type_t
#undef flag_t
#undef flags
#undef flags_count
#undef flag_get_by_short_name
#undef flag_get_by_long_name
#undef arg_to_char
#undef arg_to_int
#undef arg_to_size
#undef arg_to_num
#undef arg_equals_ci
#undef arg_to_bool
#undef flag_set_from_arg

#ifdef __cplusplus
}
#endif
