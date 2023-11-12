#ifdef __cplusplus
extern "C" {
#endif

#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/error.c"

#include "args.h"

NOCH_DEF bool argIsFlag(const char *arg) {
	nochAssert(arg != NULL);

	if (strlen(arg) > 1)
		return arg[0] == '-' && !argIsFlagsEnd(arg);
	else
		return false;
}

NOCH_DEF bool argIsLongFlag(const char *arg) {
	nochAssert(arg != NULL);

	if (strlen(arg) > 2)
		return arg[0] == '-' && arg[1] == '-';
	else
		return false;
}

NOCH_DEF bool argIsFlagsEnd(const char *arg) {
	nochAssert(arg != NULL);

	return strcmp(arg, "--") == 0;
}

typedef enum {
	FLAG_STRING = 0,
	FLAG_CHAR,
	FLAG_INT,
	FLAG_SIZE,
	FLAG_NUM,
	FLAG_BOOL,

	FLAG_TYPE_COUNT,
} FlagType;

typedef struct {
	FlagType type;

	union {
		const char **str;
		char        *ch;
		int         *intx;
		size_t      *size;
		double      *num;
		bool        *boolx;
	} var;

	union {
		const char *str;
		char        ch;
		int         intx;
		size_t      size;
		double      num;
		bool        boolx;
	} defaultVal;

	const char *shortName, *longName, *desc;
} Flag;

Flag   flags[FLAGS_CAPACITY];
size_t flagsCount = 0;

static Flag *getFlagByShortName(const char *shortName) {
	if (shortName == NULL)
		return NULL;

	for (size_t i = 0; i < flagsCount; ++ i) {
		if (flags[i].shortName == NULL)
			continue;

		if (strcmp(flags[i].shortName, shortName) == 0)
			return &flags[i];
	}

	return NULL;
}

static Flag *getFlagByLongName(const char *longName) {
	if (longName == NULL)
		return NULL;

	for (size_t i = 0; i < flagsCount; ++ i) {
		if (flags[i].longName == NULL)
			continue;

		if (strcmp(flags[i].longName, longName) == 0)
			return &flags[i];
	}

	return NULL;
}

static int argToChar(const char *arg, char *var) {
	nochAssert(arg != NULL);
	nochAssert(var != NULL);

	if (strlen(arg) != 1)
		return -1;

	*var = arg[0];
	return 0;
}

static int argToInt(const char *arg, int *var) {
	nochAssert(arg != NULL);
	nochAssert(var != NULL);

	char *ptr;
	*var = (int)strtol(arg, &ptr, 10);
	return *ptr == '\0'? 0 : -1;
}

static int argToSize(const char *arg, size_t *var) {
	nochAssert(arg != NULL);
	nochAssert(var != NULL);

	char *ptr;
	*var = (size_t)strtoull(arg, &ptr, 10);
	return *ptr == '\0'? 0 : -1;
}

static int argToNum(const char *arg, double *var) {
	nochAssert(arg != NULL);
	nochAssert(var != NULL);

	char *ptr;
	*var = (double)strtod(arg, &ptr);
	return *ptr == '\0'? 0 : -1;
}

/* Case insensitive equality function */
static bool argEquals(const char *a, const char *b) {
	size_t len = strlen(a);
	if (len != strlen(b))
		return -1;

	for (size_t i = 0; i < len; ++ i) {
		if (tolower(a[i]) != tolower(b[i]))
			return -1;
	}

	return 0;
}

#define ARG_EQUALS_4(STR, A, B, C, D) \
	(argEquals(STR, A) ||             \
	 argEquals(STR, B) ||             \
	 argEquals(STR, C) ||             \
	 argEquals(STR, D))

static int argToBool(const char *arg, bool *var) {
	nochAssert(arg != NULL);
	nochAssert(var != NULL);

	if (ARG_EQUALS_4(arg, "true",  "1", "yes", "y"))
		*var = true;
	else if (ARG_EQUALS_4(arg, "false", "0", "no",  "n"))
		*var = false;
	else
		return -1;

	return 0;
}

#undef ARG_EQUALS_4

/* Set the flags value from an arg. Type is automatically assumed from the flag type */
static int setFlagFromArg(Flag *flag, const char *arg, const char *orig) {
	nochAssert(flag != NULL);
	nochAssert(arg  != NULL);

#define FLAG_SET(FIELD, FUNC, ...)           \
	do {                                     \
		if (FUNC(arg, flag->var.FIELD) != 0) \
			return nochError(__VA_ARGS__);   \
	} while (0)

	nochAssert(flag->type < FLAG_TYPE_COUNT);

	switch (flag->type) {
	case FLAG_STRING: *flag->var.str = arg; break;
	case FLAG_CHAR: FLAG_SET(ch,    argToChar, "\"%s\": Expected a character", orig); break;
	case FLAG_INT:  FLAG_SET(intx,  argToInt,  "\"%s\": Expected an integer",  orig); break;
	case FLAG_SIZE: FLAG_SET(size,  argToSize, "\"%s\": Expected a size",      orig); break;
	case FLAG_NUM:  FLAG_SET(num,   argToNum,  "\"%s\": Expected a number",    orig); break;
	case FLAG_BOOL: FLAG_SET(boolx, argToBool, "\"%s\": Expected a boolean",   orig); break;

	default: nochAssert(0 && "Unknown flag type");
	}

#undef FLAG_SET

	return 0;
}

#define IMPL_FLAG_FUNC(POSTFIX, TYPE, FLAG_TYPE, FIELD)                      \
	NOCH_DEF void flag##POSTFIX(const char *shortName, const char *longName, \
	                            const char *desc, TYPE *var) {               \
		nochAssert(var != NULL);                                             \
		nochAssert(flagsCount < FLAGS_CAPACITY);                             \
		if (shortName != NULL)                                               \
			nochAssert(strlen(shortName) <= MAX_FLAG_NAME_LEN);              \
		if (longName != NULL)                                                \
			nochAssert(strlen(longName)  <= MAX_FLAG_NAME_LEN);              \
		                                                                     \
		Flag *flag = &flags[flagsCount ++];                                  \
		flag->type             = FLAG_TYPE;                                  \
		flag->var.FIELD        = var;                                        \
		flag->defaultVal.FIELD = *(var);                                     \
		flag->shortName        = shortName;                                  \
		flag->longName         = longName;                                   \
		flag->desc             = desc;                                       \
	}

