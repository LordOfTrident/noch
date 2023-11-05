#ifdef __cplusplus
extern "C" {
#endif

#include "colorer.h"

#ifdef PLATFORM_WINDOWS

static WORD fgColorToAttr[] = {
	/* COLOR_BLACK   */ 0,
	/* COLOR_RED     */ FOREGROUND_RED,
	/* COLOR_GREEN   */ FOREGROUND_GREEN,
	/* COLOR_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN,
	/* COLOR_BLUE    */ FOREGROUND_BLUE,
	/* COLOR_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE,
	/* COLOR_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE,
	/* COLOR_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

	/* COLOR_GREY           */ FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_RED     */ FOREGROUND_RED | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_GREEN   */ FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_YELLOW  */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_BLUE    */ FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_MAGENTA */ FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_CYAN    */ FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	/* COLOR_BRIGHT_WHITE   */ FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
	                           FOREGROUND_INTENSITY,
};

static WORD bgColorToAttr[] = {
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

static WORD fgColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
static WORD bgColor = 0;

static HANDLE stdoutHandle;
static HANDLE stderrHandle;

static CONSOLE_SCREEN_BUFFER_INFO prevCsbi;

static HANDLE fileToHandle(FILE *file) {
	if (file == stdout)
		return stdoutHandle;
	else if (file == stderr)
		return stderrHandle;
	else
		return INVALID_HANDLE_VALUE;
}

#else

static const char *fgColorToAnsi[] = {
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

static const char *bgColorToAnsi[] = {
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

static bool colorIsInit = false;

static bool hasColor(FILE *file) {
	return file == stdout || file == stderr;
}

static void colorInit(void) {
#ifdef PLATFORM_WINDOWS
	stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	stderrHandle = GetStdHandle(STD_ERROR_HANDLE);

	GetConsoleScreenBufferInfo(stdoutHandle, &prevCsbi);
#endif

	colorIsInit = true;
}

NOCH_DEF void colorResetF(FILE *file) {
	if (!colorIsInit)
		colorInit();

	if (!hasColor(file))
		return;

#ifdef PLATFORM_WINDOWS
	SetConsoleTextAttribute(fileToHandle(file), prevCsbi.wAttributes);
#else
	fprintf(file, "\x1b[0m");
#endif
}

NOCH_DEF void colorHighlightF(FILE *file) {
	if (!colorIsInit)
		colorInit();

	if (!hasColor(file))
		return;

#ifdef PLATFORM_WINDOWS
	fgColor |= FOREGROUND_INTENSITY;
#else
	fprintf(file, "\x1b[1m");
#endif
}

NOCH_DEF void colorSetFgF(FILE *file, int color) {
	if (!colorIsInit)
		colorInit();

	if (!hasColor(file))
		return;

#ifdef WIN32
	fgColor = fgColorToAttr[color];
	SetConsoleTextAttribute(fileToHandle(file), fgColor | bgColor);
#else
	fprintf(file, "%s", fgColorToAnsi[color]);
#endif
}

NOCH_DEF void colorSetBgF(FILE *file, int color) {
	if (!colorIsInit)
		colorInit();

	if (!hasColor(file))
		return;

#ifdef WIN32
	bgColor = bgColorToAttr[color];
	SetConsoleTextAttribute(file_to_handle(file), fgColor | bgColor);
#else
	fprintf(file, "%s", bgColorToAnsi[color]);
#endif
}

NOCH_DEF void colorSetF(FILE *file, int fg, int bg) {
	if (!colorIsInit)
		colorInit();

	colorSetFgF(file, fg);
	colorSetBgF(file, bg);
}

NOCH_DEF void colorPrintF(FILE *file, const char *fmt, ...) {
	if (!colorIsInit)
		colorInit();

	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	bool escape = false;
	for (const char *it = str; *it != '\0'; ++ it) {
		if (*it == '\\' && it[1] == '[')
			escape = true;
		else if (*it == '[' && !escape) {
			int len = 0;
			for (int i = 0; i < 3; ++ i) {
				if (it[i + 1] == '\0')
					break;
				else if (it[i + 1] == ']') {
					len = i;
					break;
				}
			}
			if (len == 0)
				goto printAndContinue;

			if      (it[1] == '.' && len == 1) colorResetF(file);
			else if (it[1] == '!' && len == 1) colorHighlightF(file);
			else {
				bool bg = false;
				char colorCode;
				if (len == 2) {
					if (it[1] == '*')
						bg = true;
					else if (it[1] == '!')
						colorHighlightF(file);
					else
						goto printAndContinue;

					colorCode = it[2];
				} else
					colorCode = it[1];

				void (*colorFunc)(FILE*, int) = bg? colorSetBgF : colorSetFgF;

				switch (colorCode) {
				case 'o': colorFunc(file, COLOR_BLACK);   break;
				case 'r': colorFunc(file, COLOR_RED);     break;
				case 'g': colorFunc(file, COLOR_GREEN);   break;
				case 'y': colorFunc(file, COLOR_YELLOW);  break;
				case 'b': colorFunc(file, COLOR_BLUE);    break;
				case 'm': colorFunc(file, COLOR_MAGENTA); break;
				case 'c': colorFunc(file, COLOR_CYAN);    break;
				case 'w': colorFunc(file, COLOR_WHITE);   break;

				case 'O': colorFunc(file, COLOR_GREY);           break;
				case 'R': colorFunc(file, COLOR_BRIGHT_RED);     break;
				case 'G': colorFunc(file, COLOR_BRIGHT_GREEN);   break;
				case 'Y': colorFunc(file, COLOR_BRIGHT_YELLOW);  break;
				case 'B': colorFunc(file, COLOR_BRIGHT_BLUE);    break;
				case 'M': colorFunc(file, COLOR_BRIGHT_MAGENTA); break;
				case 'C': colorFunc(file, COLOR_BRIGHT_CYAN);    break;
				case 'W': colorFunc(file, COLOR_BRIGHT_WHITE);   break;

				default: goto printAndContinue;
				}
			}

			it += len + 1;
			continue;
		} else if (escape) {
			escape = false;
			fprintf(file, "\\");
		}

	printAndContinue:
		fprintf(file, "%c", *it);
	}

#undef MAX_FMT_LEN

}

#ifdef __cplusplus
}
#endif
