#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"

#include "log.h"

#define log_file           NOCH_PRIV(log_file)
#define log_flags          NOCH_PRIV(log_flags)
#define log_is_init        NOCH_PRIV(log_is_init)
#define log_color_to_attr  NOCH_PRIV(log_color_to_attr)
#define log_stdout_handle  NOCH_PRIV(log_stdout_handle)
#define log_stderr_handle  NOCH_PRIV(log_stderr_handle)
#define log_prev_csbi      NOCH_PRIV(log_prev_csbi)
#define log_file_to_handle NOCH_PRIV(log_file_to_handle)
#define log_color_to_ansi  NOCH_PRIV(log_color_to_ansi)
#define log_has_color      NOCH_PRIV(log_has_color)
#define init_log           NOCH_PRIV(init_log)
#define log_reset_color    NOCH_PRIV(log_reset_color)

static FILE *NOCH_PRIV(log_file)    = NULL;
static int   NOCH_PRIV(log_flags)   = LOG_BASIC;
static bool  NOCH_PRIV(log_is_init) = false;

#ifdef PLATFORM_WINDOWS

static WORD NOCH_PRIV(log_color_to_attr)[] = {
	0,
	/* LOG_RED     */ FOREGROUND_RED | FOREGROUND_INTENSITY,
	/* LOG_GREEN   */ FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_BLUE    */ FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

static HANDLE NOCH_PRIV(log_stdout_handle);
static HANDLE NOCH_PRIV(log_stderr_handle);

static CONSOLE_SCREEN_BUFFER_INFO NOCH_PRIV(log_prev_csbi);

static HANDLE NOCH_PRIV(log_file_to_handle)(void) {
	if (log_file == stdout)
		return log_stdout_handle;
	else if (log_file == stderr)
		return log_stderr_handle;
	else
		return INVALID_HANDLE_VALUE;
}

#else

static const char *NOCH_PRIV(log_color_to_ansi)[] = {
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

static bool NOCH_PRIV(log_has_color)(void) {
	return log_file == stdout || log_file == stderr;
}

static void NOCH_PRIV(init_log)(void) {
	log_is_init = true;

#ifdef PLATFORM_WINDOWS
	log_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	log_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

	GetConsoleScreenBufferInfo(log_stdout_handle, &log_prev_csbi);
#endif

	if (log_file == NULL)
		log_file = stderr;
}

NOCH_DEF void set_log_file(FILE *file) {
	log_file = file;
}

NOCH_DEF void set_log_flags(int flags) {
	log_flags = flags;
}

static void NOCH_PRIV(log_reset_color)(void) {
	if (log_has_color())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(log_file_to_handle(), log_prev_csbi.wAttributes);
#else
		fputs("\x1b[0m", log_file);
#endif
}

NOCH_DEF void log_generic(int color, const char *title, const char *path,
                          size_t line, const char *fmt, ...) {
	if (!log_is_init)
		init_log();

	log_reset_color();

#ifdef PLATFORM_WINDOWS
	HANDLE handle = log_file_to_handle();
#endif

	if (log_flags & LOG_TIME_DATE) {
		if (log_has_color())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY);
#else
			fputs("\x1b[0;1;90m", log_file);
#endif

		time_t timer;
		struct tm* info;
		timer = time(NULL);
		info  = localtime(&timer);

		if (log_flags & LOG_DATE) {
			char buf[16];
			strftime(buf, 16, "%Y-%m-%d ", info);
			fputs(buf, log_file);
		}

		if (log_flags & LOG_TIME)
			fprintf(log_file, "%02d:%02d:%02d ", info->tm_hour, info->tm_min, info->tm_sec);
	}

	if (log_has_color())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(handle, log_color_to_attr[color]);
#else
		fputs(log_color_to_ansi[color], log_file);
#endif

	fprintf(log_file, "[%s]", title);

	if (log_flags & LOG_LOCATION) {
		if (log_has_color())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_RED  | FOREGROUND_GREEN |
			                                FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#else
			fputs("\x1b[0;1;97m", log_file);
#endif

		fputc(' ', log_file);

		if (log_flags & LOG_FILE)
			fprintf(log_file, "%s:", path);
		if (log_flags & LOG_LINE)
			fprintf(log_file, "%lu:", (long unsigned)line);
	}

	char    str[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	log_reset_color();
	fprintf(log_file, " %s\n", str);
}

#undef log_file
#undef log_flags
#undef log_is_init
#undef log_color_to_attr
#undef log_stdout_handle
#undef log_stderr_handle
#undef log_prev_csbi
#undef log_file_to_handle
#undef log_color_to_ansi
#undef log_has_color
#undef init_log
#undef log_reset_color

#ifdef __cplusplus
}
#endif
