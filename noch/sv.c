#ifdef __cplusplus
extern "C" {
#endif

#include "internal/assert.h"

#include "sv.h"

#define SV_NPOS        (size_t)-1
#define SV_WHITESPACES " \f\n\r\t\v"

NOCH_DEF sv_t sv_new(const char *cstr, size_t len) {
	sv_t this;
	this.len = len;
	this.ptr = cstr;
	return this;
}

NOCH_DEF sv_t sv_cstr(const char *cstr) {
	sv_t this;
	this.len = strlen(cstr);
	this.ptr = cstr;
	return this;
}

NOCH_DEF char sv_at(sv_t this, size_t idx) {
	NOCH_ASSERT(idx < this.len);
	return this.ptr[idx];
}

NOCH_DEF bool sv_is_null(sv_t this) {
	return this.ptr == NULL;
}

NOCH_DEF bool sv_equals(sv_t this, sv_t to) {
	if (this.len != to.len)
		return false;

	for (size_t i = 0; i < this.len; ++ i) {
		if (this.ptr[i] != to.ptr[i])
			return false;
	}

	return true;
}

NOCH_DEF bool sv_has_prefix(sv_t this, sv_t prefix) {
	if (this.len < prefix.len)
		return false;

	for (size_t i = 0; i < prefix.len; ++ i) {
		if (this.ptr[i] != prefix.ptr[i])
			return false;
	}

	return true;
}

NOCH_DEF bool sv_has_suffix(sv_t this, sv_t suffix) {
	if (this.len < suffix.len)
		return false;

	for (size_t i = this.len - suffix.len; i < this.len; ++ i) {
		if (this.ptr[i] != suffix.ptr[i])
			return false;
	}

	return true;
}

NOCH_DEF sv_t sv_substr(sv_t this, size_t start, size_t len) {
	if (len != SV_NPOS) {
		if (start + len > this.len)
			return SV_NULL;
	} else if (start > this.len)
		return SV_NULL;

	sv_t substr;
	substr.ptr = this.ptr + start;
	substr.len = len == SV_NPOS? this.len - start : len;
	return substr;
}

NOCH_DEF sv_t sv_trim_front(sv_t this, const char *chs) {
	sv_t matches = sv_cstr(chs);
	sv_t trimmed = this;
	for (size_t i = 0; i < this.len; ++ i) {
		if (!sv_contains(matches, this.ptr[i]))
			break;

		++ trimmed.ptr;
		-- trimmed.len;
	}

	return trimmed;
}

NOCH_DEF sv_t sv_trim_back(sv_t this, const char *chs) {
	sv_t matches = sv_cstr(chs);
	sv_t trimmed = this;
	for (size_t i = this.len - 1; i != SV_NPOS; -- i) {
		if (!sv_contains(matches, this.ptr[i]))
			break;

		-- trimmed.len;
	}

	return trimmed;
}

NOCH_DEF sv_t sv_trim(sv_t this, const char *chs) {
	this = sv_trim_front(this, chs);
	return sv_trim_back (this, chs);
}

NOCH_DEF bool sv_contains(sv_t this, char ch) {
	for (size_t i = 0; i < this.len; ++ i) {
		if (this.ptr[i] == ch)
			return true;
	}

	return false;
}

NOCH_DEF size_t sv_count(sv_t this, char ch) {
	size_t count = 0;
	for (size_t i = 0; i < this.len; ++ i) {
		if (this.ptr[i] == ch)
			++ count;
	}

	return count;
}

NOCH_DEF size_t sv_find_first(sv_t this, char ch) {
	for (size_t i = 0; i < this.len; ++ i) {
		if (this.ptr[i] == ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF size_t sv_find_last(sv_t this, char ch) {
	for (size_t i = this.len - 1; i != SV_NPOS; -- i) {
		if (this.ptr[i] == ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF size_t sv_find_first_not(sv_t this, char ch) {
	for (size_t i = 0; i < this.len; ++ i) {
		if (this.ptr[i] != ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF size_t sv_find_last_not(sv_t this, char ch) {
	for (size_t i = this.len - 1; i != SV_NPOS; -- i) {
		if (this.ptr[i] != ch)
			return i;
	}

	return SV_NPOS;
}

NOCH_DEF bool sv_contains_substr(sv_t this, sv_t substr) {
	if (substr.len > this.len)
		return false;

	for (size_t i = 0; i < this.len - (substr.len - 1); ++ i) {
		bool matched = true;
		for (size_t j = 0; j < substr.len; ++ j) {
			if (this.ptr[i + j] != substr.ptr[j]) {
				matched = false;
				break;
			}
		}

		if (matched)
			return true;
	}

	return false;
}

NOCH_DEF size_t sv_count_substr(sv_t this, sv_t substr) {
	size_t count = 0;

	if (substr.len > this.len)
		return count;

	for (size_t i = 0; i < this.len - (substr.len - 1); ++ i) {
		bool matched = true;
		for (size_t j = 0; j < substr.len; ++ j) {
			if (this.ptr[i + j] != substr.ptr[j]) {
				matched = false;
				break;
			}
		}

		if (matched)
			++ count;
	}

	return count;
}

NOCH_DEF size_t sv_find_substr(sv_t this, sv_t substr) {
	if (substr.len > this.len)
		return SV_NPOS;

	for (size_t i = 0; i < this.len - (substr.len - 1); ++ i) {
		bool matched = true;
		for (size_t j = 0; j < substr.len; ++ j) {
			if (this.ptr[i + j] != substr.ptr[j]) {
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
