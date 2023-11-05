#ifndef NOCH_UTF8_H_HEADER_GUARD
#define NOCH_UTF8_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  /* size_t */
#include <stdint.h>  /* uint32_t, uint8_t */
#include <string.h>  /* strlen */
#include <stdbool.h> /* bool, true, false */

#include "internal/def.h"

/* TODO: Add a way to detect if a utf8 character takes up 2 terminal screen characters. This means
         also add a u8_str_print_len to get the length when printed to the terminal, etc. */

#define U8_1_BYTE_MASK (uint8_t)0x80 /* 10000000 */
#define U8_2_BYTE_MASK (uint8_t)0xE0 /* 11100000 */
#define U8_3_BYTE_MASK (uint8_t)0xF0 /* 11110000 */
#define U8_4_BYTE_MASK (uint8_t)0xF8 /* 11111000 */

#define U8_1_BYTE_MASK_NEG (uint8_t)~0x80 /* 01111111 */
#define U8_2_BYTE_MASK_NEG (uint8_t)~0xE0 /* 00011111 */
#define U8_3_BYTE_MASK_NEG (uint8_t)~0xF0 /* 00001111 */
#define U8_4_BYTE_MASK_NEG (uint8_t)~0xF8 /* 00000111 */

#define FOREACH_IN_STRINGU8(STR, VAR, BODY)                 \
	do {                                                    \
		const char *nochIt_ = STR;                          \
		while (*nochIt_ != '\0';) {                         \
			size_t nochRuneSize_;                           \
			Rune VAR = runeFromU8(nochIt_, &nochRuneSize_); \
			BODY                                            \
			nochIt_ += nochRuneSize_;                       \
		}                                                   \
	} while (0)

typedef uint32_t Rune;

NOCH_DEF size_t getCodepointSize(char cp);
NOCH_DEF size_t getRuneSize     (Rune rune);

NOCH_DEF size_t runeToU8  (Rune rune, char *str);
NOCH_DEF Rune   runeFromU8(const char *str, size_t *size);

NOCH_DEF bool runeIsAscii(Rune rune);
NOCH_DEF Rune runeToLower(Rune rune);
NOCH_DEF Rune runeToUpper(Rune rune);
NOCH_DEF bool runeIsLower(Rune rune);
NOCH_DEF bool runeIsUpper(Rune rune);

NOCH_DEF const char *stringU8Next(const char *str);
NOCH_DEF const char *stringU8Prev(const char *str, const char *base);

NOCH_DEF size_t      stringU8Size  (const char *str);
NOCH_DEF size_t      stringU8Length(const char *str);
NOCH_DEF const char *stringU8At    (const char *str, size_t idx);

NOCH_DEF const char *stringU8FindSub (const char *str, const char *toFind, size_t *idx, bool cs);
NOCH_DEF const char *stringU8FindRune(const char *str, Rune        toFind, size_t *idx, bool cs);

#ifdef __cplusplus
}
#endif
#endif
