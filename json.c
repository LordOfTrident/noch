#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"
#include "internal/alloc.h"
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
#define jparser_err               NOCH_PRIVATE(jparser_err)
#define jparser_col               NOCH_PRIVATE(jparser_col)
#define jparser_create_data_copy  NOCH_PRIVATE(jparser_create_data_copy)
#define jparser_init              NOCH_PRIVATE(jparser_init)
#define jparser_deinit            NOCH_PRIVATE(jparser_deinit)
#define jparser_expect_input_end  NOCH_PRIVATE(jparser_expect_input_end)

const char *json_type_to_str_map[JSON_TYPES_COUNT] = {
	"null", /* JSON_NULL */

	"string", /* JSON_STR */
	"float",  /* JSON_FLOAT */
	"int",    /* JSON_INT */
	"bool",   /* JSON_BOOL */

	"list", /* JSON_LIST */
	"obj",  /* JSON_OBJ */
};

const char *json_type_to_str(json_type_t type) {
	NOCH_ASSERT(type < JSON_TYPES_COUNT && type >= 0);
	return json_type_to_str_map[type];
}

static json_t json_null_instance;

NOCH_DEF json_t *json_null(void) {
	return &json_null_instance;
}

static char *json_strdup(const char *str) {
	char *duped;
	NOCH_MUST_ALLOC(char, duped, strlen(str) + 1);

	strcpy(duped, str);
	return duped;
}

#define JSON_ALLOC(TO, TYPE)          \
	do {                              \
		NOCH_MUST_ALLOC(TYPE, TO, 1); \
		memset(TO, 0, sizeof(TYPE));  \
	} while (0)

NOCH_DEF json_str_t *json_new_str(const char *val) {
	NOCH_ASSERT(val != NULL);

	json_str_t *str;
	JSON_ALLOC(str, json_str_t);
	str->_.type = JSON_STR;

	str->len = strlen(val);
	str->buf = json_strdup(val);
	return str;
}

NOCH_DEF json_float_t *json_new_float(double val) {
	json_float_t *float_;
	JSON_ALLOC(float_, json_float_t);
	float_->_.type = JSON_FLOAT;

	float_->val = val;
	return float_;
}

NOCH_DEF json_int_t *json_new_int(int64_t val) {
	json_int_t *int_;
	JSON_ALLOC(int_, json_int_t);
	int_->_.type = JSON_INT;

	int_->val = val;
	return int_;
}

NOCH_DEF json_bool_t *json_new_bool(bool val) {
	json_bool_t *bool_;
	JSON_ALLOC(bool_, json_bool_t);
	bool_->_.type = JSON_BOOL;

	bool_->val = val;
	return bool_;
}

NOCH_DEF json_list_t *json_new_list(void) {
	json_list_t *list;
	JSON_ALLOC(list, json_list_t);
	list->_.type = JSON_LIST;

	list->cap = JSON_LIST_CHUNK_SIZE;
	NOCH_MUST_ALLOC(json_t*, list->buf, list->cap);
	return list;
}

NOCH_DEF json_obj_t *json_new_obj(void) {
	json_obj_t *obj;
	JSON_ALLOC(obj, json_obj_t);
	obj->_.type = JSON_OBJ;

	obj->cap = JSON_OBJ_CHUNK_SIZE;
	NOCH_MUST_ALLOC(char*,   obj->keys, obj->cap);
	NOCH_MUST_ALLOC(json_t*, obj->vals, obj->cap);
	return obj;
}

