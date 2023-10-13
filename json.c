#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"
#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "json.h"

#define json_null_instance        NOCH_PRIVATE(json_null_instance)
#define json_new                  NOCH_PRIVATE(json_new)
#define json_strdup               NOCH_PRIVATE(json_strdup)
#define jstream_t                 NOCH_PRIVATE(jstream_t)
#define jstream_print             NOCH_PRIVATE(jstream_print)
#define jstream_printf            NOCH_PRIVATE(jstream_printf)
#define jstream_print_str         NOCH_PRIVATE(jstream_print_str)
#define jstream_print_float       NOCH_PRIVATE(jstream_print_float)
#define jstream_indent            NOCH_PRIVATE(jstream_indent)
#define jstream_print_json        NOCH_PRIVATE(jstream_print_json)
#define json_tok_t                NOCH_PRIVATE(json_tok_t)
#define jparser_t                 NOCH_PRIVATE(jparser_t)
#define jparser_data_clear        NOCH_PRIVATE(jparser_data_clear)
#define jparser_data_add          NOCH_PRIVATE(jparser_data_add)
#define jparser_skip_cmnt         NOCH_PRIVATE(jparser_skip_cmnt)
#define jparser_skip_ws_and_cmnts NOCH_PRIVATE(jparser_skip_ws_and_cmnts)
#define jparser_tok_start_here    NOCH_PRIVATE(jparser_tok_start_here)
#define jparser_tok_single        NOCH_PRIVATE(jparser_tok_single)
#define jparser_get_hex4          NOCH_PRIVATE(jparser_get_hex4)
#define jparser_unicode_seq       NOCH_PRIVATE(jparser_unicode_seq)
#define jparser_tok_str           NOCH_PRIVATE(jparser_tok_str)
#define jparser_tok_num           NOCH_PRIVATE(jparser_tok_num)
#define jparser_tok_id            NOCH_PRIVATE(jparser_tok_id)
#define jparser_advance           NOCH_PRIVATE(jparser_advance)
#define jparser_parse             NOCH_PRIVATE(jparser_parse)
#define jparser_parse_obj         NOCH_PRIVATE(jparser_parse_obj)
#define jparser_parse_list        NOCH_PRIVATE(jparser_parse_list)

const char *json_type_to_str_map[JSON_TYPES_COUNT] = {
	"null", /* JSON_NULL */

	"string", /* JSON_STR */
	"float",  /* JSON_FLOAT */
	"int64",  /* JSON_INT64 */
	"bool",   /* JSON_BOOL */

	"list", /* JSON_LIST */
	"obj",  /* JSON_OBJ */
};

const char *json_type_to_str(json_type_t type) {
	NOCH_ASSERT(type < JSON_TYPES_COUNT && type >= 0);
	return json_type_to_str_map[type];
}

static json_t json_null_instance;

static json_t *json_new(json_type_t type) {
	json_t *json = (json_t*)NOCH_ALLOC(sizeof(json_t));
	NOCH_CHECK_ALLOC(json);

	memset(json, 0, sizeof(json_t));
	json->type = type;
	return json;
}

NOCH_DEF json_t *json_null(void) {
	return &json_null_instance;
}

static char *json_strdup(const char *str) {
	char *duped = (char*)NOCH_ALLOC(strlen(str) + 1);
	NOCH_CHECK_ALLOC(duped);

	strcpy(duped, str);
	return duped;
}

NOCH_DEF json_t *json_new_str(const char *str) {
	NOCH_ASSERT(str != NULL);

	json_t *json = json_new(JSON_STR);
	json->as.str.len = strlen(str);
	json->as.str.buf = json_strdup(str);

	return json;
}

NOCH_DEF json_t *json_new_float(double float_) {
	json_t *json = json_new(JSON_FLOAT);
	json->as.float_ = float_;
	return json;
}

NOCH_DEF json_t *json_new_int64(int64_t int64) {
	json_t *json = json_new(JSON_INT64);
	json->as.int64 = int64;
	return json;
}

