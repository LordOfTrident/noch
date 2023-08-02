#ifndef NOCH_UTF8_H_HEADER_GUARD
#define NOCH_UTF8_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t, uint8_t */
#include <string.h> /* strlen */

#include "internal/assert.h"
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

#define FOREACH_IN_U8_STR(STR, VAR, BODY)                          \
	do {                                                           \
		const char *_foreach_it = STR;                             \
		while (*_foreach_it != '\0';) {                            \
			size_t _rune_size;                                     \
			rune_t VAR = rune_decode_u8(_foreach_it, &_rune_size); \
			BODY                                                   \
			_foreach_it += _rune_size;                             \
		}                                                          \
	} while (0)

typedef uint32_t rune_t;

NOCH_DEF size_t u8_rune_size(char ch);
NOCH_DEF size_t rune_size(rune_t r);

NOCH_DEF size_t rune_encode_u8(rune_t r, char *b);
NOCH_DEF rune_t rune_decode_u8(const char *b, size_t *size);

NOCH_DEF bool   rune_is_ascii(rune_t r);
NOCH_DEF rune_t rune_to_lower(rune_t r);
NOCH_DEF rune_t rune_to_upper(rune_t r);
NOCH_DEF bool   rune_is_lower(rune_t r);
NOCH_DEF bool   rune_is_upper(rune_t r);

NOCH_DEF const char *u8_str_next(const char *str);
NOCH_DEF const char *u8_str_prev(const char *str, const char *base);

NOCH_DEF size_t      u8_str_bytes     (const char *str);
NOCH_DEF size_t      u8_str_len       (const char *str);
NOCH_DEF const char *u8_str_idx_to_ptr(const char *str, size_t idx);

NOCH_DEF const char *u8_str_find_str (const char *str, const char *find, size_t *idx);
NOCH_DEF const char *u8_str_find_rune(const char *str, rune_t      find, size_t *idx);

NOCH_DEF const char *u8_str_find_str_ci (const char *str, const char *find, size_t *idx);
NOCH_DEF const char *u8_str_find_rune_ci(const char *str, rune_t      find, size_t *idx);

#ifdef __cplusplus
}
#endif
#endif