IMPL_FLAG_FUNC(String, const char*, FLAG_STRING, str)
IMPL_FLAG_FUNC(Char,   char,        FLAG_CHAR,   ch)
IMPL_FLAG_FUNC(Int,    int,         FLAG_INT,    intx)
IMPL_FLAG_FUNC(Size,   size_t,      FLAG_SIZE,   size)
IMPL_FLAG_FUNC(Num,    double,      FLAG_NUM,    num)
IMPL_FLAG_FUNC(Bool,   bool,        FLAG_BOOL,   boolx)

#undef IMPL_FLAG_FUNC

NOCH_DEF Args argsNew(int argc, const char **argv) {
	nochAssert(argv != NULL);

	Args a;
	a.c    = (size_t)argc;
	a.v    = argv;
	a.base = (char**)argv;
	return a;
}

NOCH_DEF const char *argsShift(Args *args) {
	nochAssert(args != NULL);

	if (args->c <= 0)
		return NULL;

	const char *arg = *args->v ++;
	-- args->c;
	return arg;
}

NOCH_DEF int argsParseFlags(Args *args, Args *stripped) {
	nochAssert(args != NULL);

	/* If stripped args are expected to be returned, allocate memory for them
	   (allocate the same size as the original arguments so we dont have to do any reallocs) */
	if (stripped != NULL) {
		stripped->base = (char**)nochAlloc((args->c + 1) * sizeof(char*));
		if (stripped->base == NULL)
			NOCH_OUT_OF_MEM();

		stripped->v = (const char**)stripped->base;
		stripped->c = 0;
	}

	bool flagsEnd = false;
	for (size_t i = 0; i < args->c; ++ i) {
		const char *arg = args->v[i], *orig = arg;
		if (argIsFlagsEnd(arg) && !flagsEnd) {
			/* If stripped args arent expected to be returned, we can just return from the
			   function after reaching the end of flag args */

			if (stripped == NULL) {
				if (i + 1 < args->c)
					return nochError("\"%s\": Unexpected argument", args->v[i + 1]);

				break;
			} else {
				flagsEnd = true;
				continue;
			}
		} else if (!argIsFlag(arg) || flagsEnd) {
			/* If stripped args are supposed to be returned, save each non-flag arg */

			if (stripped != NULL)
				stripped->v[stripped->c ++] = arg;
			else
				return nochError("\"%s\": Unexpected argument", orig);

			/* Dont parse non-flag args as flags */
			continue;
		}

		bool isLong = argIsLongFlag(arg);
		arg += isLong + 1;

		/* Find the end of the flag name */
		const char *tmp   = arg;
		size_t      count = 0;
		while (*arg != '\0' && *arg != '=') {
			++ count;
			++ arg;
		}
		if (count > MAX_FLAG_NAME_LEN)
			return nochError("\"%s\": Unknown flag", orig);

		/* Allocate memory for flag name and copy it there */
		char name[MAX_FLAG_NAME_LEN];
		strncpy(name, tmp, count);
		name[count] = '\0';

		/* Get the flag pointer */
		Flag *flag = isLong? getFlagByLongName(name) : getFlagByShortName(name);
		if (flag == NULL)
			return nochError("\"%s\": Unknown flag", orig);

		/* If the flag has '=', save the value */
		if (*arg == '=') {
			++ arg;

			if (setFlagFromArg(flag, arg, orig) != 0)
				return -1;
		} else if (flag->type == FLAG_BOOL)
			/* If there was no value in the flag, Set the flag to true if its a boolean flag */
			*flag->var.boolx = true;
		else {
			/* Otherwise, read the next argument for the value */
			++ i;
			if (i >= args->c) {
				if (flag->type == FLAG_STRING)
					/* If there is no value for the flag, Set the flag to
					   an empty string if its a string flag */
					*flag->var.str = "";
				else
					return nochError("\"%s\": Missing value", orig);
			} else
				if (setFlagFromArg(flag, args->v[i], orig) != 0)
					return -1;
		}
	}

	if (stripped != NULL)
		stripped->v[stripped->c] = NULL;

	return 0;
}