NOCH_DEF json_t *json_new_bool(bool bool_) {
	json_t *json = json_new(JSON_BOOL);
	json->as.bool_ = bool_;
	return json;
}

NOCH_DEF json_t *json_new_list(void) {
	json_t *json = json_new(JSON_LIST);
	json->as.list.cap = JSON_LIST_CHUNK_SIZE;
	json->as.list.buf = (json_t**)NOCH_ALLOC(json->as.list.cap * sizeof(json_t*));
	NOCH_CHECK_ALLOC(json->as.list.buf);
	return json;
}

NOCH_DEF json_t *json_new_obj(void) {
	json_t *json = json_new(JSON_OBJ);
	json->as.obj.cap  = JSON_OBJ_CHUNK_SIZE;
	json->as.obj.keys = (char**)NOCH_ALLOC(json->as.obj.cap * sizeof(char*));
	NOCH_CHECK_ALLOC(json->as.obj.keys);

	json->as.obj.vals = (json_t**)NOCH_ALLOC(json->as.obj.cap * sizeof(json_t*));
	NOCH_CHECK_ALLOC(json->as.obj.vals);

	return json;
}

NOCH_DEF void json_destroy(json_t *json) {
	NOCH_ASSERT(json != NULL);

	switch (json->type) {
	case JSON_NULL:  return;
	case JSON_STR:   NOCH_FREE(json->as.str.buf);
	case JSON_FLOAT: break;
	case JSON_INT64: break;
	case JSON_BOOL:  break;

	case JSON_LIST:
		for (size_t i = 0; i < json->as.list.size; ++ i)
			json_destroy(json->as.list.buf[i]);

		NOCH_FREE(json->as.list.buf);
		break;

	case JSON_OBJ:
		for (size_t i = 0; i < json->as.obj.size; ++ i) {
			NOCH_FREE(json->as.obj.keys[i]);
			json_destroy(json->as.obj.vals[i]);
		}

		NOCH_FREE(json->as.obj.keys);
		NOCH_FREE(json->as.obj.vals);
		break;

	default: NOCH_ASSERT(0 && "Unknown json type");
	}

	NOCH_FREE(json);
}

NOCH_DEF int json_obj_add(json_t *obj, const char *key, json_t *json) {
	NOCH_ASSERT(obj != NULL);
	NOCH_ASSERT(obj->type == JSON_OBJ);
	NOCH_ASSERT(key  != NULL);
	NOCH_ASSERT(json != NULL);

	if (obj->as.obj.size >= obj->as.obj.cap) {
		obj->as.obj.cap *= 2;
		obj->as.obj.keys = (char**)NOCH_REALLOC(obj->as.obj.keys, obj->as.obj.cap * sizeof(char*));
		NOCH_CHECK_ALLOC(obj->as.obj.keys);

		obj->as.obj.vals = (json_t**)NOCH_REALLOC(obj->as.obj.vals,
		                                          obj->as.obj.cap * sizeof(json_t*));
		NOCH_CHECK_ALLOC(obj->as.obj.vals);
	}

	char **new_key = &obj->as.obj.keys[obj->as.obj.size];
	*new_key = (char*)NOCH_ALLOC(strlen(key) + 1);
	NOCH_CHECK_ALLOC(*new_key);

	strcpy(*new_key, key);

	obj->as.obj.vals[obj->as.obj.size ++] = json;
	return 0;
}

NOCH_DEF int json_list_add(json_t *list, json_t *json) {
	NOCH_ASSERT(list != NULL);
	NOCH_ASSERT(list->type == JSON_LIST);
	NOCH_ASSERT(json != NULL);

	if (list->as.list.size >= list->as.list.cap) {
		list->as.list.cap *= 2;
		list->as.list.buf = (json_t**)NOCH_REALLOC(list->as.list.buf,
		                                           list->as.list.cap * sizeof(json_t*));
		NOCH_CHECK_ALLOC(list->as.list.buf);
	}

	list->as.list.buf[list->as.list.size ++] = json;
	return 0;
}

NOCH_DEF json_t *json_obj_at(json_t *obj, const char *key) {
	NOCH_ASSERT(obj != NULL);
	NOCH_ASSERT(obj->type == JSON_OBJ);
	NOCH_ASSERT(key != NULL);

	for (size_t i = 0; i < obj->as.obj.size; ++ i) {
		if (strcmp(obj->as.obj.keys[i], key) == 0)
			return obj->as.obj.vals[i];
	}

	return NULL;
}

NOCH_DEF json_t *json_list_at(json_t *list, size_t idx) {
	NOCH_ASSERT(list != NULL);
	NOCH_ASSERT(list->type == JSON_LIST);

	if (idx >= list->as.list.size)
		return NULL;
	else
		return list->as.list.buf[idx];
}

/* JSON stream structure */
typedef struct {
	FILE  *file;
	char  *buf;
	size_t size, cap;
} jstream_t;

static int jstream_print(jstream_t *this, const char *str) {
	if (this->file != NULL)
		fputs(str, this->file);
	else {
		size_t prev = this->size;
		this->size += strlen(str);
		if (this->size + 1 >= this->cap) {
			do
				this->cap *= 2;
			while (this->size + 1 >= this->cap);

			this->buf = (char*)NOCH_REALLOC(this->buf, this->cap);
			NOCH_CHECK_ALLOC(this->buf);
		}

		strcpy(this->buf + prev, str);
	}

	return 0;
}

static int jstream_printf(jstream_t *this, const char *fmt, ...) {
	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	return jstream_print(this, str);
}

#define JSTREAM_MUST(X) \
	do {                \
		if ((X) != 0)   \
			return -1;  \
	} while (0)

#define JSTREAM_PRINT(S, STR)  JSTREAM_MUST(jstream_print(S, STR))
#define JSTREAM_PRINTF(S, ...) JSTREAM_MUST(jstream_printf(S, __VA_ARGS__))

static int jstream_print_str(jstream_t *this, json_str_t *str) {
	JSTREAM_PRINT(this, "\"");

	for (size_t i = 0; i < str->len; ++ i) {
		switch (str->buf[i]) {
		case '"':  JSTREAM_PRINT(this, "\\\""); break;
		case '\\': JSTREAM_PRINT(this, "\\\\"); break;
		case '\b': JSTREAM_PRINT(this, "\\b");  break;
		case '\f': JSTREAM_PRINT(this, "\\f");  break;
		case '\n': JSTREAM_PRINT(this, "\\n");  break;
		case '\r': JSTREAM_PRINT(this, "\\r");  break;
		case '\t': JSTREAM_PRINT(this, "\\t");  break;

		default:
			if ((unsigned char)str->buf[i] < 32 || str->buf[i] == 127)
				JSTREAM_PRINTF(this, "\\u%04X", str->buf[i]);
			else
				JSTREAM_PRINTF(this, "%c", str->buf[i]);
		}
	}

	JSTREAM_PRINT(this, "\"");
	return 0;
}

static int jstream_print_float(jstream_t *this, float num) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%f", num);

	bool   found = false;
	size_t i;
	for (i = 0; buf[i] != '\0'; ++ i) {
		if (buf[i] == '.' && !found)
			found = true;
	}
	if (!found) {
		JSTREAM_PRINT(this, buf);
		return 0;
	} else
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

	JSTREAM_PRINT(this, buf);
	return 0;
}

static int jstream_indent(jstream_t *this, size_t nest) {
	if (this->file != NULL) {
		for (size_t i = 0; i < nest; ++ i)
			fputc('\t', this->file);
	} else {
		size_t prev = this->size;
		this->size += nest;
		if (this->size + 1 >= this->cap) {
			do
				this->cap *= 2;
			while (this->size + 1 >= this->cap);

			this->buf = (char*)NOCH_REALLOC(this->buf, this->cap);
			NOCH_CHECK_ALLOC(this->buf);
		}

		memset(this->buf + prev, '\t', nest);
		this->buf[this->size] = '\0';
	}

	return 0;
}

#define JSTREAM_INDENT(S, NEST) JSTREAM_MUST(jstream_indent(S, NEST))

static int jstream_print_json(jstream_t *this, json_t *json, size_t nest, bool comma) {
	NOCH_ASSERT(json != NULL);

	switch (json->type) {
	case JSON_NULL:  JSTREAM_PRINT(this, "null"); break;
	case JSON_STR:   JSTREAM_MUST(jstream_print_str  (this, &json->as.str));       break;
	case JSON_FLOAT: JSTREAM_MUST(jstream_print_float(this, json->as.float_));     break;
	case JSON_INT64: JSTREAM_PRINTF(this, "%lli", (long long)json->as.int64);      break;
	case JSON_BOOL:  JSTREAM_PRINTF(this, "%s", json->as.bool_? "true" : "false"); break;

	case JSON_LIST:
		++ nest;

		JSTREAM_PRINT(this, "[");
		if (json->as.list.size > 0)
			JSTREAM_PRINT(this, "\n");

		for (size_t i = 0; i < json->as.list.size; ++ i) {
			JSTREAM_INDENT(this, nest);
			JSTREAM_MUST(jstream_print_json(this, json->as.list.buf[i], nest,
			                                i + 1 < json->as.list.size));
		}
		JSTREAM_INDENT(this, -- nest);
		JSTREAM_PRINT(this, "]");
		break;

	case JSON_OBJ:
		++ nest;

		JSTREAM_PRINT(this, "{");
		if (json->as.obj.size > 0)
			JSTREAM_PRINT(this, "\n");

		for (size_t i = 0; i < json->as.obj.size; ++ i) {
			JSTREAM_INDENT(this, nest);
			JSTREAM_PRINTF(this, "\"%s\": ", json->as.obj.keys[i]);
			JSTREAM_MUST(jstream_print_json(this, json->as.obj.vals[i], nest,
			                                i + 1 < json->as.obj.size));
		}
		JSTREAM_INDENT(this, -- nest);
		JSTREAM_PRINT(this, "}");
		break;

	default: NOCH_ASSERT(0 && "Unknown json type");
	}

	if (comma)
		JSTREAM_PRINT(this, ",");

	JSTREAM_PRINT(this, "\n");
	return 0;
}

#undef JSTREAM_INDENT
#undef JSTREAM_PRINT
#undef JSTREAM_PRINTF
#undef JSTREAM_MUST

NOCH_DEF void json_fprint(json_t *json, FILE *file) {
	NOCH_ASSERT(file != NULL);

	jstream_t stream = {0};
	stream.file = file;

	jstream_print_json(&stream, json, 0, false);
}

NOCH_DEF char *json_stringify(json_t *json) {
	jstream_t stream = {0};
	stream.cap = JSON_STRINGIFY_CHUNK_SIZE;
	stream.buf = (char*)NOCH_ALLOC(stream.cap);
	NOCH_CHECK_ALLOC(stream.buf);

	return jstream_print_json(&stream, json, 0, false) == 0? stream.buf : NULL;
}

typedef enum {
	JSON_TOK_EOI,

	JSON_TOK_STR,
	JSON_TOK_INT,
	JSON_TOK_FLOAT,
	JSON_TOK_BOOL,
	JSON_TOK_NULL,

	JSON_TOK_COMMA,
	JSON_TOK_COLON,

	JSON_TOK_LCURLY,
	JSON_TOK_RCURLY,

	JSON_TOK_LSQUARE,
	JSON_TOK_RSQUARE,
} json_tok_t;

/* JSON parser structure */
typedef struct {
	const char *it, *bol; /* iterator, beginning of line */
	size_t      row;

	json_tok_t tok;
	size_t     tok_row, tok_col;

	char  *data;
	size_t data_cap, data_size;

	size_t err_col, err_row;
} jparser_t;

#define JPARSER_ERR(P, MSG, ROW, COL) \
	((P)->err_row = ROW,              \
	 (P)->err_col = COL,              \
	 NOCH_PARSER_ERR(MSG))

#define JPARSER_COL(P) ((size_t)(P)->it - (size_t)(P)->bol + 1)

static void jparser_data_clear(jparser_t *this) {
	this->data[0]   = '\0';
	this->data_size = 0;
}

static int jparser_data_add(jparser_t *this, char ch) {
	if (this->data_size + 1 >= this->data_cap) {
		this->data_cap *= 2;
		this->data = (char*)NOCH_REALLOC(this->data, this->data_cap);
		NOCH_CHECK_ALLOC(this->data);
	}

	this->data[this->data_size ++] = ch;
	this->data[this->data_size]    = '\0';
	return 0;
}

/* Not standard!!!! but very useful */
static int jparser_skip_cmnt(jparser_t *this) {
	NOCH_ASSERT(*this->it == '/' && this->it[1] == '*');

	size_t row = this->row, col = JPARSER_COL(this);

	this->it += 2;
	bool prev_asterisk = false;
	while (!prev_asterisk || *this->it != '/') {
		prev_asterisk = *this->it == '*';

		if (*this->it == '\n') {
			++ this->row;
			this->bol = this->it + 1;
		} else if (*this->it == '\0')
			return JPARSER_ERR(this, "Comment not terminated", row, col);

		++ this->it;
	}

	return 0;
}

static int jparser_skip_ws_and_cmnts(jparser_t *this) {
	while (*this->it != '\0') {
		if (*this->it == '/' && this->it[1] == '*') {
			if (jparser_skip_cmnt(this) != 0)
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

static void jparser_tok_start_here(jparser_t *this) {
	this->tok_row = this->row;
	this->tok_col = JPARSER_COL(this);
}

static int jparser_tok_single(jparser_t *this, json_tok_t tok) {
	jparser_data_clear(this);
	jparser_tok_start_here(this);
	this->tok = tok;

	++ this->it;
	return 0;
}

static int jparser_get_hex4(jparser_t *this, uint16_t *ret) {
	NOCH_ASSERT(ret != NULL);

	char buf[5] = {0};
	for (size_t i = 0; i < 4; ++ i) {
		if (!isxdigit(*this->it))
			return JPARSER_ERR(this, "Expected 4 hexadecimal digits", this->row, JPARSER_COL(this));

		buf[i] = *this->it ++;
	}

	*ret = strtol(buf, NULL, 16);
	return 0;
}

static int jparser_unicode_seq(jparser_t *this) {
	size_t col = JPARSER_COL(this) - 1;
	++ this->it;

	uint32_t rune;
	uint16_t first, second;
	if (jparser_get_hex4(this, &first) != 0)
		return -1;

	/* Surrogate pair */
	if (first >= 0xD800 && first <= 0xDBFF) {
		if (*this->it == '\0')
			return JPARSER_ERR(this, "Expected a surrogate pair", this->tok_row, this->tok_col);

		if (this->it[0] != '\\' || this->it[1] != 'u')
			return JPARSER_ERR(this, "Invalid unicode sequence", this->row, col);

		this->it += 2;
		if (jparser_get_hex4(this, &second) != 0)
			return -1;

		if (second < 0xDC00 || second > 0xDFFF)
			return JPARSER_ERR(this, "Invalid unicode sequence", this->row, col);

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
		return JPARSER_ERR(this, "Invalid unicode sequence", this->row, col);

	for (const char *it = out; *it != '\0'; ++ it) {
		if (jparser_data_add(this, *it) != 0)
			return -1;
	}

    return 0;
}

static int jparser_tok_str(jparser_t *this) {
	NOCH_ASSERT(*this->it == '"');

	jparser_data_clear(this);
	jparser_tok_start_here(this);
	this->tok = JSON_TOK_STR;

	++ this->it;
	bool escape = false;
	while (escape || *this->it != '"') {
		if (*this->it == '\0' || *this->it  == '\n')
			return JPARSER_ERR(this, "String not terminated", this->tok_row, this->tok_col);
		else if (escape) {
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
				if (jparser_unicode_seq(this) != 0)
					return -1;
				continue;

			default: return JPARSER_ERR(this, "Unknown escape sequence",
			                            this->row, JPARSER_COL(this) - 1);
			}

			if (jparser_data_add(this, escaped) != 0)
				return -1;
		} else if (*this->it == '\\') {
			escape = true;
		} else {
			if (jparser_data_add(this, *this->it) != 0)
				return -1;
		}

		++ this->it;
	}

	++ this->it;
	return 0;
}

static int jparser_tok_num(jparser_t *this) {
	NOCH_ASSERT(isdigit(*this->it) || *this->it == '-');

	jparser_data_clear(this);
	jparser_tok_start_here(this);

	bool exponent = false, fpoint = false;

	if (*this->it == '-') {
		if (jparser_data_add(this, *this->it ++) != 0)
			return -1;

		if (!isdigit(*this->it))
			return JPARSER_ERR(this, "Expected a number", this->row, JPARSER_COL(this));
	}

	while (true) {
		if (*this->it == 'e') {
			if (exponent)
				return JPARSER_ERR(this, "Encountered exponent in number twice",
				                   this->row, JPARSER_COL(this));
			else
				exponent = true;

			if (this->it[1] == '+' || this->it[1] == '-')
				++ this->it;

			if (!isdigit(this->it[1]))
				return JPARSER_ERR(this, "Expected a number", this->row, JPARSER_COL(this) + 1);
		} else if (*this->it == '.') {
			if (exponent)
				return JPARSER_ERR(this, "Unexpected floating point in exponent",
				                   this->row, JPARSER_COL(this));
			else if (fpoint)
				return JPARSER_ERR(this, "Encountered a floating point in number twice",
				                   this->row, JPARSER_COL(this));
			else {
				fpoint = true;

				if (!isdigit(this->it[1]))
					return JPARSER_ERR(this, "Expected a number", this->row, JPARSER_COL(this) + 1);
			}
		} else if (*this->it == '-' || isalpha(*this->it))
			return JPARSER_ERR(this, "Unexpected character in number",
			                   this->row, JPARSER_COL(this));
		else if (!isdigit(*this->it))
			break;

		if (jparser_data_add(this, *this->it ++) != 0)
			return -1;
	}

	this->tok = exponent || fpoint? JSON_TOK_FLOAT : JSON_TOK_INT;
	return 0;
}

static int jparser_tok_id(jparser_t *this) {
	NOCH_ASSERT(isalpha(*this->it));

	jparser_data_clear(this);
	jparser_tok_start_here(this);

	while (isalnum(*this->it)) {
		if (jparser_data_add(this, *this->it) != 0)
			return -1;

		++ this->it;
	}

	if (strcmp(this->data, "null") == 0)
		this->tok = JSON_TOK_NULL;
	else if (strcmp(this->data, "true") == 0 || strcmp(this->data, "false") == 0)
		this->tok = JSON_TOK_BOOL;
	else
		return JPARSER_ERR(this, "Unknown identifier", this->tok_row, this->tok_col);

	return 0;
}

static int jparser_advance(jparser_t *this) {
	if (jparser_skip_ws_and_cmnts(this) != 0)
		return -1;
	else if (*this->it == '\0') {
		this->tok     = JSON_TOK_EOI;
		this->tok_row = this->row;
		this->tok_col = JPARSER_COL(this);
		return 0;
	}

	switch (*this->it) {
	case '"': return jparser_tok_str(this);
	case '{': return jparser_tok_single(this, JSON_TOK_LCURLY);
	case '}': return jparser_tok_single(this, JSON_TOK_RCURLY);
	case '[': return jparser_tok_single(this, JSON_TOK_LSQUARE);
	case ']': return jparser_tok_single(this, JSON_TOK_RSQUARE);
	case ',': return jparser_tok_single(this, JSON_TOK_COMMA);
	case ':': return jparser_tok_single(this, JSON_TOK_COLON);

	default:
		if (isdigit(*this->it) || *this->it == '-')
			return jparser_tok_num(this);
		else if (isalpha(*this->it))
			return jparser_tok_id(this);
		else
			return JPARSER_ERR(this, "Unexpected character", this->row, JPARSER_COL(this));
	}
}

#undef JPARSER_COL

#define JPARSER_MUST_ADVANCE(P)      \
	do {                             \
		if (jparser_advance(P) != 0) \
			goto fail;               \
	} while (0)

#define JPARSER_TOK_MUST_BE(P, TOK, MSG)                     \
	do {                                                     \
		if ((P)->tok != TOK) {                               \
			JPARSER_ERR(P, MSG, (P)->tok_row, (P)->tok_col); \
			goto fail;                                       \
		}                                                    \
	} while (0)

static json_t *jparser_parse(jparser_t *this);

static json_t *jparser_parse_obj(jparser_t *this) {
	json_t *obj = json_new_obj();
	JPARSER_MUST_ADVANCE(this);

	while (this->tok != JSON_TOK_RCURLY) {
		JPARSER_TOK_MUST_BE(this, JSON_TOK_STR, "Expected a key string");
		char *key = json_strdup(this->data);

		JPARSER_MUST_ADVANCE(this);
		JPARSER_TOK_MUST_BE(this, JSON_TOK_COLON, "Expected a \":\"");

		JPARSER_MUST_ADVANCE(this);
		json_t *json = jparser_parse(this);
		if (json == NULL)
			goto fail;

		if (json_obj_add(obj, key, json) != 0) {
			json_destroy(json);
			goto fail;
		}

		NOCH_FREE(key);

		JPARSER_MUST_ADVANCE(this);
		if (this->tok == JSON_TOK_COMMA)
			JPARSER_MUST_ADVANCE(this);
		else
			JPARSER_TOK_MUST_BE(this, JSON_TOK_RCURLY, "Expected a \"}\"");
	}

	return obj;

fail:
	json_destroy(obj);
	return NULL;
}

static json_t *jparser_parse_list(jparser_t *this) {
	json_t *list = json_new_list();
	JPARSER_MUST_ADVANCE(this);

	while (this->tok != JSON_TOK_RSQUARE) {
		json_t *json = jparser_parse(this);
		if (json == NULL)
			goto fail;

		if (json_list_add(list, json) != 0) {
			json_destroy(json);
			goto fail;
		}

		JPARSER_MUST_ADVANCE(this);
		if (this->tok == JSON_TOK_COMMA)
			JPARSER_MUST_ADVANCE(this);
		else
			JPARSER_TOK_MUST_BE(this, JSON_TOK_RSQUARE, "Expected a \"]\"");
	}

	return list;

fail:
	json_destroy(list);
	return NULL;
}

#undef JPARSER_TOK_MUST_BE
#undef JPARSER_MUST_ADVANCE

static json_t *jparser_parse(jparser_t *this) {
	switch (this->tok) {
	case JSON_TOK_EOI:
		JPARSER_ERR(this, "Unexpected end of file", this->tok_row, this->tok_col);
		break;

	case JSON_TOK_STR:   return json_new_str(this->data);
	case JSON_TOK_FLOAT: return json_new_float(atof(this->data));
	case JSON_TOK_INT:   return json_new_int64((int64_t)atoll(this->data));
	case JSON_TOK_BOOL:  return json_new_bool(this->data[0] == 't');
	case JSON_TOK_NULL:  return json_null();

	case JSON_TOK_LCURLY:  return jparser_parse_obj(this);
	case JSON_TOK_LSQUARE: return jparser_parse_list(this);

	default: JPARSER_ERR(this, "Unexpected token", this->tok_row, this->tok_col);
	}

	return NULL;
}

#undef JPARSER_ERR

#define JSON_SET_WHERE(ROW_PTR, ROW, COL_PTR, COL) \
	do {                                           \
		if (ROW_PTR != NULL)                       \
			*ROW_PTR = ROW;                        \
		if (COL_PTR != NULL)                       \
			*COL_PTR = COL;                        \
	} while (0)

NOCH_DEF json_t *json_from_file(const char *path, size_t *row, size_t *col) {
	NOCH_ASSERT(path != NULL);

	FILE *file = fopen(path, "r");
	if (file == NULL) {
		NOCH_FOPEN_FAIL();
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	size_t size = (size_t)ftell(file);
	rewind(file);

	if (size == 0) {
		JSON_SET_WHERE(row, 0, col, 0);
		NOCH_PARSER_ERR("File is empty");
		return NULL;
	}

	char *in = (char*)NOCH_ALLOC(size + 1);
	NOCH_CHECK_ALLOC(in);

	if (fread(in, size, 1, file) <= 0) {
		NOCH_FREAD_FAIL();
		return NULL;
	}

	in[size] = '\0';
	fclose(file);

	json_t *json = json_from_mem(in, row, col);
	free(in);
	return json;
}

NOCH_DEF json_t *json_from_mem(const char *in, size_t *row, size_t *col) {
	NOCH_ASSERT(in != NULL);

#define JPARSER_TOK_CAP 64

	jparser_t parser = {0};
	parser.it  = in;
	parser.bol = parser.it;
	parser.row = 1;

	parser.data_cap = JPARSER_TOK_CAP;
	parser.data     = (char*)NOCH_ALLOC(parser.data_cap);
	NOCH_CHECK_ALLOC(parser.data);

#undef JPARSER_TOK_CAP

	if (jparser_advance(&parser) != 0) {
		NOCH_FREE(parser.data);
		JSON_SET_WHERE(row, parser.err_row, col, parser.err_col);
		return NULL;
	}

	json_t *json = jparser_parse(&parser);
	if (json != NULL) {
		if (jparser_advance(&parser) != 0) {
			json_destroy(json);
			NOCH_FREE(parser.data);
			JSON_SET_WHERE(row, parser.err_row, col, parser.err_col);
			return NULL;
		}

		if (parser.tok != JSON_TOK_EOI) {
			json_destroy(json);
			NOCH_FREE(parser.data);
			JSON_SET_WHERE(row, parser.tok_row, col, parser.tok_col);
			NOCH_PARSER_ERR("Unexpected token after data");
			return NULL;
		}
	}

	NOCH_FREE(parser.data);
	JSON_SET_WHERE(row, parser.err_row, col, parser.err_col);
	return json;
}

#undef JSON_SET_WHERE

#undef json_null_instance
#undef json_new
#undef json_strdup
#undef jstream_t
#undef jstream_print
#undef jstream_printf
#undef jstream_print_str
#undef jstream_print_float
#undef jstream_indent
#undef jstream_print_json
#undef json_tok_t
#undef jparser_t
#undef jparser_data_clear
#undef jparser_data_add
#undef jparser_skip_cmnt
#undef jparser_skip_ws_and_cmnts
#undef jparser_tok_start_here
#undef jparser_tok_single
#undef jparser_get_hex4
#undef jparser_unicode_seq
#undef jparser_tok_str
#undef jparser_tok_num
#undef jparser_tok_id
#undef jparser_advance
#undef jparser_parse
#undef jparser_parse_obj
#undef jparser_parse_list

#ifdef __cplusplus
}
#endif
