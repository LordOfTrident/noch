#ifdef __cplusplus
extern "C" {
#endif

#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/error.c"

#include "json.h"

const char *jsonTypeToStringMap[JSON_TYPES_COUNT] = {
	"null",   /* JSON_NULL */
	"string", /* JSON_STRING */
	"float",  /* JSON_FLOAT */
	"int",    /* JSON_INT */
	"bool",   /* JSON_BOOL */
	"list",   /* JSON_LIST */
	"obj",    /* JSON_OBJ */
};

const char *jsonTypeToString(int type) {
	nochAssert(type < JSON_TYPES_COUNT && type >= 0);
	return jsonTypeToStringMap[type];
}

static Json jsonNullInstance;

NOCH_DEF Json *jsonNull(void) {
	return &jsonNullInstance;
}

static char *jsonStringDup(const char *str) {
	nochAssert(str != NULL);

	char *ptr = (char*)nochAlloc(strlen(str) + 1);
	if (ptr == NULL)
		NOCH_OUT_OF_MEM();

	strcpy(ptr, str);
	return ptr;
}

static JsonString *jsonNewString_(char *value) {
	JsonString *this = (JsonString*)nochAlloc(sizeof(JsonString));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.type = JSON_STRING;
	this->data      = value;
	this->length    = strlen(value);
	return this;
}

NOCH_DEF JsonString *jsonNewString(const char *value) {
	return jsonNewString_(jsonStringDup(value));
}

NOCH_DEF JsonFloat *jsonNewFloat(double value) {
	JsonFloat *this = (JsonFloat*)nochAlloc(sizeof(JsonFloat));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.type = JSON_FLOAT;
	this->value     = value;
	return this;
}

NOCH_DEF JsonInt *jsonNewInt(int64_t value) {
	JsonInt *this = (JsonInt*)nochAlloc(sizeof(JsonInt));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.type = JSON_INT;
	this->value     = value;
	return this;
}

NOCH_DEF JsonBool *jsonNewBool(bool value) {
	JsonBool *this = (JsonBool*)nochAlloc(sizeof(JsonBool));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.type = JSON_BOOL;
	this->value     = value;
	return this;
}

NOCH_DEF JsonList *jsonNewList(void) {
	JsonList *this = (JsonList*)nochAlloc(sizeof(JsonList));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.type = JSON_LIST;
	this->cap       = JSON_LIST_CHUNK_SIZE;
	this->size      = 0;
	this->buf       = (Json**)nochAlloc(this->cap * sizeof(Json*));
	if (this->buf == NULL)
		NOCH_OUT_OF_MEM();
	return this;
}

NOCH_DEF JsonObj *jsonNewObj(void) {
	JsonObj *this = (JsonObj*)nochAlloc(sizeof(JsonObj));
	if (this == NULL)
		NOCH_OUT_OF_MEM();

	this->base.type = JSON_OBJ;
	this->cap       = JSON_OBJ_CHUNK_SIZE;
	this->size      = 0;
	this->buckets   = (JsonObjBucket*)nochAlloc(this->cap * sizeof(JsonObjBucket));
	if (this->buckets == NULL)
		NOCH_OUT_OF_MEM();
	return this;
}

NOCH_DEF Json **jsonObjAt(JsonObj *this, const char *key) {
	nochAssert(this != NULL && key != NULL);

	for (size_t i = 0; i < this->size; ++ i) {
		if (this->buckets[i].key == NULL)
			continue;

		if (strcmp(this->buckets[i].key, key) == 0)
			return &this->buckets[i].value;
	}

	return NULL;
}

NOCH_DEF Json **jsonListAt(JsonList *this, size_t idx) {
	nochAssert(this != NULL);

	if (idx >= this->size)
		return NULL;
	else
		return this->buf + idx;
}

NOCH_DEF Json **jsonObjSet_(JsonObj *this, const char *key, Json *json) {
	nochAssert(this != NULL && key != NULL);

	for (size_t i = 0; i < this->size; ++ i) {
		if (this->buckets[i].key != NULL) {
			if (strcmp(this->buckets[i].key, key) != 0)
				continue;
		}

		if (this->buckets[i].value != NULL)
			jsonDestroy(this->buckets[i].value);

		if (this->buckets[i].key == NULL)
			this->buckets[i].key = jsonStringDup(key);

		this->buckets[i].value = json;
		return &this->buckets[i].value;
	}

	if (this->size >= this->cap) {
		this->cap    *= 2;
		this->buckets = (JsonObjBucket*)nochRealloc(this->buckets,
		                                            this->cap * sizeof(JsonObjBucket));
		if (this->buckets == NULL)
			NOCH_OUT_OF_MEM();
	}

	this->buckets[this->size].key   = jsonStringDup(key);
	this->buckets[this->size].value = json;
	return &this->buckets[this->size ++].value;
}

NOCH_DEF int jsonObjRemove(JsonObj *this, const char *key) {
	nochAssert(this != NULL && key != NULL);

	for (size_t i = 0; i < this->size; ++ i) {
		if (this->buckets[i].key == NULL)
			continue;

		if (strcmp(this->buckets[i].key, key) == 0) {
			if (this->buckets[i].value != NULL)
				jsonDestroy(this->buckets[i].value);

			free(this->buckets[i].key);
			this->buckets[i].key   = NULL;
			this->buckets[i].value = NULL;
			return 0;
		}
	}

	return -1;
}

NOCH_DEF Json **jsonListPush_(JsonList *this, Json *json) {
	nochAssert(this != NULL && json != NULL);

	if (this->size >= this->cap) {
		this->cap *= 2;
		this->buf  = (Json**)nochRealloc(this->buf, this->cap * sizeof(Json*));
		if (this->buf == NULL)
			NOCH_OUT_OF_MEM();
	}

	this->buf[this->size] = json;
	return this->buf + this->size ++;
}

NOCH_DEF void jsonListPop(JsonList *this) {
	nochAssert(this != NULL);
	nochAssert(this->size > 0);

	jsonDestroy(this->buf[-- this->size]);
}

NOCH_DEF void jsonDestroy_(Json *this) {
	nochAssert(this != NULL);

	switch (this->type) {
	case JSON_NULL: return;

	case JSON_STRING:
		nochFree(JSON_STRING(this)->data);
		break;

	case JSON_FLOAT: case JSON_INT: case JSON_BOOL: break;

	case JSON_LIST: {
		JsonList *list = JSON_LIST(this);

		for (size_t i = 0; i < list->size; ++ i)
			jsonDestroy(list->buf[i]);

		nochFree(list->buf);
	} break;

	case JSON_OBJ: {
		JsonObj *obj = JSON_OBJ(this);

		for (size_t i = 0; i < obj->size; ++ i) {
			if (obj->buckets[i].key == NULL)
				continue;

			if (obj->buckets[i].value != NULL)
				jsonDestroy(obj->buckets[i].value);

			nochFree(obj->buckets[i].key);
		}

		nochFree(obj->buckets);
	} break;

	default: nochAssert(0 && "Unknown JSON type");
	}

	nochFree(this);
}

typedef struct {
	FILE  *file;
	char  *buf;
	size_t size, cap;
} JsonOutputStream;

static void jsonOutput(JsonOutputStream *this, const char *str) {
	if (this->file != NULL)
		fprintf(this->file, "%s", str);
	else {
		size_t prevSize = this->size;
		this->size     += strlen(str);
		if (this->size + 1 >= this->cap) {
			do
				this->cap *= 2;
			while (this->size + 1 >= this->cap);

			this->buf = (char*)nochRealloc(this->buf, this->cap);
			if (this->buf == NULL)
				NOCH_OUT_OF_MEM();
		}

		strcpy(this->buf + prevSize, str);
	}
}

static void jsonOutputFmt(JsonOutputStream *this, const char *fmt, ...) {
	char    str[256];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	jsonOutput(this, str);
}

static void jsonOutputString(JsonOutputStream *this, JsonString *json) {
	jsonOutput(this, "\"");

	for (size_t i = 0; i < json->length; ++ i) {
		switch (json->data[i]) {
		case '"':  jsonOutput(this, "\\\""); break;
		case '\\': jsonOutput(this, "\\\\"); break;
		case '\b': jsonOutput(this, "\\b");  break;
		case '\f': jsonOutput(this, "\\f");  break;
		case '\n': jsonOutput(this, "\\n");  break;
		case '\r': jsonOutput(this, "\\r");  break;
		case '\t': jsonOutput(this, "\\t");  break;

		default:
			if ((unsigned char)json->data[i] < 32 || json->data[i] == 127)
				jsonOutputFmt(this, "\\u%04X", json->data[i]);
			else
				jsonOutputFmt(this, "%c", json->data[i]);
		}
	}

	jsonOutput(this, "\"");
}

static void jsonOutputFloat(JsonOutputStream *this, JsonFloat *json) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%.13f", json->value);

	bool   found = false;
	size_t i;
	for (i = 0; buf[i] != '\0'; ++ i) {
		if (buf[i] == '.' && !found)
			found = true;
	}

	if (found) {
		-- i;
		while (true) {
			if (buf[i] != '0') {
				if (buf[i] == '.') {
					buf[++ i] = '0';
					buf[++ i] = '\0';
				}

				break;
			}

			buf[i] = '\0';
			-- i;
		}
	}

	jsonOutput(this, buf);
}