NOCH_DEF void json_destroy(json_t *json) {
	NOCH_ASSERT(json != NULL);

	switch (json->type) {
	case JSON_NULL:  return;
	case JSON_STR:   NOCH_FREE(JSON_STR(json)->buf);
	case JSON_FLOAT: break;
	case JSON_INT:   break;
	case JSON_BOOL:  break;

	case JSON_LIST: {
		json_list_t *list = JSON_LIST(json);

		for (size_t i = 0; i < list->size; ++ i)
			json_destroy(list->buf[i]);

		NOCH_FREE(list->buf);
	} break;

	case JSON_OBJ: {
		json_obj_t *obj = JSON_OBJ(json);

		for (size_t i = 0; i < obj->size; ++ i) {
			NOCH_FREE(obj->keys[i]);
			json_destroy(obj->vals[i]);
		}

		NOCH_FREE(obj->keys);
		NOCH_FREE(obj->vals);
	} break;

	default: NOCH_ASSERT(0 && "Unknown json type");
	}

	NOCH_FREE(json);
}

NOCH_DEF int json_obj_add(json_obj_t *obj, const char *key, json_t *json) {
	NOCH_ASSERT(obj  != NULL);
	NOCH_ASSERT(key  != NULL);
	NOCH_ASSERT(json != NULL);

	if (obj->size >= obj->cap) {
		obj->cap *= 2;
		NOCH_MUST_REALLOC(char*,   obj->keys, obj->cap);
		NOCH_MUST_REALLOC(json_t*, obj->vals, obj->cap);
	}

	char **new_key = &obj->keys[obj->size];
	NOCH_MUST_ALLOC(char, *new_key, strlen(key) + 1);
	strcpy(*new_key, key);

	obj->vals[obj->size ++] = json;
	return 0;
}

NOCH_DEF int json_list_add(json_list_t *list, json_t *json) {
	NOCH_ASSERT(list != NULL);
	NOCH_ASSERT(json != NULL);

	if (list->size >= list->cap) {
		list->cap *= 2;
		NOCH_MUST_REALLOC(json_t*, list->buf, list->cap);
	}

	list->buf[list->size ++] = json;
	return 0;
}

NOCH_DEF json_t *json_obj_at(json_obj_t *obj, const char *key) {
	NOCH_ASSERT(obj != NULL);
	NOCH_ASSERT(key != NULL);

	for (size_t i = 0; i < obj->size; ++ i) {
		if (strcmp(obj->keys[i], key) == 0)
			return obj->vals[i];
	}

	return NULL;
}

NOCH_DEF json_t *json_list_at(json_list_t *list, size_t idx) {
	NOCH_ASSERT(list != NULL);

	if (idx >= list->size)
		return NULL;
	else
		return list->buf[idx];
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

			NOCH_MUST_REALLOC(char, this->buf, this->cap);
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

			NOCH_MUST_REALLOC(char, this->buf, this->cap);
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
	case JSON_NULL:
		JSTREAM_PRINT(this, "null");
		break;

	case JSON_STR:
		JSTREAM_MUST(jstream_print_str(this, JSON_STR(json)));
		break;

	case JSON_FLOAT:
		JSTREAM_MUST(jstream_print_float(this, JSON_FLOAT(json)->val));
		break;

	case JSON_INT:
		JSTREAM_PRINTF(this, "%lli", JSON_INT(json)->val);
		break;

	case JSON_BOOL:
		JSTREAM_PRINTF(this, "%s", JSON_BOOL(json)->val? "true" : "false");
		break;

	case JSON_LIST: {
		json_list_t *list = JSON_LIST(json);

		++ nest;
		JSTREAM_PRINT(this, "[");
		if (list->size > 0)
			JSTREAM_PRINT(this, "\n");

		for (size_t i = 0; i < list->size; ++ i) {
			JSTREAM_INDENT(this, nest);
			JSTREAM_MUST(jstream_print_json(this, list->buf[i], nest, i + 1 < list->size));
		}
		JSTREAM_INDENT(this, -- nest);
		JSTREAM_PRINT(this, "]");
	} break;

	case JSON_OBJ: {
		json_obj_t *obj = JSON_OBJ(json);

		++ nest;
		JSTREAM_PRINT(this, "{");
		if (obj->size > 0)
			JSTREAM_PRINT(this, "\n");

		for (size_t i = 0; i < obj->size; ++ i) {
			JSTREAM_INDENT(this, nest);
			JSTREAM_PRINTF(this, "\"%s\": ", obj->keys[i]);
			JSTREAM_MUST(jstream_print_json(this, obj->vals[i], nest, i + 1 < obj->size));
		}
		JSTREAM_INDENT(this, -- nest);
		JSTREAM_PRINT(this, "}");
	} break;

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
	NOCH_MUST_ALLOC(char, stream.buf, stream.cap);

	return jstream_print_json(&stream, json, 0, false) == 0? stream.buf : NULL;
}

