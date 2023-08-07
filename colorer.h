#ifndef NOCH_COLORER_H_HEADER_GUARD
#define NOCH_COLORER_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdio.h>  /* stdout, stderr, fputs, fputc */
#include <stdarg.h> /* va_list, va_start, va_end, vsnprintf */

#include "internal/def.h"
#include "platform.h"

#ifdef PLATFORM_WINDOWS
#	include "windows.h"
#endif

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

NOCH_DEF void init_color(void);

NOCH_DEF void reset_color (void);
NOCH_DEF void highlight_fg(void);

NOCH_DEF void set_fg_color(int color);
NOCH_DEF void set_bg_color(int color);
NOCH_DEF void set_color   (int fg, int bg);

NOCH_DEF void printf_color(const char *fmt, ...);

NOCH_DEF void freset_color (FILE *file);
NOCH_DEF void fhighlight_fg(FILE *file);

NOCH_DEF void fset_fg_color(FILE *file, int color);
NOCH_DEF void fset_bg_color(FILE *file, int color);
NOCH_DEF void fset_color   (FILE *file, int fg, int bg);

NOCH_DEF void fprintf_color(FILE *file, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
