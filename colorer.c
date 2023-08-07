#ifdef __cplusplus
extern "C" {
#endif

#include "colorer.h"

#ifdef PLATFORM_WINDOWS

static WORD s__color_to_win_attr_fg[] = {
	/* COLOR_BLACK   */ 0,
	/* COLOR_RED     */ FOREGROUND_RED,
	/* COLOR_GREEN   */ FOREGROUND_GREEN,
	/* COLOR_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN,
	/* COLOR_BLUE    */ FOREGROUND_BLUE,
	/* COLOR_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE,
	/* COLOR_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE,
	/* COLOR_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

	/* COLOR_GREY          */ FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_RED     */ FOREGROUND_RED | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_GREEN   */ FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_BLUE    */ FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
	                           FOREGROUND_INTENSITY,
};

static WORD s__color_to_win_attr_bg[] = {
	/* COLOR_BLACK   */ 0,
	/* COLOR_RED     */ BACKGROUND_RED,
	/* COLOR_GREEN   */ BACKGROUND_GREEN,
	/* COLOR_YELLOW  */ BACKGROUND_RED | BACKGROUND_GREEN,
	/* COLOR_BLUE    */ BACKGROUND_BLUE,
	/* COLOR_MAGENTA */ BACKGROUND_RED | BACKGROUND_BLUE,
	/* COLOR_CYAN    */ BACKGROUND_GREEN | BACKGROUND_BLUE,
	/* COLOR_WHITE   */ BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,

	/* COLOR_GREY           */ BACKGROUND_INTENSITY,
	/* COLOR_BRIGHT_RED     */ BACKGROUND_RED | BACKGROUND_INTENSITY,
	/* COLOR_BRIGHT_GREEN   */ BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	/* COLOR_BRIGHT_YELLOW  */ BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	/* COLOR_BRIGHT_BLUE    */ BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	/* COLOR_BRIGHT_MAGENTA */ BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	/* COLOR_BRIGHT_CYAN    */ BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	/* COLOR_BRIGHT_WHITE   */ BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE |
	                           BACKGROUND_INTENSITY,
};

static WORD s__current_fg_color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
static WORD s__current_bg_color = 0;

static HANDLE s__win_stdout_handle;
static HANDLE s__win_stderr_handle;

static CONSOLE_SCREEN_BUFFER_INFO s__orig_csbi;

static HANDLE s__file_to_win_handle(FILE *file) {
	if (file == stdout)
		return s__win_stdout_handle;
	else if (file == stderr)
		return s__win_stderr_handle;
	else
		return INVALID_HANDLE_VALUE;
}

#else

static const char *s__color_to_esc_seq_fg[] = {
	/* COLOR_BLACK   */ "\x1b[30m",
	/* COLOR_RED     */ "\x1b[31m",
	/* COLOR_GREEN   */ "\x1b[32m",
	/* COLOR_YELLOW  */ "\x1b[33m",
	/* COLOR_BLUE    */ "\x1b[34m",
	/* COLOR_MAGENTA */ "\x1b[35m",
	/* COLOR_CYAN    */ "\x1b[36m",
	/* COLOR_WHITE   */ "\x1b[37m",

	/* COLOR_GREY           */ "\x1b[90m",
	/* COLOR_BRIGHT_RED     */ "\x1b[91m",
	/* COLOR_BRIGHT_GREEN   */ "\x1b[92m",
	/* COLOR_BRIGHT_YELLOW  */ "\x1b[93m",
	/* COLOR_BRIGHT_BLUE    */ "\x1b[94m",
	/* COLOR_BRIGHT_MAGENTA */ "\x1b[95m",
	/* COLOR_BRIGHT_CYAN    */ "\x1b[96m",
	/* COLOR_BRIGHT_WHITE   */ "\x1b[97m",
};

static const char *s__color_to_esc_seq_bg[] = {
	/* COLOR_BLACK   */ "\x1b[40m",
	/* COLOR_RED     */ "\x1b[41m",
	/* COLOR_GREEN   */ "\x1b[42m",
	/* COLOR_YELLOW  */ "\x1b[43m",
	/* COLOR_BLUE    */ "\x1b[44m",
	/* COLOR_MAGENTA */ "\x1b[45m",
	/* COLOR_CYAN    */ "\x1b[46m",
	/* COLOR_WHITE   */ "\x1b[47m",

	/* COLOR_GREY           */ "\x1b[100m",
	/* COLOR_BRIGHT_RED     */ "\x1b[101m",
	/* COLOR_BRIGHT_GREEN   */ "\x1b[102m",
	/* COLOR_BRIGHT_YELLOW  */ "\x1b[103m",
	/* COLOR_BRIGHT_BLUE    */ "\x1b[104m",
	/* COLOR_BRIGHT_MAGENTA */ "\x1b[105m",
	/* COLOR_BRIGHT_CYAN    */ "\x1b[106m",
	/* COLOR_BRIGHT_WHITE   */ "\x1b[107m",
};

#endif

static bool s__file_can_be_colored(FILE *file) {
	return file == stdout || file == stderr;
}

NOCH_DEF void init_color(void) {
#ifdef PLATFORM_WINDOWS
	s__win_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	s__win_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

	GetConsoleScreenBufferInfo(s__win_stdout_handle, &s__orig_csbi);
#endif
}

NOCH_DEF void freset_color(FILE *file) {
	if (!s__file_can_be_colored(file))
		return;

#ifdef PLATFORM_WINDOWS
	SetConsoleTextAttribute(s__file_to_win_handle(file), s__orig_csbi.wAttributes);
#else
	fputs("\x1b[0m", file);
#endif
}

NOCH_DEF void fhighlight_fg(FILE *file) {
	if (!s__file_can_be_colored(file))
		return;

#ifdef PLATFORM_WINDOWS
	s__current_fg_color |= FOREGROUND_INTENSITY;
#else
	fputs("\x1b[1m", file);
#endif
}

NOCH_DEF void fset_fg_color(FILE *file, int color) {
	if (!s__file_can_be_colored(file))
		return;

#ifdef WIN32
	s__current_fg_color = s__color_to_win_attr_fg[color];
	SetConsoleTextAttribute(s__file_to_win_handle(file), s__current_fg_color | s__current_bg_color);
#else
	fputs(s__color_to_esc_seq_fg[color], file);
#endif
}

NOCH_DEF void fset_bg_color(FILE *file, int color) {
	if (!s__file_can_be_colored(file))
		return;

#ifdef WIN32
	s__current_bg_color = s__color_to_win_attr_bg[color];
	SetConsoleTextAttribute(s__file_to_win_handle(file), s__current_fg_color | s__current_bg_color);
#else
	fputs(s__color_to_esc_seq_bg[color], file);
#endif
}

NOCH_DEF void fset_color(FILE *file, int fg, int bg) {
	fset_fg_color(file, fg);
	fset_bg_color(file, bg);
}

NOCH_DEF void fprintf_color(FILE *file, const char *fmt, ...) {
	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	bool escape = false, get_color = false, bg = false;
	for (const char *ch = str; *ch != '\0'; ++ ch) {
		if (escape) {
			escape = false;

			switch (*ch) {
			case '#': fputc('#', file);    break;
			case 'X': freset_color(file);  break;
			case '!': fhighlight_fg(file); break;

			case 'f': get_color = true; bg = false; break;
			case 'b': get_color = true; bg = true;  break;

			default:
				fputc('#', file);
				fputc(*ch, file);
			}
		} else if (get_color) {
			get_color = false;

			void (*color_func)(FILE*, int) = bg? fset_bg_color : fset_fg_color;

			switch (*ch) {
			case 'o': color_func(file, COLOR_BLACK);   break;
			case 'r': color_func(file, COLOR_RED);     break;
			case 'g': color_func(file, COLOR_GREEN);   break;
			case 'y': color_func(file, COLOR_YELLOW);  break;
			case 'b': color_func(file, COLOR_BLUE);    break;
			case 'm': color_func(file, COLOR_MAGENTA); break;
			case 'c': color_func(file, COLOR_CYAN);    break;
			case 'w': color_func(file, COLOR_WHITE);   break;

			case 'O': color_func(file, COLOR_GREY);           break;
			case 'R': color_func(file, COLOR_BRIGHT_RED);     break;
			case 'G': color_func(file, COLOR_BRIGHT_GREEN);   break;
			case 'Y': color_func(file, COLOR_BRIGHT_YELLOW);  break;
			case 'B': color_func(file, COLOR_BRIGHT_BLUE);    break;
			case 'M': color_func(file, COLOR_BRIGHT_MAGENTA); break;
			case 'C': color_func(file, COLOR_BRIGHT_CYAN);    break;
			case 'W': color_func(file, COLOR_BRIGHT_WHITE);   break;

			default:
				fputc('#', file);
				fputc(bg? 'b' : 'f', file);
				fputc(*ch, file);
			}
		} else if (*ch == '#')
			escape = true;
		else
			fputc(*ch, file);
	}
}

NOCH_DEF void reset_color(void) {
	freset_color(stdout);
}

NOCH_DEF void highlight_fg(void) {
	fhighlight_fg(stdout);
}

NOCH_DEF void set_fg_color(int color) {
	fset_fg_color(stdout, color);
}

NOCH_DEF void set_bg_color(int color) {
	fset_bg_color(stdout, color);
}

NOCH_DEF void set_color(int fg, int bg) {
	fset_color(stdout, fg, bg);
}

NOCH_DEF void printf_color(const char *fmt, ...) {
	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	fprintf_color(stdout, str);
}

#ifdef __cplusplus
}
#endif