static void jsonIndent(JsonOutputStream *this, size_t size) {
	if (this->file != NULL) {
		for (size_t i = 0; i < size; ++ i)
			fprintf(this->file, "\t");
	} else {
		size_t prevSize = this->size;
		this->size     += size;
		if (this->size + 1 >= this->cap) {
			do
				this->cap *= 2;
			while (this->size + 1 >= this->cap);

			this->buf = (char*)nochRealloc(this->buf, this->cap);
			if (this->buf == NULL)
				NOCH_OUT_OF_MEM();
		}

		memset(this->buf + prevSize, '\t', size);
		this->buf[this->size] = '\0';
	}
}

static void jsonOutputJson(JsonOutputStream *this, Json *json, size_t indentSize, bool comma) {
	nochAssert(this != NULL && json != NULL);

	switch (json->type) {
	case JSON_NULL:   jsonOutput(this, "null"); break;
	case JSON_STRING: jsonOutputString(this, JSON_STRING(json)); break;
	case JSON_FLOAT:  jsonOutputFloat(this,  JSON_FLOAT(json));  break;
	case JSON_INT:    jsonOutputFmt(this, "%lli", (long long)JSON_INT(json)->value);       break;
	case JSON_BOOL:   jsonOutputFmt(this, "%s", JSON_BOOL(json)->value? "true" : "false"); break;

	case JSON_LIST: {
		JsonList *list = JSON_LIST(json);

		++ indentSize;
		jsonOutput(this, "[");
		if (list->size > 0)
			jsonOutput(this, "\n");

		for (size_t i = 0; i < list->size; ++ i) {
			jsonIndent(this, indentSize);
			jsonOutputJson(this, list->buf[i], indentSize, i + 1 < list->size);
		}

		jsonIndent(this, -- indentSize);
		jsonOutput(this, "]");
	} break;

	case JSON_OBJ: {
		JsonObj *obj = JSON_OBJ(json);

		++ indentSize;
		jsonOutput(this, "{");
		if (obj->size > 0)
			jsonOutput(this, "\n");

		for (size_t i = 0; i < obj->size; ++ i) {
			jsonIndent(this, indentSize);
			jsonOutputFmt(this, "\"%s\": ", obj->buckets[i].key);
			jsonOutputJson(this, obj->buckets[i].value, indentSize, i + 1 < obj->size);
		}
		jsonIndent(this, -- indentSize);
		jsonOutput(this, "}");
	} break;

	default: nochAssert(0 && "Unknown JSON type");
	}

	if (comma)
		jsonOutput(this, ",");

	jsonOutput(this, "\n");
}

