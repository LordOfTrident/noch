#ifdef __cplusplus
extern "C" {
#endif

#include "internal/assert.h"

#include "utf8.h"

static uint8_t codepointSizeMap[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0,
};

NOCH_DEF size_t getCodepointSize(char cp) {
	return (size_t)codepointSizeMap[(uint8_t)cp];
}

NOCH_DEF size_t getRuneSize(Rune rune) {
	if (rune <= 0x7F)
		return 1;
	else if (rune <= 0x7FF)
		return 2;
	else if (rune <= 0xFFFF)
		return 3;
	else if (rune <= 0x10FFFF)
		return 4;
	else
		return 0;
}

NOCH_DEF size_t runeToU8(Rune rune, char *str) {
	nochAssert(str != NULL);

	if (rune <= 0x7F) {
		str[0] = rune;
		return 1;
	} else if (rune <= 0x7FF) {
		str[0] = 0xC0 | (rune >> 6);
		str[1] = 0x80 | (rune & 0x3F);
		return 2;
	} else if (rune <= 0xFFFF) {
		str[0] = 0xE0 | (rune >> 12);
		str[1] = 0x80 | (rune >> 6 & 0x3F);
		str[2] = 0x80 | (rune      & 0x3F);
		return 3;
	} else if (rune <= 0x10FFFF) {
		str[0] = 0xF0 | (rune >> 18);
		str[1] = 0x80 | (rune >> 12 & 0x3F);
		str[2] = 0x80 | (rune >> 6  & 0x3F);
		str[3] = 0x80 | (rune       & 0x3F);
		return 4;
	} else
		return (size_t)-1;
}

NOCH_DEF Rune runeFromU8(const char *str, size_t *size) {
	nochAssert(str != NULL);

	size_t runeSize = getCodepointSize(*str);

	if (size != NULL)
		*size = runeSize;

	switch (runeSize) {
	case 1: return (Rune)(str[0] & U8_1_BYTE_MASK_NEG);
	case 2: return (Rune)(str[0] & U8_2_BYTE_MASK_NEG) << 6  | (Rune)(str[1] & 0x3F);
	case 3: return (Rune)(str[0] & U8_3_BYTE_MASK_NEG) << 12 | (Rune)(str[1] & 0x3F) << 6 |
	               (Rune)(str[2] & 0x3F);
	case 4: return (Rune)(str[0] & U8_4_BYTE_MASK_NEG) << 18 | (Rune)(str[1] & 0x3F) << 12 |
	               (Rune)(str[2] & 0x3F)               << 6  | (Rune)(str[3] & 0x3F);

	default: return (Rune)-1;
	}
}

NOCH_DEF bool runeIsAscii(Rune rune) {
	return rune < 128;
}

/* TODO: Support unicode */
NOCH_DEF Rune runeToLower(Rune rune) {
	return rune >= 'A' && rune <= 'Z'? rune - 'A' + 'a' : rune;
}

NOCH_DEF Rune runeToUpper(Rune rune) {
	return rune >= 'a' && rune <= 'z'? rune - 'a' + 'A' : rune;
}

NOCH_DEF bool runeuneIsLower(Rune rune) {
	return rune >= 'a' && rune <= 'z';
}

NOCH_DEF bool runeuneIsUpper(Rune rune) {
	return rune >= 'A' && rune <= 'Z';
}

NOCH_DEF const char *stringU8Next(const char *str) {
	nochAssert(str != NULL);

	if (*str == '\0')
		return NULL;

	size_t runeSize = getCodepointSize(*str);
	return runeSize == 0? NULL : str + runeSize;
}

NOCH_DEF const char *stringU8Prev(const char *str, const char *base) {
	nochAssert(str != NULL);

	do {
		-- str;
		if (str < base)
			return NULL;
	} while ((*str & 0x80) != 0 && (*str & 0xC0) != 0xC0);

	return str;
}

NOCH_DEF size_t stringU8Size(const char *str) {
	nochAssert(str != NULL);
	return strlen(str);
}

NOCH_DEF size_t stringU8Length(const char *str) {
	nochAssert(str != NULL);

	size_t len = 0;
	while (*str != '\0') {
		++ len;
		str += getCodepointSize(*str);
	}
	return len;
}

NOCH_DEF const char *stringU8At(const char *str, size_t idx) {
	nochAssert(str != NULL);

	const char *it = str;
	size_t      at = 0;
	while (*it != '\0') {
		if (at == idx)
			return it;

		++ at;
		it += getCodepointSize(*it);
	}

	if (*it == '\0')
		return it;

	return NULL;
}

NOCH_DEF const char *stringU8FindSub(const char *str, const char *find, size_t *idx, bool cs) {
	nochAssert(str  != NULL);
	nochAssert(find != NULL);

	const char *it = str;
	size_t      at = 0;
	while (*it != '\0') {
		const char *a = it, *b = find;
		Rune      runeA,     runeB;
		while (true) {
			size_t sizeA, sizeB;
			runeA = runeFromU8(a, &sizeA);
			runeB = runeFromU8(b, &sizeB);

			bool check = cs? runeA != runeB : runeToLower(runeA) != runeToLower(runeB);
			if (check || runeA == '\0' || runeB == '\0')
				break;

			a += sizeA;
			b += sizeB;
		}

		if (runeB == '\0') {
			if (idx != NULL)
				*idx = at;

			return it;
		}

		++ at;
		it += getCodepointSize(*it);
	}

	return NULL;
}

NOCH_DEF const char *stringU8FindRune(const char *str, Rune find, size_t *idx, bool cs) {
	nochAssert(str != NULL);

	const char *it = str;
	size_t      at = 0;
	while (*it != '\0') {
		size_t size;
		Rune rune = runeFromU8(it, &size);

		if (cs? rune == find : runeToLower(rune) == runeToLower(find)) {
			if (idx != NULL)
				*idx = at;

			return it;
		}

		++ at;
		it += getCodepointSize(*it);
	}

	if (idx != NULL)
		*idx = (size_t)-1;

	return NULL;
}

#ifdef __cplusplus
}
#endif
