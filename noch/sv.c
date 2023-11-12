#ifdef __cplusplus
extern "C" {
#endif

#include "internal/assert.h"

#include "sv.h"

#define SV_NPOS        (size_t)-1
#define SV_WHITESPACES " \f\n\r\t\v"

NOCH_DEF StringView svNew(const char *data, size_t len) {
	StringView this;
	this.length = len;
	this.data   = data;
	return this;
}

NOCH_DEF StringView svFromString(const char *cstr) {
	StringView this;
	this.length = strlen(cstr);
	this.data   = cstr;
	return this;
}

NOCH_DEF char svAt(StringView this, size_t idx) {
	nochAssert(idx < this.length);
	return this.data[idx];
}

NOCH_DEF bool svIsNull(StringView this) {
	return this.data == NULL;
}

NOCH_DEF bool svEquals(StringView this, StringView to) {
	if (this.length != to.length)
		return false;

	for (size_t i = 0; i < this.length; ++ i) {
		if (this.data[i] != to.data[i])
			return false;
	}

	return true;
}

NOCH_DEF bool svHasPrefix(StringView this, StringView prefix) {
	if (this.length < prefix.length)
		return false;

	for (size_t i = 0; i < prefix.length; ++ i) {
		if (this.data[i] != prefix.data[i])
			return false;
	}

	return true;
}

NOCH_DEF bool svHasSuffix(StringView this, StringView suffix) {
	if (this.length < suffix.length)
		return false;

	for (size_t i = this.length - suffix.length; i < this.length; ++ i) {
		if (this.data[i] != suffix.data[i])
			return false;
	}

	return true;
}

NOCH_DEF StringView svSubstring(StringView this, size_t start, size_t length) {
	if (length != SV_NPOS) {
		if (start + length > this.length)
			return SV_NULL;
	} else if (start > this.length)
		return SV_NULL;

	StringView substr;
	substr.data = this.data + start;
	substr.length = length == SV_NPOS? this.length - start : length;
	return substr;
}

NOCH_DEF StringView svTrimLeft(StringView this, const char *chs) {
	StringView matches = svFromString(chs);
	StringView trimmed = this;
	for (size_t i = 0; i < this.length; ++ i) {
		if (!svContains(matches, this.data[i]))
			break;

		++ trimmed.data;
		-- trimmed.length;
	}

	return trimmed;
}

NOCH_DEF StringView svTrimRight(StringView this, const char *chs) {
	StringView matches = svFromString(chs);
	StringView trimmed = this;
	for (size_t i = this.length - 1; i != SV_NPOS; -- i) {
		if (!svContains(matches, this.data[i]))
			break;

		-- trimmed.length;
	}

	return trimmed;
}

NOCH_DEF StringView svTrim(StringView this, const char *chs) {
	this = svTrimLeft (this, chs);
	return svTrimRight(this, chs);
}

NOCH_DEF bool svContains(StringView this, char ch) {
	for (size_t i = 0; i < this.length; ++ i) {
		if (this.data[i] == ch)
			return true;
	}

	return false;
}

NOCH_DEF size_t svCount(StringView this, char ch) {
	size_t count = 0;
	for (size_t i = 0; i < this.length; ++ i) {
		if (this.data[i] == ch)
			++ count;
	}

	return count;
}

NOCH_DEF size_t svFindFirst(StringView this, char ch) {
	for (size_t i = 0; i < this.length; ++ i) {
		if (this.data[i] == ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF size_t svFindLast(StringView this, char ch) {
	for (size_t i = this.length - 1; i != SV_NPOS; -- i) {
		if (this.data[i] == ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF size_t svFindFirstNot(StringView this, char ch) {
	for (size_t i = 0; i < this.length; ++ i) {
		if (this.data[i] != ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF size_t svFindLastNot(StringView this, char ch) {
	for (size_t i = this.length - 1; i != SV_NPOS; -- i) {
		if (this.data[i] != ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF bool svContainsSubstring(StringView this, StringView substr) {
	if (substr.length > this.length)
		return false;

	for (size_t i = 0; i < this.length - (substr.length - 1); ++ i) {
		bool matched = true;
		for (size_t j = 0; j < substr.length; ++ j) {
			if (this.data[i + j] != substr.data[j]) {
				matched = false;
				break;
			}
		}

		if (matched)
			return true;
	}

	return false;
}

NOCH_DEF size_t svCountSubstrings(StringView this, StringView substr) {
	size_t count = 0;

	if (substr.length > this.length)
		return count;

	for (size_t i = 0; i < this.length - (substr.length - 1); ++ i) {
		bool matched = true;
		for (size_t j = 0; j < substr.length; ++ j) {
			if (this.data[i + j] != substr.data[j]) {
				matched = false;
				break;
			}
		}

		if (matched)
			++ count;
	}

	return count;
}

NOCH_DEF size_t svFindSubstring(StringView this, StringView substr) {
	if (substr.length > this.length)
		return SV_NPOS;

	for (size_t i = 0; i < this.length - (substr.length - 1); ++ i) {
		bool matched = true;
		for (size_t j = 0; j < substr.length; ++ j) {
			if (this.data[i + j] != substr.data[j]) {
				matched = false;
				break;
			}
		}

		if (matched)
			return i;
	}

	return SV_NPOS;
}

#ifdef __cplusplus
}
#endif
