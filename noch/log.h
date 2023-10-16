#ifndef NOCH_LOG_H_HEADER_GUARD
#define NOCH_LOG_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdlib.h> /* exit, EXIT_FAILURE */
#include <stdio.h>  /* stdout, stderr, fputs, fputc */
#include <stdarg.h> /* va_list, va_start, va_end, vsnprintf */
#include <time.h>   /* struct tm, time, localtime, strftime,  */

#include "internal/def.h"
#include "platform.h"

#ifdef PLATFORM_WINDOWS
#	include "win.h"
#endif

#define LOG_TIME_DATE (LOG_TIME | LOG_DATE)
#define LOG_LOCATION  (LOG_FILE | LOG_LINE)

enum {
	LOG_BASIC = 0,
	LOG_TIME  = 1 << 0,
	LOG_DATE  = 1 << 1,
	LOG_FILE  = 1 << 2,
	LOG_LINE  = 1 << 3,
};

NOCH_DEF void set_log_file (FILE *file);
NOCH_DEF void set_log_flags(int   flags);

enum {
	LOG_RED = 1,
	LOG_GREEN,
	LOG_YELLOW,
	LOG_BLUE,
	LOG_MAGENTA,
	LOG_CYAN,
	LOG_WHITE,
};

#define LOG_INFO(...)  log_generic(LOG_CYAN,    "INFO",  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_generic(LOG_YELLOW,  "WARN",  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_generic(LOG_RED,     "ERROR", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) \
	(log_generic(LOG_MAGENTA, "FATAL", __FILE__, __LINE__, __VA_ARGS__), exit(EXIT_FAILURE))

NOCH_DEF void log_generic(int color, const char *title, const char *path,
                          size_t line, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