NOCH_DEF void flagsUsage(FILE *file) {
	nochAssert(file != NULL);

	if (flagsCount == 0)
		return;

	/* Find the offset of the flag descriptions so all the descriptions are aligned, like so:
		  -h, --help       Show the usage
		  -v, --version    Show the version
		  -r               Foo bar baz
		  -f               Whatever description
	*/

#define FMT_SHORTNAME "  -%s", flag->shortName
#define FMT_LONGNAME  "  --%s", flag->longName
#define FMT_BOTH      "  -%s, --%s", flag->shortName, flag->longName

	int longest = 0;
	for (size_t i = 0; i < flagsCount; ++ i) {
		Flag *flag = &flags[i];

		int len;
		if (flag->shortName == NULL)
			len = snprintf(NULL, 0, FMT_LONGNAME);
		else if (flag->longName == NULL)
			len = snprintf(NULL, 0, FMT_SHORTNAME);
		else
			len = snprintf(NULL, 0, FMT_BOTH);

		if (len > longest)
			longest = len;
	}

	/* Print all flags and align descriptions */
	for (size_t i = 0; i < flagsCount; ++ i) {
		Flag *flag = &flags[i];

		int len;
		if (flag->shortName == NULL)
			len = fprintf(file, FMT_LONGNAME);
		else if (flag->longName == NULL)
			len = fprintf(file, FMT_SHORTNAME);
		else
			len = fprintf(file, FMT_BOTH);

		for (int i = len; i < longest; ++ i)
			fputc(' ', file);

		fprintf(file, "    %s", flag->desc);

		/* If the default value is a false bool or a NULL string, dont print it */
		if ((flag->type == FLAG_BOOL   && !flag->defaultVal.boolx) ||
		    (flag->type == FLAG_STRING && flag->defaultVal.str == NULL)) {
			fprintf(file, "\n");
			continue;
		}

		nochAssert(flag->type < FLAG_TYPE_COUNT);

		fprintf(file, " (default \"");
		switch (flag->type) {
		case FLAG_STRING:fprintf(file, "%s",  flag->defaultVal.str);                 break;
		case FLAG_CHAR:  fprintf(file, "%c",  flag->defaultVal.ch);                  break;
		case FLAG_INT:   fprintf(file, "%i",  flag->defaultVal.intx);                break;
		case FLAG_SIZE:  fprintf(file, "%lu", (long unsigned)flag->defaultVal.size); break;
		case FLAG_NUM:   fprintf(file, "%f",  flag->defaultVal.num);                 break;
		case FLAG_BOOL:  fprintf(file, "true");                                      break;

		default: nochAssert(0 && "Unknown flag type");
		}
		fprintf(file, "\")\n");
	}

#undef FMT_SHORTNAME
#undef FMT_LONGNAME
#undef FMT_BOTH

}

NOCH_DEF void argsUsage(FILE *file, const char *name, const char **usages,
                        size_t usagesCount, const char *desc, bool printFlags) {
	nochAssert(file != NULL);

	if (usages != NULL && usagesCount > 0) {
		nochAssert(name != NULL);
		for (size_t i = 0; i < usagesCount; ++ i)
			fprintf(file, i == 0? "Usage: %s %s\n" : "       %s %s\n", name, usages[i]);

		if (desc != NULL)
			fprintf(file, "\n");
	}

	if (desc != NULL) {
		fprintf(file, "%s\n", desc);

		if (flagsCount > 0 && printFlags)
			fprintf(file, "\n");
	}

	if (printFlags) {
		fprintf(file, "Options:\n");
		flagsUsage(file);
	}
}

#ifdef __cplusplus
}
#endif
