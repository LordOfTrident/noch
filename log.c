#ifdef __cplusplus
extern "C" {
#endif

#include "log.h"

static FILE *s__log_file    = NULL;
static int   s__log_flags   = LOG_BASIC;
static bool  s__log_is_init = false;

#ifdef PLATFORM_WINDOWS

static WORD s__log_color_to_win_attr[] = {
	0,
	/* LOG_RED     */ FOREGROUND_RED | FOREGROUND_INTENSITY,
	/* LOG_GREEN   */ FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_BLUE    */ FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

static HANDLE s__log_win_stdout_handle;
static HANDLE s__log_win_stderr_handle;

static CONSOLE_SCREEN_BUFFER_INFO s__log_orig_csbi;

static HANDLE s__log_file_to_win_handle(void) {
	if (s__log_file == stdout)
		return s__log_win_stdout_handle;
	else if (s__log_file == stderr)
		return s__log_win_stderr_handle;
	else
		return INVALID_HANDLE_VALUE;
}

#else

static const char *s__log_color_to_esc_seq[] = {
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

static bool s__log_file_can_be_colored(void) {
	return s__log_file == stdout || s__log_file == stderr;
}

static void s__init_log(void) {
	s__log_is_init = true;

#ifdef PLATFORM_WINDOWS
	s__log_win_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	s__log_win_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

	GetConsoleScreenBufferInfo(s__log_win_stdout_handle, &s__log_orig_csbi);
#endif

	if (s__log_file == NULL)
		s__log_file = stderr;
}

NOCH_DEF void set_log_file(FILE *file) {
	s__log_file = file;
}

NOCH_DEF void set_log_flags(int flags) {
	s__log_flags = flags;
}

static void s__log_reset_color(void) {
	if (s__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(s__log_file_to_win_handle(), s__orig_csbi.wAttributes);
#else
		fputs("\x1b[0m", s__log_file);
#endif
}

NOCH_DEF void log_generic(int color, const char *title, const char *path,
                          size_t line, const char *fmt, ...) {
	if (!s__log_is_init)
		s__init_log();

	s__log_reset_color();

#ifdef PLATFORM_WINDOWS
	HANDLE handle = s__log_file_to_win_handle();
#endif

	if (s__log_flags & LOG_TIME_DATE) {
		if (s__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY);
#else
			fputs("\x1b[0;1;90m", s__log_file);
#endif

		time_t timer;
		struct tm* info;
		timer = time(NULL);
		info  = localtime(&timer);

		if (s__log_flags & LOG_DATE) {
			char buf[16];
			strftime(buf, 16, "%Y-%m-%d ", info);
			fputs(buf, s__log_file);
		}

		if (s__log_flags & LOG_TIME)
			fprintf(s__log_file, "%02d:%02d:%02d ", info->tm_hour, info->tm_min, info->tm_sec);
	}

	if (s__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(handle, s__log_color_to_win_attr[color]);
#else
		fputs(s__log_color_to_esc_seq[color], s__log_file);
#endif

	fprintf(s__log_file, "[%s]", title);

	if (s__log_flags & LOG_LOCATION) {
		if (s__log_file_can_be_colored())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_RED  | FOREGROUND_GREEN |
			                                FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#else
			fputs("\x1b[0;1;97m", s__log_file);
#endif

		fputc(' ', s__log_file);

		if (s__log_flags & LOG_FILE)
			fprintf(s__log_file, "%s:", path);
		else if (s__log_flags & LOG_LINE)
			fprintf(s__log_file, "%zu:", line);
	}

	char    str[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	s__log_reset_color();
	fprintf(s__log_file, " %s\n", str);
}

#ifdef __cplusplus
}
#endif
