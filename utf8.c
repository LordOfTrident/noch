#ifdef __cplusplus
extern "C" {
#endif

#include "utf8.h"

static uint8_t u8_rune_size_map[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0,
};

NOCH_DEF size_t u8_rune_size(char cp) {
	return (size_t)u8_rune_size_map[(uint8_t)cp];
}

NOCH_DEF size_t rune_size(rune_t r) {
	if (r <= 0x7F)
		return 1;
	else if (r <= 0x7FF)
		return 2;
	else if (r <= 0xFFFF)
		return 3;
	else if (r <= 0x10FFFF)
		return 4;
	else
		return (size_t)-1;
}

NOCH_DEF size_t rune_encode_u8(rune_t r, char *b) {
	NOCH_ASSERT(b != NULL);

	if (r <= 0x7F) {
		b[0] = r;
		return 1;
	} else if (r <= 0x7FF) {
		b[0] = 0xC0 | (r >> 6);
		b[1] = 0x80 | (r & 0x3F);
		return 2;
	} else if (r <= 0xFFFF) {
		b[0] = 0xE0 | (r >> 12);
		b[1] = 0x80 | (r >> 6 & 0x3F);
		b[2] = 0x80 | (r      & 0x3F);
		return 3;
	} else if (r <= 0x10FFFF) {
		b[0] = 0xF0 | (r >> 18);
		b[1] = 0x80 | (r >> 12 & 0x3F);
		b[2] = 0x80 | (r >> 6  & 0x3F);
		b[3] = 0x80 | (r       & 0x3F);
		return 4;
	} else
		return (size_t)-1;
}

NOCH_DEF rune_t rune_decode_u8(const char *b, size_t *size) {
	NOCH_ASSERT(b != NULL);

	size_t rune_size = u8_rune_size(*b);

	if (size != NULL)
		*size = rune_size;

	switch (rune_size) {
	case 1: return (rune_t)(b[0] & U8_1_BYTE_MASK_NEG);
	case 2: return (rune_t)(b[0] & U8_2_BYTE_MASK_NEG) << 6  | (rune_t)(b[1] & 0x3F);
	case 3: return (rune_t)(b[0] & U8_3_BYTE_MASK_NEG) << 12 | (rune_t)(b[1] & 0x3F) << 6 |
	               (rune_t)(b[2] & 0x3F);
	case 4: return (rune_t)(b[0] & U8_4_BYTE_MASK_NEG) << 18 | (rune_t)(b[1] & 0x3F) << 12 |
	               (rune_t)(b[2] & 0x3F)               << 6  | (rune_t)(b[3] & 0x3F);

	default: return (rune_t)-1;
	}
}

NOCH_DEF bool rune_is_ascii(rune_t r) {
	return r < 128;
}

/* TODO: Support unicode */
NOCH_DEF rune_t rune_to_lower(rune_t r) {
	return r >= 'A' && r <= 'Z'? r - 'A' + 'a' : r;
}

NOCH_DEF rune_t rune_to_upper(rune_t r) {
	return r >= 'a' && r <= 'z'? r - 'a' + 'A' : r;
}

NOCH_DEF bool rune_is_lower(rune_t r) {
	return r >= 'a' && r <= 'z';
}

NOCH_DEF bool rune_is_upper(rune_t r) {
	return r >= 'A' && r <= 'Z';
}

NOCH_DEF const char *u8_str_next(const char *str) {
	NOCH_ASSERT(str != NULL);

	if (*str == '\0')
		return NULL;

	size_t rune_size = u8_rune_size(*str);
	return rune_size == 0? NULL : str + rune_size;
}

NOCH_DEF const char *u8_str_prev(const char *str, const char *base) {
	NOCH_ASSERT(str != NULL);

	do {
		-- str;
		if (str < base)
			return NULL;
	} while ((*str & 0x80) != 0 && (*str & 0xC0) != 0xC0);

	return str;
}

NOCH_DEF size_t u8_str_bytes(const char *str) {
	NOCH_ASSERT(str != NULL);

	return strlen(str);
}

NOCH_DEF size_t u8_str_len(const char *str) {
	NOCH_ASSERT(str != NULL);

	size_t len = 0;
	while (*str != '\0') {
		++ len;
		str += u8_rune_size(*str);
	}
	return len;
}

NOCH_DEF const char *u8_str_idx_to_ptr(const char *str, size_t idx) {
	NOCH_ASSERT(str != NULL);

	const char *it  = str;
	size_t      cur = 0;
	while (*it != '\0') {
		if (cur == idx)
			return it;

		++ cur;
		it += u8_rune_size(*it);
	}

	if (*it == '\0')
		return it;

	return NULL;
}

NOCH_DEF const char *u8_str_find_str(const char *str, const char *find, size_t *idx) {
	NOCH_ASSERT(str  != NULL);
	NOCH_ASSERT(find != NULL);

	const char *it  = str;
	size_t      cur = 0;
	while (*it != '\0') {
		const char *a = it, *b = find;
		rune_t      r_a,     r_b;
		while (true) {
			size_t size_a, size_b;
			r_a = rune_decode_u8(a, &size_a);
			r_b = rune_decode_u8(b, &size_b);

			if (r_a != r_b || r_a == '\0' || r_b == '\0')
				break;

			a += size_a;
			b += size_b;
		}

		if (r_b == '\0') {
			if (idx != NULL)
				*idx = cur;

			return it;
		}

		++ cur;
		it += u8_rune_size(*it);
	}

	return NULL;
}

NOCH_DEF const char *u8_str_find_rune(const char *str, rune_t find, size_t *idx) {
	NOCH_ASSERT(str != NULL);

	const char *it  = str;
	size_t      cur = 0;
	while (*it != '\0') {
		size_t size;
		rune_t r = rune_decode_u8(it, &size);

		if (r == find) {
			if (idx != NULL)
				*idx = cur;

			return it;
		}

		++ cur;
		it += u8_rune_size(*it);
	}

	if (idx != NULL)
		*idx = (size_t)-1;

	return NULL;
}

NOCH_DEF const char *u8_str_find_str_ci(const char *str, const char *find, size_t *idx) {
	NOCH_ASSERT(str  != NULL);
	NOCH_ASSERT(find != NULL);

	const char *it  = str;
	size_t      cur = 0;
	while (*it != '\0') {
		const char *a = it, *b = find;
		rune_t      r_a,     r_b;
		while (true) {
			size_t size_a, size_b;
			r_a = rune_decode_u8(a, &size_a);
			r_b = rune_decode_u8(b, &size_b);

			if (rune_to_lower(r_a) != rune_to_lower(r_b) || r_a == '\0' || r_b == '\0')
				break;

			a += size_a;
			b += size_b;
		}

		if (r_b == '\0') {
			if (idx != NULL)
				*idx = cur;

			return it;
		}

		++ cur;
		it += u8_rune_size(*it);
	}

	return NULL;
}

NOCH_DEF const char *u8_str_find_rune_ci(const char *str, rune_t find, size_t *idx) {
	NOCH_ASSERT(str != NULL);

	const char *it  = str;
	size_t      cur = 0;
	while (*it != '\0') {
		size_t size;
		rune_t r = rune_decode_u8(it, &size);

		if (rune_to_lower(r) == rune_to_lower(find)) {
			if (idx != NULL)
				*idx = cur;

			return it;
		}

		++ cur;
		it += u8_rune_size(*it);
	}

	if (idx != NULL)
		*idx = (size_t)-1;

	return NULL;
}

#ifdef __cplusplus
}
#endif
