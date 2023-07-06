#include "args.h"

#ifdef __cplusplus
extern "C" {
#endif

NOCH_DEF bool noch_is_arg_flag(const char *arg) {
	NOCH_ASSERT(arg != NULL);

	if (strlen(arg) > 1)
		return arg[0] == '-' && !noch_is_arg_flags_end(arg);
	else
		return false;
}

NOCH_DEF bool noch_is_arg_long_flag(const char *arg) {
	NOCH_ASSERT(arg != NULL);

	if (strlen(arg) > 2)
		return arg[0] == '-' && arg[1] == '-';
	else
		return false;
}

NOCH_DEF bool noch_is_arg_flags_end(const char *arg) {
	NOCH_ASSERT(arg != NULL);

	return strcmp(arg, "--") == 0;
}

typedef enum {
	NOCH_FLAG_STR = 0,
	NOCH_FLAG_CHAR,
	NOCH_FLAG_INT,
	NOCH_FLAG_SIZE,
	NOCH_FLAG_NUM,
	NOCH_FLAG_BOOL,

	NOCH_FLAG_TYPE_COUNT,
} noch__flag_type_t;

typedef struct {
	noch__flag_type_t type;

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
} noch__flag_t;

#ifndef NOCH_FLAGS_CAPACITY
#	define NOCH_FLAGS_CAPACITY 128
#endif

noch__flag_t noch__flags[NOCH_FLAGS_CAPACITY];
size_t       noch__flags_count = 0;

static noch__flag_t *noch__get_flag_by_short_name(const char *short_name) {
	if (short_name == NULL)
		return NULL;

	for (size_t i = 0; i < noch__flags_count; ++ i) {
		if (noch__flags[i].short_name == NULL)
			continue;

		if (strcmp(noch__flags[i].short_name, short_name) == 0)
			return &noch__flags[i];
	}

	return NULL;
}

static noch__flag_t *noch__get_flag_by_long_name(const char *long_name) {
	if (long_name == NULL)
		return NULL;

	for (size_t i = 0; i < noch__flags_count; ++ i) {
		if (noch__flags[i].long_name == NULL)
			continue;

		if (strcmp(noch__flags[i].long_name, long_name) == 0)
			return &noch__flags[i];
	}

	return NULL;
}

static bool noch__arg_to_char(const char *arg, char *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	if (strlen(arg) != 1)
		return false;

	*var = arg[0];
	return true;
}

static bool noch__arg_to_int(const char *arg, int *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	char *ptr;
	*var = (int)strtol(arg, &ptr, 10);
	return *ptr == '\0';
}

static bool noch__arg_to_size(const char *arg, size_t *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	char *ptr;
	*var = (size_t)strtoull(arg, &ptr, 10);
	return *ptr == '\0';
}

static bool noch__arg_to_num(const char *arg, double *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	char *ptr;
	*var = (double)strtod(arg, &ptr);
	return *ptr == '\0';
}

/* Case insensitive string equality function */
static bool noch__ci_str_equals(const char *a, const char *b) {
	size_t len = strlen(a);
	if (len != strlen(b))
		return false;

	for (size_t i = 0; i < len; ++ i) {
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	}

	return true;
}

#define NOCH__CI_STR_EQUALS_4(STR, A, B, C, D) \
	(noch__ci_str_equals(STR, A) ||            \
	 noch__ci_str_equals(STR, B) ||            \
	 noch__ci_str_equals(STR, C) ||            \
	 noch__ci_str_equals(STR, D))

static bool noch__arg_to_bool(const char *arg, bool *var) {
	NOCH_ASSERT(arg != NULL);
	NOCH_ASSERT(var != NULL);

	if (NOCH__CI_STR_EQUALS_4(arg, "true",  "1", "yes", "y"))
		*var = true;
	else if (NOCH__CI_STR_EQUALS_4(arg, "false", "0", "no",  "n"))
		*var = false;
	else
		return false;

	return true;
}

/* Set the flags value from an arg. Type is automatically assumed from the flag type */
static noch_args_err_t noch__set_flag_from_arg(noch__flag_t *flag, const char *arg) {
	NOCH_ASSERT(flag != NULL);
	NOCH_ASSERT(arg  != NULL);

#define NOCH__SET_FLAG(FIELD, FUNC, ERROR) \
	do {                                   \
		if (!FUNC(arg, flag->var.FIELD))   \
			return ERROR;                  \
	} while (0)

	NOCH_ASSERT(flag->type < NOCH_FLAG_TYPE_COUNT);

	switch (flag->type) {
	case NOCH_FLAG_STR:  *flag->var.str = arg; break;
	case NOCH_FLAG_CHAR: NOCH__SET_FLAG(ch,    noch__arg_to_char, NOCH_ARGS_EXPECTED_CHAR); break;
	case NOCH_FLAG_INT:  NOCH__SET_FLAG(int_,  noch__arg_to_int,  NOCH_ARGS_EXPECTED_INT);  break;
	case NOCH_FLAG_SIZE: NOCH__SET_FLAG(size,  noch__arg_to_size, NOCH_ARGS_EXPECTED_SIZE); break;
	case NOCH_FLAG_NUM:  NOCH__SET_FLAG(num,   noch__arg_to_num,  NOCH_ARGS_EXPECTED_NUM);  break;
	case NOCH_FLAG_BOOL: NOCH__SET_FLAG(bool_, noch__arg_to_bool, NOCH_ARGS_EXPECTED_BOOL); break;

	default: break;
	}

	return NOCH_ARGS_OK;
}

#define NOCH__FLAG_FUNC_IMPL(POSTFIX, TYPE, FLAG_TYPE, FIELD)                        \
	NOCH_DEF void noch_flag_##POSTFIX(const char *short_name, const char *long_name, \
	                                  const char *desc, TYPE *var) {                 \
		NOCH_ASSERT(var != NULL);                                                    \
		NOCH_ASSERT(noch__flags_count < NOCH_FLAGS_CAPACITY);                        \
		if (short_name != NULL)                                                      \
			NOCH_ASSERT(strlen(short_name) <= NOCH_MAX_FLAG_NAME_LEN);               \
		if (long_name != NULL)                                                       \
			NOCH_ASSERT(strlen(long_name)  <= NOCH_MAX_FLAG_NAME_LEN);               \
		                                                                             \
		noch__flag_t *flag = &noch__flags[noch__flags_count ++];                     \
		flag->type              = FLAG_TYPE;                                         \
		flag->var.FIELD         = var;                                               \
		flag->default_val.FIELD = *(var);                                            \
		flag->short_name        = short_name;                                        \
		flag->long_name         = long_name;                                         \
		flag->desc              = desc;                                              \
	}

