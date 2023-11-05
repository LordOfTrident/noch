#ifndef NOCH_COLORER_H_HEADER_GUARD
#define NOCH_COLORER_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>   /* stdout, stderr, fprintf */
#include <stdarg.h>  /* va_list, va_start, va_end, vsnprintf */
#include <stdbool.h> /* bool, true, false */

#include "internal/def.h"
#include "platform.h"

#ifdef PLATFORM_WINDOWS
#	include "windows.h"
#endif

/* TODO: Support enabling ansi escape sequences on windows */

enum {
	COLOR_BLACK = 0,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_WHITE,

	COLOR_GREY,
	COLOR_BRIGHT_RED,
	COLOR_BRIGHT_GREEN,
	COLOR_BRIGHT_YELLOW,
	COLOR_BRIGHT_BLUE,
	COLOR_BRIGHT_MAGENTA,
	COLOR_BRIGHT_CYAN,
	COLOR_BRIGHT_WHITE,
};

NOCH_DEF void colorResetF    (FILE *file);
NOCH_DEF void colorHighlightF(FILE *file);

NOCH_DEF void colorSetFgF(FILE *file, int color);
NOCH_DEF void colorSetBgF(FILE *file, int color);
NOCH_DEF void colorSetF  (FILE *file, int fg, int bg);

NOCH_DEF void colorPrintF(FILE *file, const char *fmt, ...);

#define colorReset()     colorResetF(stdout)
#define colorHighlight() colorHighlightF(stdout)

#define colorSetFg(COLOR) colorSetFgF(stdout, COLOR)
#define colorSetBg(COLOR) colorSetBgF(stdout, COLOR)
#define colorSet(FG, BG)  colorSetF  (stdout, FG, BG)

#define colorPrint(...) colorPrintF(stdout, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif
