#ifdef __cplusplus
extern "C" {
#endif

#include "log.h"

static FILE *logFile   = NULL;
static int   logFlags  = LOG_BASIC;
static bool  logIsInit = false;

#ifdef PLATFORM_WINDOWS

static WORD logColorToAttr[] = {
	0,
	/* LOG_RED     */ FOREGROUND_RED | FOREGROUND_INTENSITY,
	/* LOG_GREEN   */ FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* LOG_BLUE    */ FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* LOG_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

static HANDLE logStdoutHandle;
static HANDLE logStderrHandle;

static CONSOLE_SCREEN_BUFFER_INFO logPrevCsbi;

static HANDLE logFileToHandle(void) {
	if (logFile == stdout)
		return logStdoutHandle;
	else if (logFile == stderr)
		return logStderrHandle;
	else
		return INVALID_HANDLE_VALUE;
}

#else

static const char *logColorToAnsi[] = {
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

static bool logHasColor(void) {
	return logFile == stdout || logFile == stderr;
}

static void logInit(void) {
	logIsInit = true;

#ifdef PLATFORM_WINDOWS
	logStdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	logStderrHandle = GetStdHandle(STD_ERROR_HANDLE);

	GetConsoleScreenBufferInfo(logStdoutHandle, &logPrevCsbi);
#endif

	if (logFile == NULL)
		logFile = stderr;
}

NOCH_DEF void logSetFile(FILE *file) {
	logFile = file;
}

NOCH_DEF void logSetFlags(int flags) {
	logFlags = flags;
}

static void logResetColor(void) {
	if (logHasColor())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(logFileToHandle(), logPrevCsbi.wAttributes);
#else
		fprintf(logFile, "\x1b[0m");
#endif
}

NOCH_DEF void logGeneric(int color, const char *title, const char *path,
                         size_t line, const char *fmt, ...) {
	if (!logIsInit)
		logInit();

	logResetColor();

#ifdef PLATFORM_WINDOWS
	HANDLE handle = logFileToHandle();
#endif

	if (logFlags & LOG_TIME_DATE) {
		if (logHasColor())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY);
#else
			fprintf(logFile, "\x1b[0;1;90m");
#endif

		time_t timer;
		struct tm* info;
		timer = time(NULL);
		info  = localtime(&timer);

		if (logFlags & LOG_DATE) {
			char buf[16];
			strftime(buf, 16, "%Y-%m-%d ", info);
			fprintf(logFile, "%s", buf);
		}

		if (logFlags & LOG_TIME)
			fprintf(logFile, "%02d:%02d:%02d ", info->tm_hour, info->tm_min, info->tm_sec);
	}

	if (logHasColor())
#ifdef PLATFORM_WINDOWS
		SetConsoleTextAttribute(handle, logColorToAttr[color]);
#else
		fprintf(logFile, "%s", logColorToAnsi[color]);
#endif

	fprintf(logFile, "[%s]", title);

	if (logFlags & LOG_LOCATION) {
		if (logHasColor())
#ifdef PLATFORM_WINDOWS
			SetConsoleTextAttribute(handle, FOREGROUND_RED  | FOREGROUND_GREEN |
			                                FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#else
			fprintf(logFile, "\x1b[0;1;97m");
#endif

		fprintf(logFile, " ");

		if (logFlags & LOG_FILE)
			fprintf(logFile, "%s:", path);
		if (logFlags & LOG_LINE)
			fprintf(logFile, "%lu:", (long unsigned)line);
	}

	char    str[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	logResetColor();
	fprintf(logFile, " %s\n", str);
}

#ifdef __cplusplus
}
#endif
