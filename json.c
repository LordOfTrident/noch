#ifdef __cplusplus
extern "C" {
#endif

#include "internal/err.c"
#include "json.h"

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
	if (json == NULL) {
		NOCH_OUT_OF_MEM();
		return NULL;
	}

	memset(json, 0, sizeof(json_t));
	json->type = type;
	return json;
}

NOCH_DEF json_t *json_null(void) {
	return &json_null_instance;
}

static char *json_strdup(const char *str) {
	char *duped = (char*)NOCH_ALLOC(strlen(str) + 1);
	if (duped == NULL) {
		NOCH_OUT_OF_MEM();
		return NULL;
	}

	strcpy(duped, str);
	return duped;
}

NOCH_DEF json_t *json_new_str(const char *str) {
	NOCH_ASSERT(str != NULL);

	json_t *json = json_new(JSON_STR);
	if (json == NULL)
		return NULL;

	json->as.str.len = strlen(str);
	json->as.str.buf = json_strdup(str);
	if (json->as.str.buf == NULL) {
		NOCH_FREE(json);
		return NULL;
	}

	return json;
}

NOCH_DEF json_t *json_new_float(double float_) {
	json_t *json = json_new(JSON_FLOAT);
	if (json == NULL)
		return NULL;

	json->as.float_ = float_;
	return json;
}

NOCH_DEF json_t *json_new_int64(int64_t int64) {
	json_t *json = json_new(JSON_INT64);
	if (json == NULL)
		return NULL;

	json->as.int64 = int64;
	return json;
}

NOCH_DEF json_t *json_new_bool(bool bool_) {
	json_t *json = json_new(JSON_BOOL);
	if (json == NULL)
		return NULL;

	json->as.bool_ = bool_;
	return json;
}

NOCH_DEF json_t *json_new_list(void) {
	json_t *json = json_new(JSON_LIST);
	if (json == NULL)
		return NULL;

	json->as.list.cap = JSON_LIST_CHUNK_SIZE;
	json->as.list.buf = (json_t**)NOCH_ALLOC(json->as.list.cap * sizeof(json_t*));
	if (json->as.list.buf == NULL) {
		NOCH_FREE(json);
		NOCH_OUT_OF_MEM();
		return NULL;
	}

	return json;
}

NOCH_DEF json_t *json_new_obj(void) {
	json_t *json = json_new(JSON_OBJ);
	if (json == NULL)
		return NULL;

	json->as.obj.cap  = JSON_OBJ_CHUNK_SIZE;
	json->as.obj.keys = (char**)NOCH_ALLOC(json->as.obj.cap * sizeof(char*));
	if (json->as.obj.keys == NULL) {
		NOCH_FREE(json);
		NOCH_OUT_OF_MEM();
		return NULL;
	}

	json->as.obj.vals = (json_t**)NOCH_ALLOC(json->as.obj.cap * sizeof(json_t*));
	if (json->as.obj.vals == NULL) {
		NOCH_FREE(json->as.obj.keys);
		NOCH_FREE(json);
		NOCH_OUT_OF_MEM();
		return NULL;
	}

	return json;
}

NOCH_DEF void json_destroy(json_t *json) {
	NOCH_ASSERT(json != NULL);

	switch (json->type) {
	case JSON_NULL: return;

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
		void *tmp = NOCH_REALLOC(obj->as.obj.keys, obj->as.obj.cap * sizeof(char*));
		if (tmp == NULL)
			return NOCH_OUT_OF_MEM();
		else
			obj->as.obj.keys = (char**)tmp;

		tmp = NOCH_REALLOC(obj->as.obj.vals, obj->as.obj.cap * sizeof(json_t*));
		if (tmp == NULL)
			return NOCH_OUT_OF_MEM();
		else
			obj->as.obj.vals = (json_t**)tmp;
	}

	char **new_key = &obj->as.obj.keys[obj->as.obj.size];
	*new_key = (char*)NOCH_ALLOC(strlen(key) + 1);
	if (*new_key == NULL)
		return NOCH_OUT_OF_MEM();

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
		void *tmp = NOCH_REALLOC(list->as.list.buf, list->as.list.cap * sizeof(json_t*));
		if (tmp == NULL)
			return NOCH_OUT_OF_MEM();
		else
			list->as.list.buf = (json_t**)tmp;
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
} jsons_t;