NOCH__FLAG_FUNC_IMPL(str,  const char*, NOCH_FLAG_STR,  str)
NOCH__FLAG_FUNC_IMPL(char, char,        NOCH_FLAG_CHAR, ch)
NOCH__FLAG_FUNC_IMPL(int,  int,         NOCH_FLAG_INT,  int_)
NOCH__FLAG_FUNC_IMPL(size, size_t,      NOCH_FLAG_SIZE, size)
NOCH__FLAG_FUNC_IMPL(num,  double,      NOCH_FLAG_NUM,  num)
NOCH__FLAG_FUNC_IMPL(bool, bool,        NOCH_FLAG_BOOL, bool_)

static const char *noch__args_err_to_str_map[] = {
	"Ok",                  /* NOCH_ARGS_OK */
	"Unknown flag",        /* NOCH_ARGS_UNKNOWN_FLAG */
	"Missing argument",    /* NOCH_ARGS_MISSING */
	"Extra argument",      /* NOCH_ARGS_EXTRA */
	"Out of memory",       /* NOCH_ARGS_OUT_OF_MEM */
	"Expected a string",   /* NOCH_ARGS_EXPECTED_STR */
	"Expected a char",     /* NOCH_ARGS_EXPECTED_CHAR */
	"Expected an integer", /* NOCH_ARGS_EXPECTED_INT */
	"Expected a size",     /* NOCH_ARGS_EXPECTED_SIZE */
	"Expected a number",   /* NOCH_ARGS_EXPECTED_NUM */
	"Expected a bool",     /* NOCH_ARGS_EXPECTED_BOOL */
};

NOCH_DEF const char *noch_args_err_to_str(noch_args_err_t err) {
	/* NOCH_ARGS_OK is not an error */
	NOCH_ASSERT(err > NOCH_ARGS_OK && err < NOCH_ARGS_ERROR_COUNT);
	return noch__args_err_to_str_map[err];
}

NOCH_DEF noch_args_t noch_args_new(int argc, const char **argv) {
	NOCH_ASSERT(argv != NULL);

	noch_args_t a;
	a.c    = (size_t)argc;
	a.v    = argv;
	a.base = (char**)argv;
	return a;
}

NOCH_DEF const char *noch_args_shift(noch_args_t *args) {
	NOCH_ASSERT(args != NULL);

	if (args->c <= 0)
		return NULL;

	const char *arg = *args->v ++;
	-- args->c;
	return arg;
}