NOCH_DEF void jsonPrintF_(Json *this, FILE *file) {
	nochAssert(this != NULL && file != NULL);

	JsonOutputStream outputStream = {0};
	outputStream.file = file;

	jsonOutputJson(&outputStream, this, 0, false);
}

NOCH_DEF char *jsonStringify_(Json *this) {
	nochAssert(this != NULL);

	JsonOutputStream outputStream = {0};
	outputStream.cap = 256;
	outputStream.buf = (char*)nochAlloc(outputStream.cap);
	if (outputStream.buf == NULL)
		NOCH_OUT_OF_MEM();

	jsonOutputJson(&outputStream, this, 0, false);
	return outputStream.buf;
}

typedef struct {
	const char *in, *it, *bol; /* input, iterator, beginning of line */
	size_t      row;
	const char *path;
} JsonParser;

#define JSON_COL(THIS) ((THIS)->it - (THIS)->bol + 1)

static int jsonError(JsonParser *this, size_t row, size_t col, const char *fmt, ...) {
	char    str[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	if (this->path == NULL)
		return nochError("%lu:%lu: %s", (long unsigned)row, (long unsigned)col, str);
	else
		return nochError("%s:%lu:%lu: %s", this->path, (long unsigned)row, (long unsigned)col, str);
}

/* Not standard!!!! but very useful */
static int jsonSkipComment(JsonParser *this) {
	nochAssert(*this->it == '/' && this->it[1] == '*');

	size_t row = this->row, col = JSON_COL(this);

	this->it += 2;
	while (true) {
		if (*this->it == '\n') {
			++ this->row;
			this->bol = this->it + 1;
		} else if (*this->it == '\0')
			return jsonError(this, row, col, "Comment not terminated");
		else if (*this->it == '*' && this->it[1] == '/')
			break;

		++ this->it;
	}

	this->it += 2;
	return 0;
}

static int jsonSkipWhitespacesAndComments(JsonParser *this) {
	while (*this->it != '\0') {
		if (*this->it == '/' && this->it[1] == '*') {
			if (jsonSkipComment(this) != 0)
				return -1;
		} else if (!isspace(*this->it))
			break;

		if (*this->it == '\n') {
			++ this->row;
			this->bol = this->it + 1;
		}

		++ this->it;
	}
	return 0;
}

static int jsonParseHex4(JsonParser *this, uint16_t *ret) {
	nochAssert(ret != NULL);

	char buf[5] = {0};
	for (size_t i = 0; i < 4; ++ i) {
		if (!isxdigit(*this->it))
			return jsonError(this, this->row, JSON_COL(this),
			                 "Expected 4 hexadecimal digits, got \"%c\"", *this->it);

		buf[i] = *this->it ++;
	}

	*ret = strtol(buf, NULL, 16);
	return 0;
}

static int jsonParseUnicodeSequence(JsonParser *this, char *str, size_t *end) {
	size_t col = JSON_COL(this) - 1;
	++ this->it;

	uint32_t rune;
	uint16_t first, second;
	if (jsonParseHex4(this, &first) != 0)
		return -1;

	/* Surrogate pair */
	if (first >= 0xD800 && first <= 0xDBFF) {
		if (*this->it == '\0')
			return jsonError(this, this->row, col, "Expected a surrogate pair");

		if (this->it[0] != '\\' || this->it[1] != 'u')
			return jsonError(this, this->row, col, "Invalid unicode sequence");

		this->it += 2;
		if (jsonParseHex4(this, &second) != 0)
			return -1;

		if (second < 0xDC00 || second > 0xDFFF)
			return jsonError(this, this->row, col, "Invalid unicode sequence");

		rune = 0x10000 + (((first & 0x3FF) << 10) | (second & 0x3FF));
	} else
		rune = first;

	char out[5] = {0};

	if (rune <= 0x7F)
		out[0] = rune;
	else if (rune <= 0x7FF) {
		out[0] = (0xC0 | (rune >> 6));   /* 110xxxxx */
		out[1] = (0x80 | (rune & 0x3F)); /* 10xxxxxx */
	} else if (rune <= 0xFFFF) {
		out[0] = (0xE0 | (rune >> 12));       /* 1110xxxx */
		out[1] = (0x80 | (rune >> 6 & 0x3F)); /* 10xxxxxx */
		out[2] = (0x80 | (rune & 0x3F));      /* 10xxxxxx */
	} else if (rune <= 0x10FFFF) {
		out[0] = 0xF0 | (rune >> 18);        /* 11110xxx */
		out[1] = 0x80 | (rune >> 12 & 0x3F); /* 10xxxxxx */
		out[2] = 0x80 | (rune >> 6  & 0x3F); /* 10xxxxxx */
		out[3] = 0x80 | (rune & 0x3F);       /* 10xxxxxx */
	} else
		return jsonError(this, this->row, col, "Invalid unicode sequence");

	for (const char *it = out; *it != '\0'; ++ it)
		str[(*end) ++] = *it;

    return 0;
}

static char *jsonEscapeString(JsonParser *this) {
	nochAssert(*this->it == '"');

	size_t col = JSON_COL(this);

	const char *start  = ++ this->it;
	bool        escape = false;
	while (escape || *this->it != '"') {
		if (*this->it == '\0' || *this->it == '\n') {
			jsonError(this, this->row, col, "String not terminated");
			return NULL;
		}

		if (escape)
			escape = false;
		else if (*this->it == '\\')
			escape = true;

		++ this->it;
	}

	char *str = (char*)nochAlloc(this->it - start + 1);
	if (str == NULL)
		NOCH_OUT_OF_MEM();

	size_t end = 0;
	this->it   = start;
	while (escape || *this->it != '"') {
		if (escape) {
			escape = false;
			char escaped;

			switch (*this->it) {
			case '"': case '\\': case '/':
				escaped = *this->it;
				break;

			case 'b': escaped = '\b';   break;
			case 'f': escaped = '\f';   break;
			case 'n': escaped = '\n';   break;
			case 'r': escaped = '\r';   break;
			case 't': escaped = '\t';   break;
			case 'e': escaped = '\x1b'; break; /* Not standard!!!! but useful */

			case 'u':
				if (jsonParseUnicodeSequence(this, str, &end) != 0) {
					nochFree(str);
					return NULL;
				}
				continue;

			default:
				jsonError(this, this->row, JSON_COL(this),
				          "Unknown escape sequence \"\\%c\"", *this->it);
				nochFree(str);
				return NULL;
			}

			str[end ++] = escaped;
		} else if (*this->it == '\\')
			escape = true;
		else
			str[end ++] = *this->it;

		++ this->it;
	}

	++ this->it;
	str[end] = '\0';
	return str;
}

static Json *jsonParseAtom(JsonParser *this);

static Json *jsonParseObj(JsonParser *this) {
	nochAssert(*this->it == '{');

	size_t startRow = this->row, startCol = JSON_COL(this);

	JsonObj *obj = jsonNewObj();
	++ this->it;

	while (*this->it != '}') {
		if (jsonSkipWhitespacesAndComments(this) != 0)
			goto fail;

		if (*this->it != '"') {
			jsonError(this, this->row, JSON_COL(this), "Expected a key (string)");
			goto fail;
		}

		size_t col = JSON_COL(this);
		char  *key = jsonEscapeString(this);
		if (key == NULL)
			goto fail;

		size_t prevSize = obj->size;
		Json **ref = jsonObjSet(obj, key, NULL);
		if (obj->size == prevSize) {
			jsonError(this, this->row, col,
			          "Duplicate key \"%s\" in object", key);
			nochFree(key);
			goto fail;
		}
		nochFree(key);

		if (jsonSkipWhitespacesAndComments(this) != 0)
			goto fail;

		if (*this->it != ':') {
			jsonError(this, this->row, JSON_COL(this), "Expected a \":\"");
			goto fail;
		}
		++ this->it;

		Json *json = jsonParseAtom(this);
		if (json == NULL)
			goto fail;

		*ref = json;

		if (*this->it == ',')
			++ this->it;
		else if (*this->it != '}') {
			jsonError(this, this->row, JSON_COL(this),
			          "Expected a matching \"}\" (for \"{\" at %lu:%lu)",
			          (long unsigned)startRow, (long unsigned)startCol);
			goto fail;
		}
	}
	++ this->it;

	return (Json*)obj;

fail:
	jsonDestroy(obj);
	return NULL;
}

static Json *jsonParseList(JsonParser *this) {
	nochAssert(*this->it == '[');

	size_t startRow = this->row, startCol = JSON_COL(this);

	JsonList *list = jsonNewList();
	++ this->it;

	if (jsonSkipWhitespacesAndComments(this) != 0)
		goto fail;

	while (*this->it != ']') {
		Json *json = jsonParseAtom(this);
		if (json == NULL)
			goto fail;

		jsonListPush(list, json);

		if (*this->it == ',')
			++ this->it;
		else if (*this->it != ']') {
			jsonError(this, this->row, JSON_COL(this),
			          "Expected a matching \"]\" (for \"[\" at %lu:%lu)",
			          (long unsigned)startRow, (long unsigned)startCol);
			goto fail;
		}
	}
	++ this->it;

	return (Json*)list;

fail:
	jsonDestroy(list);
	return NULL;
}

static bool stringViewEqualsString(const char *view, size_t len, const char *str) {
	if (strncmp(view, str, len) != 0)
		return false;
	else
		return strlen(str) == len;
}

static Json *jsonParseId(JsonParser *this) {
	nochAssert(isalpha(*this->it));

	size_t col = JSON_COL(this);

	const char *start = this->it;
	while (isalnum(*this->it ++));

	size_t len = -- this->it - start;
	if (stringViewEqualsString(start, len, "null"))
		return jsonNull();
	else if (stringViewEqualsString(start, len, "true"))
		return (Json*)jsonNewBool(true);
	else if (stringViewEqualsString(start, len, "false"))
		return (Json*)jsonNewBool(false);

	jsonError(this, this->row, col, "Unknown identifier \"%.*s\"", (int)len, start);
	return NULL;
}

static Json *jsonParseString(JsonParser *this) {
	char *str = jsonEscapeString(this);
	if (str == NULL)
		return NULL;

	return (Json*)jsonNewString_(str);
}

static Json *jsonParseNumber(JsonParser *this) {
	nochAssert(isdigit(*this->it) || *this->it == '-');

	bool exponent = false, floatingPoint = false;

	const char *start = this->it;
	if (*this->it == '-') {
		++ this->it;
		if (!isdigit(*this->it)) {
			jsonError(this, this->row, JSON_COL(this), "Expected a number after \"-\"");
			return NULL;
		}
	}

	while (true) {
		if (*this->it == 'e') {
			if (exponent) {
				jsonError(this, this->row, JSON_COL(this), "Encountered exponent in number twice");
				return NULL;
			}

			exponent = true;
			if (this->it[1] == '+' || this->it[1] == '-')
				++ this->it;

			if (!isdigit(this->it[1])) {
				jsonError(this, this->row, JSON_COL(this), "expected a digit after exponent");
				return NULL;
			}
		} else if (*this->it == '.') {
			if (floatingPoint) {
				jsonError(this, this->row, JSON_COL(this),
				          "Encountered floating point in number twice");
				return NULL;
			} else if (exponent) {
				jsonError(this, this->row, JSON_COL(this), "Unexpected floating point in exponent");
				return NULL;
			}

			floatingPoint = true;
			if (!isdigit(this->it[1])) {
				jsonError(this, this->row, JSON_COL(this), "expected a digit after floating point");
				return NULL;
			}
		} else if (*this->it == '-' || isalpha(*this->it)) {
			jsonError(this, this->row, JSON_COL(this),
			          "Unexpected character \"%c\" in number", *this->it);
			return NULL;
		} else if (!isdigit(*this->it))
			break;

		++ this->it;
	}

	if (exponent || floatingPoint)
		return (Json*)jsonNewFloat(atof(start));
	else
		return (Json*)jsonNewInt((int16_t)atoll(start));
}

static Json *jsonParseAtom(JsonParser *this) {
	if (jsonSkipWhitespacesAndComments(this) != 0)
		return NULL;

	Json *parsed;
	switch (*this->it) {
	case '\0':
		jsonError(this, this->row, JSON_COL(this), "Unexpected end of input");
		return NULL;

	case '{': parsed = jsonParseObj(this);    break;
	case '[': parsed = jsonParseList(this);   break;
	case '"': parsed = jsonParseString(this); break;

	default:
		if (isdigit(*this->it) || *this->it == '-')
			parsed = jsonParseNumber(this);
		else if (isalpha(*this->it))
			parsed = jsonParseId(this);
		else {
			jsonError(this, this->row, JSON_COL(this), "Unexpected character \"%c\"", *this->it);
			return NULL;
		}
	}

	if (jsonSkipWhitespacesAndComments(this) != 0) {
		jsonDestroy(parsed);
		return NULL;
	}

	return parsed;
}

static Json *jsonParse(const char *str, const char *path) {
	JsonParser parser = {0};
	parser.path = path;
	parser.in   = str;
	parser.it   = str;
	parser.bol  = str;
	parser.row  = 1;

	Json *json = jsonParseAtom(&parser);
	if (json != NULL && *parser.it != '\0') {
		jsonError(&parser, parser.row, JSON_COL(&parser),
		          "Unexpected character \"%c\" after end of JSON data", *parser.it);
		jsonDestroy(json);
		return NULL;
	}

	return json;
}

NOCH_DEF Json *jsonFromFile(const char *path) {
	nochAssert(path != NULL);

	FILE *file = fopen(path, "r");
	if (file == NULL) {
		nochError("%s: Failed to open file", path);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	size_t size = (size_t)ftell(file);
	rewind(file);

	if (size == 0) {
		nochError("%s: File is empty", path);
		return NULL;
	}

	char *str = (char*)nochAlloc(size + 1);
	if (str == NULL)
		NOCH_OUT_OF_MEM();

	if (fread(str, size, 1, file) <= 0) {
		nochError("%s: Failed to read file", path);
		return NULL;
	}

	str[size] = '\0';
	fclose(file);

	Json *json = jsonParse(str, path);
	free(str);
	return json;
}

NOCH_DEF Json *jsonFromString(const char *str) {
	nochAssert(str != NULL);
	return jsonParse(str, NULL);
}

#undef JSON_COL

#ifdef __cplusplus
}
#endif