static int jsons_print(jsons_t *s, const char *str) {
	if (s->file != NULL)
		fputs(str, s->file);
	else {
		size_t prev = s->size;
		s->size += strlen(str);
		if (s->size + 1 >= s->cap) {
			do
				s->cap *= 2;
			while (s->size + 1 >= s->cap);

			void *tmp = NOCH_REALLOC(s->buf, s->cap);
			if (tmp == NULL)
				return NOCH_OUT_OF_MEM();
			else
				s->buf = (char*)tmp;
		}

		strcpy(s->buf + prev, str);
	}

	return 0;
}

static int jsons_printf(jsons_t *s, const char *fmt, ...) {
	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	return jsons_print(s, str);
}

#define JSONS_MUST(X)  \
	do {               \
		if ((X) != 0)  \
			return -1; \
	} while (0)

#define JSONS_PRINT(S, STR)  JSONS_MUST(jsons_print(S, STR))
#define JSONS_PRINTF(S, ...) JSONS_MUST(jsons_printf(S, __VA_ARGS__))

static int jsons_print_str(jsons_t *s, json_str_t *str) {
	JSONS_PRINT(s, "\"");

	for (size_t i = 0; i < str->len; ++ i) {
		switch (str->buf[i]) {
		case '"':  JSONS_PRINT(s, "\\\""); break;
		case '\\': JSONS_PRINT(s, "\\\\"); break;
		case '\b': JSONS_PRINT(s, "\\b");  break;
		case '\f': JSONS_PRINT(s, "\\f");  break;
		case '\n': JSONS_PRINT(s, "\\n");  break;
		case '\r': JSONS_PRINT(s, "\\r");  break;
		case '\t': JSONS_PRINT(s, "\\t");  break;

		default:
			if ((unsigned char)str->buf[i] < 32 || str->buf[i] == 127)
				JSONS_PRINTF(s, "\\u%04X", str->buf[i]);
			else
				JSONS_PRINTF(s, "%c", str->buf[i]);
		}
	}

	JSONS_PRINT(s, "\"");
	return 0;
}