NOCH_DEF noch_args_err_t noch_args_parse_flags(noch_args_t *args, size_t *where, size_t *end,
                                               noch_args_t *stripped) {
	NOCH_ASSERT(args != NULL);

	/* If stripped args are expected to be returned, allocate memory for them
	   (allocate the same size as the original arguments so we dont have to do any reallocs) */
	if (stripped != NULL) {
		stripped->base = (char**)NOCH_ALLOC(sizeof(*stripped->v) * args->c);
		if (stripped->base == NULL)
			return NOCH_ARGS_OUT_OF_MEM;

		stripped->v = (const char**)stripped->base;
		stripped->c = 0;
	}

	bool   has_extra   = false;
	size_t extra_where = 0;

	bool flags_end = false;
	for (size_t i = 0; i < args->c; ++ i) {
		const char *arg = args->v[i];
		if (noch_is_arg_flags_end(arg) && !flags_end) {
			/* If stripped args arent expected to be returned, we can just return from the
			   function after reaching the end of flag args */
			if (end != NULL)
				*end = i;

			if (stripped == NULL) {
				if (i + 1 < args->c) {
					has_extra   = true;
					extra_where = i + 1;
				}
				goto end;
			} else {
				flags_end = true;
				continue;
			}
		} else if (!noch_is_arg_flag(arg) || flags_end) {
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

		if (end != NULL)
			*end = i;

		bool is_long = noch_is_arg_long_flag(arg);
		arg += is_long + 1;

		/* Find the end of the flag name */
		const char *tmp   = arg;
		size_t      count = 0;
		while (*arg != '\0' && *arg != '=') {
			++ count;
			++ arg;
		}
		if (count > NOCH_MAX_FLAG_NAME_LEN)
			return NOCH_ARGS_UNKNOWN_FLAG;

		/* Allocate memory for flag name and copy it there */
		char name[NOCH_MAX_FLAG_NAME_LEN];
		strncpy(name, tmp, count);
		name[count] = '\0';

		/* Get the flag pointer */
		noch__flag_t *flag = is_long? noch__get_flag_by_long_name(name) :
		                     noch__get_flag_by_short_name(name);
		if (flag == NULL)
			return NOCH_ARGS_UNKNOWN_FLAG;

		/* If the flag has '=', save the value */
		if (*arg == '=') {
			++ arg;

			int err = noch__set_flag_from_arg(flag, arg);
			if (err != NOCH_ARGS_OK)
				return err;
		} else if (flag->type == NOCH_FLAG_BOOL)
			/* If there was no value in the flag, Set the flag to true if its a boolean flag */
			*flag->var.bool_ = true;
		else {
			/* Otherwise, read the next argument for the value */
			++ i;
			if (i >= args->c)
				return NOCH_ARGS_MISSING;

			if (end != NULL)
				++ *end;

			int err = noch__set_flag_from_arg(flag, (char*)args->v[i]);
			if (err != NOCH_ARGS_OK)
				return err;
		}
	}

end:
	if (has_extra) {
		if (where != NULL)
			*where = extra_where;

		return NOCH_ARGS_EXTRA;
	} else
		return NOCH_ARGS_OK;
}

NOCH_DEF void noch_fprint_flags_usage(FILE *file) {
	NOCH_ASSERT(file != NULL);

	if (noch__flags_count == 0)
		return;

	/* Find the offset of the flag descriptions so all the descriptions are aligned, like so:
		  -h, --help       Show the usage
		  -v, --version    Show the version
		  -r               Foo bar baz
		  -f               Whatever description
	*/
	int longest = 0;
	for (size_t i = 0; i < noch__flags_count; ++ i) {
		noch__flag_t *flag = &noch__flags[i];

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
	for (size_t i = 0; i < noch__flags_count; ++ i) {
		noch__flag_t *flag = &noch__flags[i];

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
		if ((flag->type == NOCH_FLAG_BOOL && !flag->default_val.bool_) ||
		    (flag->type == NOCH_FLAG_STR  && flag->default_val.str == NULL)) {
			fprintf(file, "\n");
			continue;
		}

		NOCH_ASSERT(flag->type < NOCH_FLAG_TYPE_COUNT);

		fprintf(file, " (default \"");
		switch (flag->type) {
		case NOCH_FLAG_STR:  fprintf(file, "%s",  flag->default_val.str);  break;
		case NOCH_FLAG_CHAR: fprintf(file, "%c",  flag->default_val.ch);   break;
		case NOCH_FLAG_INT:  fprintf(file, "%i",  flag->default_val.int_); break;
		case NOCH_FLAG_SIZE: fprintf(file, "%zu", flag->default_val.size); break;
		case NOCH_FLAG_NUM:  fprintf(file, "%f",  flag->default_val.num);  break;
		case NOCH_FLAG_BOOL: fprintf(file, "true");                        break;

		default: break;
		}
		fprintf(file, "\")\n");
	}
}

NOCH_DEF void noch_fprint_usage(FILE *file, const char *name, const char **usages,
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

		if (noch__flags_count > 0 && print_flags)
			fprintf(file, "\n");
	}

	if (print_flags) {
		fprintf(file, "Options:\n");
		noch_fprint_flags_usage(file);
	}
}

#ifdef __cplusplus
}
#endif