typedef enum {
	JSON_TOK_EOI = 0,

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
	const char *in, *it, *bol; /* input, iterator, beginning of line */
	size_t      row;

	json_tok_t tok;
	size_t     tok_row, tok_col;

	char  *data, *data_copy;
	size_t data_cap, data_size, data_copy_cap;

	size_t err_row, err_col;
} jparser_t;

static void jparser_init(jparser_t *this, const char *in) {
	this->in       = in;
	this->it       = in;
	this->bol      = this->it;
	this->row      = 1;
	this->data_cap = 64;
	NOCH_MUST_ALLOC(char, this->data, this->data_cap);
}

static void jparser_deinit(jparser_t *this) {
	NOCH_FREE(this->data);

	if (this->data_copy != NULL)
		NOCH_FREE(this->data_copy);
}

inline static int jparser_err(jparser_t *this, const char *msg, size_t row, size_t col) {
	this->err_row = row;
	this->err_col = col;
	NOCH_PARSER_ERR(msg);
	return -1;
}

inline static size_t jparser_col(jparser_t *this) {
	return this->it - this->bol + 1;
}

static char *jparser_create_data_copy(jparser_t *this) {
	if (this->data_copy == NULL) {
		this->data_copy_cap = this->data_cap;
		NOCH_MUST_ALLOC(char, this->data_copy, this->data_copy_cap);
	} else if (this->data_copy_cap < this->data_cap) {
		this->data_copy_cap = this->data_cap;
		NOCH_MUST_REALLOC(char, this->data_copy, this->data_copy_cap);
	}

	strcpy(this->data_copy, this->data);
	return this->data_copy;
}

static void jparser_data_clear(jparser_t *this) {
	this->data[0]   = '\0';
	this->data_size = 0;
}

static int jparser_data_add(jparser_t *this, char ch) {
	if (this->data_size + 1 >= this->data_cap) {
		this->data_cap *= 2;
		NOCH_MUST_REALLOC(char, this->data, this->data_cap);
	}

	this->data[this->data_size ++] = ch;
	this->data[this->data_size]    = '\0';
	return 0;
}

/* Not standard!!!! but very useful */
static int jparser_skip_cmnt(jparser_t *this) {
	NOCH_ASSERT(*this->it == '/' && this->it[1] == '*');

	size_t row = this->row, col = jparser_col(this);

	this->it += 2;
	bool prev_asterisk = false;
	while (!prev_asterisk || *this->it != '/') {
		prev_asterisk = *this->it == '*';

		if (*this->it == '\n') {
			++ this->row;
			this->bol = this->it + 1;
		} else if (*this->it == '\0')
			return jparser_err(this, "Comment not terminated", row, col);

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
	this->tok_col = jparser_col(this);
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
			return jparser_err(this, "Expected 4 hexadecimal digits", this->row, jparser_col(this));

		buf[i] = *this->it ++;
	}

	*ret = strtol(buf, NULL, 16);
	return 0;
}