static int jsons_print_float(jsons_t *s, float num) {
	char buf[256];
	snprintf(buf, sizeof(buf), "%f", num);

	bool   found = false;
	size_t i;
	for (i = 0; buf[i] != '\0'; ++ i) {
		if (buf[i] == '.' && !found)
			found = true;
	}
	if (!found) {
		JSONS_PRINT(s, buf);
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

	JSONS_PRINT(s, buf);
	return 0;
}

static int jsons_indent(jsons_t *s, size_t nest) {
	if (s->file != NULL) {
		for (size_t i = 0; i < nest; ++ i)
			fputc('\t', s->file);
	} else {
		size_t prev = s->size;
		s->size += nest;
		if (s->size + 1 >= s->cap) {
			do
				s->cap *= 2;
			while (s->size + 1 >= s->cap);

			void *tmp = NOCH_REALLOC(s->buf, s->cap);
			if (tmp == NULL)
				return NOCH_OUT_OF_MEM();
			else
				s->buf = (char*)tmp;
		}

		memset(s->buf + prev, '\t', nest);
		s->buf[s->size] = '\0';
	}

	return 0;
}

#define JSONS_INDENT(S, NEST) JSONS_MUST(jsons_indent(S, NEST))

static int jsons_print_json(jsons_t *s, json_t *json, size_t nest, bool comma) {
	NOCH_ASSERT(json != NULL);

	switch (json->type) {
	case JSON_NULL: JSONS_PRINT(s, "null"); break;

	case JSON_STR:   JSONS_MUST(jsons_print_str  (s, &json->as.str));   break;
	case JSON_FLOAT: JSONS_MUST(jsons_print_float(s, json->as.float_)); break;
	case JSON_INT64: JSONS_PRINTF(s, "%lli", (long long)json->as.int64);      break;
	case JSON_BOOL:  JSONS_PRINTF(s, "%s", json->as.bool_? "true" : "false"); break;

	case JSON_LIST:
		++ nest;

		JSONS_PRINT(s, "[");
		if (json->as.list.size > 0)
			JSONS_PRINT(s, "\n");

		for (size_t i = 0; i < json->as.list.size; ++ i) {
			JSONS_INDENT(s, nest);
			JSONS_MUST(jsons_print_json(s, json->as.list.buf[i], nest, i + 1 < json->as.list.size));
		}
		JSONS_INDENT(s, -- nest);
		JSONS_PRINT(s, "]");
		break;

	case JSON_OBJ:
		++ nest;

		JSONS_PRINT(s, "{");
		if (json->as.obj.size > 0)
			JSONS_PRINT(s, "\n");

		for (size_t i = 0; i < json->as.obj.size; ++ i) {
			JSONS_INDENT(s, nest);
			JSONS_PRINTF(s, "\"%s\": ", json->as.obj.keys[i]);
			JSONS_MUST(jsons_print_json(s, json->as.obj.vals[i], nest, i + 1 < json->as.obj.size));
		}
		JSONS_INDENT(s, -- nest);
		JSONS_PRINT(s, "}");
		break;

	default: NOCH_ASSERT(0 && "Unknown json type");
	}

	if (comma)
		JSONS_PRINT(s, ",");

	JSONS_PRINT(s, "\n");
	return 0;
}

#undef JSONS_INDENT
#undef JSONS_PRINT
#undef JSONS_PRINTF
#undef JSONS_MUST

NOCH_DEF void json_fprint(json_t *json, FILE *file) {
	NOCH_ASSERT(file != NULL);

	jsons_t s = {0};
	s.file    = file;

	jsons_print_json(&s, json, 0, false);
}

NOCH_DEF char *json_stringify(json_t *json) {
	jsons_t s = {0};
	s.cap     = JSON_STRINGIFY_CHUNK_SIZE;
	s.buf     = (char*)NOCH_ALLOC(s.cap);
	if (s.buf == NULL)
		return NULL;

	return jsons_print_json(&s, json, 0, false) == 0? s.buf : NULL;
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
} jsonp_t;

#define JSONP_ERR(P, MSG, ROW, COL) \
	((P)->err_row = ROW,            \
	 (P)->err_col = COL,            \
	 NOCH_PARSER_ERR(MSG))

#define JSONP_COL(P) ((size_t)(P)->it - (size_t)(P)->bol + 1)

static void jsonp_data_clear(jsonp_t *p) {
	p->data[0]   = '\0';
	p->data_size = 0;
}

static int jsonp_data_add(jsonp_t *p, char ch) {
	if (p->data_size + 1 >= p->data_cap) {
		p->data_cap *= 2;
		void *tmp = NOCH_REALLOC(p->data, p->data_cap);
		if (tmp == NULL)
			return NOCH_OUT_OF_MEM();
		else
			p->data = (char*)tmp;
	}

	p->data[p->data_size ++] = ch;
	p->data[p->data_size]    = '\0';
	return 0;
}

/* Not standard!!!! but very useful */
static int jsonp_skip_cmnt(jsonp_t *p) {
	NOCH_ASSERT(*p->it == '/' && p->it[1] == '*');

	size_t row = p->row, col = JSONP_COL(p);

	p->it += 2;
	bool prev_asterisk = false;
	while (!prev_asterisk || *p->it != '/') {
		prev_asterisk = *p->it == '*';

		if (*p->it == '\n') {
			++ p->row;
			p->bol = p->it + 1;
		} else if (*p->it == '\0')
			return JSONP_ERR(p, "Comment not terminated", row, col);

		++ p->it;
	}

	return 0;
}

static int jsonp_skip_ws_and_cmnts(jsonp_t *p) {
	while (*p->it != '\0') {
		if (*p->it == '/' && p->it[1] == '*') {
			if (jsonp_skip_cmnt(p) != 0)
				return -1;
		} else if (!isspace(*p->it))
			break;

		if (*p->it == '\n') {
			++ p->row;
			p->bol = p->it + 1;
		}

		++ p->it;
	}

	return 0;
}

static void jsonp_tok_start_here(jsonp_t *p) {
	p->tok_row = p->row;
	p->tok_col = JSONP_COL(p);
}

static int jsonp_tok_single(jsonp_t *p, json_tok_t tok) {
	jsonp_data_clear(p);
	jsonp_tok_start_here(p);
	p->tok = tok;

	++ p->it;
	return 0;
}

static int jsonp_get_hex4(jsonp_t *p, uint16_t *ret) {
	NOCH_ASSERT(ret != NULL);

	char buf[5] = {0};
	for (size_t i = 0; i < 4; ++ i) {
		if (!isxdigit(*p->it))
			return JSONP_ERR(p, "Expected 4 hexadecimal digits", p->row, JSONP_COL(p));

		buf[i] = *p->it ++;
	}

	*ret = strtol(buf, NULL, 16);
	return 0;
}

static int jsonp_useq(jsonp_t *p) {
	size_t col = JSONP_COL(p) - 1;
	++ p->it;

	uint32_t rune;
	uint16_t first, second;
	if (jsonp_get_hex4(p, &first) != 0)
		return -1;

	/* Surrogate pair */
	if (first >= 0xD800 && first <= 0xDBFF) {
		if (*p->it == '\0')
			return JSONP_ERR(p, "Expected a surrogate pair", p->tok_row, p->tok_col);

		if (p->it[0] != '\\' || p->it[1] != 'u')
			return JSONP_ERR(p, "Invalid unicode sequence", p->row, col);

		p->it += 2;
		if (jsonp_get_hex4(p, &second) != 0)
			return -1;

		if (second < 0xDC00 || second > 0xDFFF)
			return JSONP_ERR(p, "Invalid unicode sequence", p->row, col);

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
		return JSONP_ERR(p, "Invalid unicode sequence", p->row, col);

	for (const char *it = out; *it != '\0'; ++ it) {
		if (jsonp_data_add(p, *it) != 0)
			return -1;
	}

    return 0;
}

static int jsonp_tok_str(jsonp_t *p) {
	NOCH_ASSERT(*p->it == '"');

	jsonp_data_clear(p);
	jsonp_tok_start_here(p);
	p->tok = JSON_TOK_STR;

	++ p->it;
	bool escape = false;
	while (escape || *p->it != '"') {
		if (*p->it == '\0' || *p->it  == '\n')
			return JSONP_ERR(p, "String not terminated", p->tok_row, p->tok_col);
		else if (escape) {
			escape = false;
			char escaped;

			switch (*p->it) {
			case '"': case '\\': case '/': escaped = *p->it; break;

			case 'b': escaped = '\b';   break;
			case 'f': escaped = '\f';   break;
			case 'n': escaped = '\n';   break;
			case 'r': escaped = '\r';   break;
			case 't': escaped = '\t';   break;
			case 'e': escaped = '\x1b'; break; /* Not standard!!!! but useful */

			case 'u':
				if (jsonp_useq(p) != 0)
					return -1;
				continue;

			default: return JSONP_ERR(p, "Unknown escape sequence", p->row, JSONP_COL(p) - 1);
			}

			if (jsonp_data_add(p, escaped) != 0)
				return -1;
		} else if (*p->it == '\\') {
			escape = true;
		} else {
			if (jsonp_data_add(p, *p->it) != 0)
				return -1;
		}

		++ p->it;
	}

	++ p->it;
	return 0;
}

static int jsonp_tok_num(jsonp_t *p) {
	NOCH_ASSERT(isdigit(*p->it) || *p->it == '-');

	jsonp_data_clear(p);
	jsonp_tok_start_here(p);

	bool exponent = false, fpoint = false;

	if (*p->it == '-') {
		if (jsonp_data_add(p, *p->it ++) != 0)
			return -1;

		if (!isdigit(*p->it))
			return JSONP_ERR(p, "Expected a number", p->row, JSONP_COL(p));
	}

	while (true) {
		if (*p->it == 'e') {
			if (exponent)
				return JSONP_ERR(p, "Encountered exponent in number twice", p->row, JSONP_COL(p));
			else
				exponent = true;

			if (p->it[1] == '+' || p->it[1] == '-')
				++ p->it;

			if (!isdigit(p->it[1]))
				return JSONP_ERR(p, "Expected a number", p->row, JSONP_COL(p) + 1);
		} else if (*p->it == '.') {
			if (exponent)
				return JSONP_ERR(p, "Unexpected floating point in exponent", p->row, JSONP_COL(p));
			else if (fpoint)
				return JSONP_ERR(p, "Encountered a floating point in number twice",
				                 p->row, JSONP_COL(p));
			else {
				fpoint = true;

				if (!isdigit(p->it[1]))
					return JSONP_ERR(p, "Expected a number", p->row, JSONP_COL(p) + 1);
			}
		} else if (*p->it == '-' || isalpha(*p->it))
			return JSONP_ERR(p, "Unexpected character in number", p->row, JSONP_COL(p));
		else if (!isdigit(*p->it))
			break;

		if (jsonp_data_add(p, *p->it ++) != 0)
			return -1;
	}

	p->tok = exponent || fpoint? JSON_TOK_FLOAT : JSON_TOK_INT;
	return 0;
}

static int jsonp_tok_id(jsonp_t *p) {
	NOCH_ASSERT(isalpha(*p->it));

	jsonp_data_clear(p);
	jsonp_tok_start_here(p);

	while (isalnum(*p->it)) {
		if (jsonp_data_add(p, *p->it) != 0)
			return -1;

		++ p->it;
	}

	if (strcmp(p->data, "null") == 0)
		p->tok = JSON_TOK_NULL;
	else if (strcmp(p->data, "true") == 0 || strcmp(p->data, "false") == 0)
		p->tok = JSON_TOK_BOOL;
	else
		return JSONP_ERR(p, "Unknown identifier", p->tok_row, p->tok_col);

	return 0;
}

static int jsonp_advance(jsonp_t *p) {
	if (jsonp_skip_ws_and_cmnts(p) != 0)
		return -1;
	else if (*p->it == '\0') {
		p->tok     = JSON_TOK_EOI;
		p->tok_row = p->row;
		p->tok_col = JSONP_COL(p);
		return 0;
	}

	switch (*p->it) {
	case '"': return jsonp_tok_str(p);

	case '{': return jsonp_tok_single(p, JSON_TOK_LCURLY);
	case '}': return jsonp_tok_single(p, JSON_TOK_RCURLY);
	case '[': return jsonp_tok_single(p, JSON_TOK_LSQUARE);
	case ']': return jsonp_tok_single(p, JSON_TOK_RSQUARE);
	case ',': return jsonp_tok_single(p, JSON_TOK_COMMA);
	case ':': return jsonp_tok_single(p, JSON_TOK_COLON);

	default:
		if (isdigit(*p->it) || *p->it == '-')
			return jsonp_tok_num(p);
		else if (isalpha(*p->it))
			return jsonp_tok_id(p);
		else
			return JSONP_ERR(p, "Unexpected character", p->row, JSONP_COL(p));
	}
}

#undef JSONP_COL

#define JSONP_MUST_ADVANCE(P)      \
	do {                           \
		if (jsonp_advance(P) != 0) \
			goto fail;             \
	} while (0)

#define JSONP_TOK_MUST_BE(P, TOK, MSG)                     \
	do {                                                   \
		if ((P)->tok != TOK) {                             \
			JSONP_ERR(P, MSG, (P)->tok_row, (P)->tok_col); \
			goto fail;                                     \
		}                                                  \
	} while (0)

static json_t *jsonp_parse(jsonp_t *p);

static json_t *jsonp_parse_obj(jsonp_t *p) {
	json_t *obj = json_new_obj();
	JSONP_MUST_ADVANCE(p);

	while (p->tok != JSON_TOK_RCURLY) {
		JSONP_TOK_MUST_BE(p, JSON_TOK_STR, "Expected a key string");
		char *key = json_strdup(p->data);
		if (key == NULL)
			goto fail;

		JSONP_MUST_ADVANCE(p);
		JSONP_TOK_MUST_BE(p, JSON_TOK_COLON, "Expected a \":\"");

		JSONP_MUST_ADVANCE(p);
		json_t *json = jsonp_parse(p);
		if (json == NULL)
			goto fail;

		if (json_obj_add(obj, key, json) != 0) {
			json_destroy(json);
			goto fail;
		}

		NOCH_FREE(key);

		JSONP_MUST_ADVANCE(p);
		if (p->tok == JSON_TOK_COMMA)
			JSONP_MUST_ADVANCE(p);
		else
			JSONP_TOK_MUST_BE(p, JSON_TOK_RCURLY, "Expected a \"}\"");
	}

	return obj;

fail:
	json_destroy(obj);
	return NULL;
}

static json_t *jsonp_parse_list(jsonp_t *p) {
	json_t *list = json_new_list();
	JSONP_MUST_ADVANCE(p);

	while (p->tok != JSON_TOK_RSQUARE) {
		json_t *json = jsonp_parse(p);
		if (json == NULL)
			goto fail;

		if (json_list_add(list, json) != 0) {
			json_destroy(json);
			goto fail;
		}

		JSONP_MUST_ADVANCE(p);
		if (p->tok == JSON_TOK_COMMA)
			JSONP_MUST_ADVANCE(p);
		else
			JSONP_TOK_MUST_BE(p, JSON_TOK_RSQUARE, "Expected a \"]\"");
	}

	return list;

fail:
	json_destroy(list);
	return NULL;
}

#undef JSONP_TOK_MUST_BE
#undef JSONP_MUST_ADVANCE

static json_t *jsonp_parse(jsonp_t *p) {
	switch (p->tok) {
	case JSON_TOK_EOI: JSONP_ERR(p, "Unexpected end of file", p->tok_row, p->tok_col); break;

	case JSON_TOK_STR:   return json_new_str(p->data);
	case JSON_TOK_FLOAT: return json_new_float(atof(p->data));
	case JSON_TOK_INT:   return json_new_int64((int64_t)atoll(p->data));
	case JSON_TOK_BOOL:  return json_new_bool(p->data[0] == 't');
	case JSON_TOK_NULL:  return json_null();

	case JSON_TOK_LCURLY:  return jsonp_parse_obj(p);
	case JSON_TOK_LSQUARE: return jsonp_parse_list(p);

	default: JSONP_ERR(p, "Unexpected token", p->tok_row, p->tok_col); break;
	}

	return NULL;
}

#undef JSONP_ERR

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
	if (in == NULL) {
		NOCH_OUT_OF_MEM();
		return NULL;
	}

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

#define JSONP_TOK_CAP 64

	jsonp_t p = {0};
	p.it      = in;
	p.bol     = p.it;
	p.row     = 1;

	p.data_cap = JSONP_TOK_CAP;
	p.data     = (char*)NOCH_ALLOC(p.data_cap);
	if (p.data == NULL) {
		NOCH_OUT_OF_MEM();
		return NULL;
	}

#undef JSONP_TOK_CAP

	if (jsonp_advance(&p) != 0) {
		NOCH_FREE(p.data);
		JSON_SET_WHERE(row, p.err_row, col, p.err_col);
		return NULL;
	}

	json_t *json = jsonp_parse(&p);
	if (json != NULL) {
		if (jsonp_advance(&p) != 0) {
			json_destroy(json);
			NOCH_FREE(p.data);
			JSON_SET_WHERE(row, p.err_row, col, p.err_col);
			return NULL;
		}

		if (p.tok != JSON_TOK_EOI) {
			json_destroy(json);
			NOCH_FREE(p.data);
			JSON_SET_WHERE(row, p.tok_row, col, p.tok_col);
			NOCH_PARSER_ERR("Unexpected token after data");
			return NULL;
		}
	}

	NOCH_FREE(p.data);
	JSON_SET_WHERE(row, p.err_row, col, p.err_col);
	return json;
}

#undef JSON_SET_WHERE

#ifdef __cplusplus
}
#endif
