#ifdef __cplusplus
extern "C" {
#endif

#include "log.h"

static FILE *i__log_file    = NULL;
static int   i__log_flags   = LOG_BASIC;
static bool  i__log_is_init = false;

#ifdef PLATFORM_WINDOWS

static WORD i__log_color_to_win_attr[] = {
	0,
	/* LOG_RED     */ FOREGROUND_RED | FOREGROUND_INTENSITY,
	/* LOG_GREEN   */ FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_BLUE    */ FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

static HANDLE i__log_win_stdout_handle;
static HANDLE i__log_win_stderr_handle;

static CONSOLE_SCREEN_BUFFER_INFO i__log_orig_csbi;

static HANDLE i__log_file_to_win_handle(void) {
	if (i__log_file == stdout)
		return i__log_win_stdout_handle;
	else if (i__log_file == stderr)
		return i__log_win_stderr_handle;
	else
		return INVALID_HANDLE_VALUE;
}

#else

static const char *i__log_color_to_esc_seq[] = {
	0,
	/* LOG_RED     */ "\x1b[1;31m",
	/* LOG_GREEN   */ "\x1b[1;32m",
	/* LOG_YELLOW  */ "\x1b[1;33m",
	/* LOG_BLUE    */ "\x1b[1;34m",
	/* LOG_MAGENTA */ "\x1b[1;35m",
	/* LOG_CYAN    */ "\x1b[1;36m",
	/* LOG_WHITE   */ "\x1b[1;37m",
};

#endif

static bool i__log_file_can_be_colored(void) {
	return i__log_file == stdout || i__log_file == stderr;
}

static void i__init_log(void) {
	i__log_is_init = true;

#ifdef PLATFORM_WINDOWS
	i__log_win_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	i__log_win_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

	GetConsoleScreenBufferInfo(i__log_win_stdout_handle, &i__log_orig_csbi);
#endif

	if (i__log_file == NULL)
		i__log_file = stderr;
}

NOCH_DEF void set_log_file(FILE *file) {
	i__log_file = file;
}

NOCH_DEF void set_log_flags(int flags) {
	i__log_flags = flags;
}

static void i__log_reset_color(void) {
	if (i__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(i__log_file_to_win_handle(), i__log_orig_csbi.wAttributes);
#else
		fputs("\x1b[0m", i__log_file);
#endif
}

NOCH_DEF void log_generic(int color, const char *title, const char *path,
                          size_t line, const char *fmt, ...) {
	if (!i__log_is_init)
		i__init_log();

	i__log_reset_color();

#ifdef PLATFORM_WINDOWS
	HANDLE handle = i__log_file_to_win_handle();
#endif

	if (i__log_flags & LOG_TIME_DATE) {
		if (i__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY);
#else
			fputs("\x1b[0;1;90m", i__log_file);
#endif

		time_t timer;
		struct tm* info;
		timer = time(NULL);
		info  = localtime(&timer);

		if (i__log_flags & LOG_DATE) {
			char buf[16];
			strftime(buf, 16, "%Y-%m-%d ", info);
			fputs(buf, i__log_file);
		}

		if (i__log_flags & LOG_TIME)
			fprintf(i__log_file, "%02d:%02d:%02d ", info->tm_hour, info->tm_min, info->tm_sec);
	}

	if (i__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(handle, i__log_color_to_win_attr[color]);
#else
		fputs(i__log_color_to_esc_seq[color], i__log_file);
#endif

	fprintf(i__log_file, "[%s]", title);

	if (i__log_flags & LOG_LOCATION) {
		if (i__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_RED  | FOREGROUND_GREEN |
			                                FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#else
			fputs("\x1b[0;1;97m", i__log_file);
#endif

		fputc(' ', i__log_file);

		if (i__log_flags & LOG_FILE)
			fprintf(i__log_file, "%s:", path);
		if (i__log_flags & LOG_LINE)
			fprintf(i__log_file, "%lu:", (long unsigned)line);
	}

	char    str[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	i__log_reset_color();
	fprintf(i__log_file, " %s\n", str);
}

#ifdef __cplusplus
}
#endif