static int jparser_unicode_seq(jparser_t *this) {
	size_t col = jparser_col(this) - 1;
	++ this->it;

	uint32_t rune;
	uint16_t first, second;
	if (jparser_get_hex4(this, &first) != 0)
		return -1;

	/* Surrogate pair */
	if (first >= 0xD800 && first <= 0xDBFF) {
		if (*this->it == '\0')
			return jparser_err(this, "Expected a surrogate pair", this->tok_row, this->tok_col);

		if (this->it[0] != '\\' || this->it[1] != 'u')
			return jparser_err(this, "Invalid unicode sequence", this->row, col);

		this->it += 2;
		if (jparser_get_hex4(this, &second) != 0)
			return -1;

		if (second < 0xDC00 || second > 0xDFFF)
			return jparser_err(this, "Invalid unicode sequence", this->row, col);

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
		return jparser_err(this, "Invalid unicode sequence", this->row, col);

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
			return jparser_err(this, "String not terminated", this->tok_row, this->tok_col);
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

			default: return jparser_err(this, "Unknown escape sequence",
			                            this->row, jparser_col(this) - 1);
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
			return jparser_err(this, "Expected a number", this->row, jparser_col(this));
	}

	while (true) {
		if (*this->it == 'e') {
			if (exponent)
				return jparser_err(this, "Encountered exponent in number twice",
				                   this->row, jparser_col(this));
			else
				exponent = true;

			if (this->it[1] == '+' || this->it[1] == '-')
				++ this->it;

			if (!isdigit(this->it[1]))
				return jparser_err(this, "Expected a number", this->row, jparser_col(this) + 1);
		} else if (*this->it == '.') {
			if (exponent)
				return jparser_err(this, "Unexpected floating point in exponent",
				                   this->row, jparser_col(this));
			else if (fpoint)
				return jparser_err(this, "Encountered a floating point in number twice",
				                   this->row, jparser_col(this));
			else {
				fpoint = true;

				if (!isdigit(this->it[1]))
					return jparser_err(this, "Expected a number", this->row, jparser_col(this) + 1);
			}
		} else if (*this->it == '-' || isalpha(*this->it))
			return jparser_err(this, "Unexpected character in number",
			                   this->row, jparser_col(this));
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
		return jparser_err(this, "Unknown identifier", this->tok_row, this->tok_col);

	return 0;
}

static int jparser_advance(jparser_t *this) {
	if (jparser_skip_ws_and_cmnts(this) != 0)
		return -1;
	else if (*this->it == '\0') {
		this->tok     = JSON_TOK_EOI;
		this->tok_row = this->row;
		this->tok_col = jparser_col(this);
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
			return jparser_err(this, "Unexpected character", this->row, jparser_col(this));
	}
}

/* TODO: Is there a better way possibly? */
#define JPARSER_MUST_ADVANCE(P)      \
	do {                             \
		if (jparser_advance(P) != 0) \
			goto fail;               \
	} while (0)

#define JPARSER_TOK_MUST_BE(P, TOK, MSG)                     \
	do {                                                     \
		if ((P)->tok != TOK) {                               \
			jparser_err(P, MSG, (P)->tok_row, (P)->tok_col); \
			goto fail;                                       \
		}                                                    \
	} while (0)

static json_t *jparser_parse(jparser_t *this);

static json_obj_t *jparser_parse_obj(jparser_t *this) {
	json_obj_t *obj = json_new_obj();
	if (jparser_advance(this) != 0)
		goto fail;

	while (this->tok != JSON_TOK_RCURLY) {
		if (this->tok != JSON_TOK_STR) {
			jparser_err(this, "Expected a key string", this->tok_row, this->tok_col);
			goto fail;
		}

		char *key = jparser_create_data_copy(this);

		if (jparser_advance(this) != 0)
			goto fail;

		if (this->tok != JSON_TOK_COLON) {
			jparser_err(this, "Expected a \":\"", this->tok_row, this->tok_col);
			goto fail;
		}

		if (jparser_advance(this) != 0)
			goto fail;

		json_t *json = jparser_parse(this);
		if (json == NULL)
			goto fail;

		if (json_obj_add(obj, key, json) != 0) {
			json_destroy(json);
			goto fail;
		}

		if (jparser_advance(this) != 0)
			goto fail;

		if (this->tok == JSON_TOK_COMMA) {
			if (jparser_advance(this) != 0) {
				goto fail;
			}
		} else if (this->tok != JSON_TOK_RCURLY) {
			jparser_err(this, "Expected a \"}\"", this->tok_row, this->tok_col);
			goto fail;
		}
	}

	return obj;

fail:
	json_destroy((json_t*)obj);
	return NULL;
}

static json_list_t *jparser_parse_list(jparser_t *this) {
	json_list_t *list = json_new_list();
	if (jparser_advance(this) != 0)
		goto fail;

	while (this->tok != JSON_TOK_RSQUARE) {
		json_t *json = jparser_parse(this);
		if (json == NULL)
			goto fail;

		if (json_list_add(list, json) != 0) {
			json_destroy(json);
			goto fail;
		}

		if (jparser_advance(this) != 0)
			goto fail;

		if (this->tok == JSON_TOK_COMMA) {
			if (jparser_advance(this) != 0)
				goto fail;
		} else if (this->tok != JSON_TOK_RSQUARE) {
			jparser_err(this, "Expected a \"]\"", this->tok_row, this->tok_col);
			goto fail;
		}
	}

	return list;

fail:
	json_destroy((json_t*)list);
	return NULL;
}

static int jparser_expect_input_end(jparser_t *this) {
	if (jparser_advance(this) != 0)
		return -1;

	if (this->tok != JSON_TOK_EOI)
		return jparser_err(this, "Unexpected token after data end", this->tok_row, this->tok_col);

	return 0;
}

static json_t *jparser_parse(jparser_t *this) {
	if (this->it == this->in) {
		if (jparser_advance(this) != 0)
			return NULL;
	}

	switch (this->tok) {
	case JSON_TOK_EOI:
		jparser_err(this, "Unexpected end of input", this->tok_row, this->tok_col);
		break;

	case JSON_TOK_STR:   return (json_t*)json_new_str(this->data);
	case JSON_TOK_FLOAT: return (json_t*)json_new_float(atof(this->data));
	case JSON_TOK_INT:   return (json_t*)json_new_int((int64_t)atoll(this->data));
	case JSON_TOK_BOOL:  return (json_t*)json_new_bool(this->data[0] == 't');
	case JSON_TOK_NULL:  return (json_t*)json_null();

	case JSON_TOK_LCURLY:  return (json_t*)jparser_parse_obj(this);
	case JSON_TOK_LSQUARE: return (json_t*)jparser_parse_list(this);

	default: jparser_err(this, "Unexpected token", this->tok_row, this->tok_col);
	}

	return NULL;
}

NOCH_DEF json_t *json_from_file(const char *path, size_t *row, size_t *col) {
	NOCH_ASSERT(path != NULL);

	if (row != NULL) *row = 0;
	if (col != NULL) *col = 0;

	FILE *file = fopen(path, "r");
	if (file == NULL)
		return NOCH_NULL(NOCH_FOPEN_FAIL());

	fseek(file, 0, SEEK_END);
	size_t size = (size_t)ftell(file);
	rewind(file);

	if (size == 0)
		return NOCH_NULL(NOCH_PARSER_ERR("File is empty"));

	char *in;
	NOCH_MUST_ALLOC(char, in, size + 1);

	if (fread(in, size, 1, file) <= 0)
		return NOCH_NULL(NOCH_FREAD_FAIL());

	in[size] = '\0';
	fclose(file);

	json_t *json = json_from_mem(in, row, col);
	free(in);
	return json;
}

NOCH_DEF json_t *json_from_mem(const char *in, size_t *row, size_t *col) {
	NOCH_ASSERT(in != NULL);

	jparser_t parser_ = {0}, *parser = &parser_;
	jparser_init(parser, in);

	json_t *json = jparser_parse(parser);
	if (json != NULL) {
		if (jparser_expect_input_end(parser) != 0) {
			json_destroy(json);
			json = NULL;
		}
	}

	jparser_deinit(parser);
	if (row != NULL) *row = parser->err_row;
	if (col != NULL) *col = parser->err_col;
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
