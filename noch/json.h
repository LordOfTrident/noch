#ifndef NOCH_JSON_H_HEADER_GUARD
#define NOCH_JSON_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stdio.h>  /* fread, fputs, fputc, fopen, fclose, FILE, fseek, SEEK_END, rewind, ftell */
#include <stdint.h> /* int64_t, uint32_t, uint16_t, uint8_t */
#include <stdlib.h> /* atoll, atof, strtol */
#include <stdarg.h> /* va_list, va_start, va_end, vsnprintf */
#include <string.h> /* memset, strlen, strcpy */
#include <ctype.h>  /* isspace, isalnum, isalpha, isdigit, isxdigit */

#include "internal/def.h"
#include "internal/err.h"
#include "internal/assert.h"

typedef enum {
	JSON_NULL = 0,

	JSON_STR,
	JSON_FLOAT,
	JSON_INT,
	JSON_BOOL,

	JSON_LIST,
	JSON_OBJ,

	JSON_TYPES_COUNT,
} json_type_t;

const char *json_type_to_str(json_type_t type);

#ifndef JSON_LIST_CHUNK_SIZE
#	define JSON_LIST_CHUNK_SIZE 64
#endif

#ifndef JSON_OBJ_CHUNK_SIZE
#	define JSON_OBJ_CHUNK_SIZE 64
#endif

#ifndef JSON_STRINGIFY_CHUNK_SIZE
#	define JSON_STRINGIFY_CHUNK_SIZE 128
#endif

typedef struct {
	json_type_t type;
} json_t;

typedef struct {
	json_t _;

	char  *buf;
	size_t len;
} json_str_t;

typedef struct {
	json_t _;

	double val;
} json_float_t;

typedef struct {
	json_t _;

	int64_t val;
} json_int_t;

typedef struct {
	json_t _;

	bool val;
} json_bool_t;

typedef struct {
	json_t _;

	json_t **buf;
	size_t   cap, size;
} json_list_t;

/* TODO: Make json_obj_t use a hash map */
typedef struct {
	json_t _;

	char   **keys;
	json_t **vals;
	size_t   cap, size;
} json_obj_t;

#define JSON_STR(JSON)                      \
	(NOCH_ASSERT((JSON)->type == JSON_STR), \
	 (json_str_t*)(JSON))

#define JSON_FLOAT(JSON)                      \
	(NOCH_ASSERT((JSON)->type == JSON_FLOAT), \
	 (json_float_t*)(JSON))

#define JSON_INT(JSON)                      \
	(NOCH_ASSERT((JSON)->type == JSON_INT), \
	 (json_int_t*)(JSON))

#define JSON_BOOL(JSON)                      \
	(NOCH_ASSERT((JSON)->type == JSON_BOOL), \
	 (json_bool_t*)(JSON))

#define JSON_LIST(JSON)                      \
	(NOCH_ASSERT((JSON)->type == JSON_LIST), \
	 (json_list_t*)(JSON))

#define JSON_OBJ(JSON)                      \
	(NOCH_ASSERT((JSON)->type == JSON_OBJ), \
	 (json_obj_t*)(JSON))

#define JSON_EXPECT_STR(STR, JSON, ...)              \
	do {                                             \
		json_t *_recieved_json = JSON;               \
		if (_recieved_json == NULL)                  \
			STR = NULL;                              \
		else if (_recieved_json->type != JSON_STR) { \
			STR = NULL;                              \
			__VA_ARGS__                              \
		} else                                       \
			STR = JSON_STR(_recieved_json);          \
	} while (0)

#define JSON_EXPECT_FLOAT(FLOAT, JSON, ...)            \
	do {                                               \
		json_t *_recieved_json = JSON;                 \
		if (_recieved_json == NULL)                    \
			FLOAT = NULL;                              \
		else if (_recieved_json->type != JSON_FLOAT) { \
			FLOAT = NULL;                              \
			__VA_ARGS__                                \
		} else                                         \
			FLOAT = JSON_FLOAT(_recieved_json);        \
	} while (0)

#define JSON_EXPECT_INT(INT, JSON, ...)              \
	do {                                             \
		json_t *_recieved_json = JSON;               \
		if (_recieved_json == NULL)                  \
			INT = NULL;                              \
		else if (_recieved_json->type != JSON_INT) { \
			INT = NULL;                              \
			__VA_ARGS__                              \
		} else                                       \
			INT = JSON_INT(_recieved_json);          \
	} while (0)

#define JSON_EXPECT_BOOL(BOOL, JSON, ...)             \
	do {                                              \
		json_t *_recieved_json = JSON;                \
		if (_recieved_json == NULL)                   \
			BOOL = NULL;                              \
		else if (_recieved_json->type != JSON_BOOL) { \
			BOOL = NULL;                              \
			__VA_ARGS__                               \
		} else                                        \
			BOOL = JSON_BOOL(_recieved_json);         \
	} while (0)

#define JSON_EXPECT_LIST(LIST, JSON, ...)             \
	do {                                              \
		json_t *_recieved_json = JSON;                \
		if (_recieved_json == NULL)                   \
			LIST = NULL;                              \
		else if (_recieved_json->type != JSON_LIST) { \
			LIST = NULL;                              \
			__VA_ARGS__                               \
		} else                                        \
			LIST = JSON_LIST(_recieved_json);         \
	} while (0)

#define JSON_EXPECT_OBJ(OBJ, JSON, ...)              \
	do {                                             \
		json_t *_recieved_json = JSON;               \
		if (_recieved_json == NULL)                  \
			OBJ = NULL;                              \
		else if (_recieved_json->type != JSON_OBJ) { \
			OBJ = NULL;                              \
			__VA_ARGS__                              \
		} else                                       \
			OBJ = JSON_OBJ(_recieved_json);          \
	} while (0)

NOCH_DEF json_t *json_null(void);

NOCH_DEF json_str_t   *json_new_str  (const char *val);
NOCH_DEF json_float_t *json_new_float(double      val);
NOCH_DEF json_int_t   *json_new_int64(int64_t     val);
NOCH_DEF json_bool_t  *json_new_bool (bool        val);

NOCH_DEF json_list_t *json_new_list(void);
NOCH_DEF json_obj_t  *json_new_obj (void);

NOCH_DEF void json_destroy(json_t *json);

#define JSON_DESTROY(JSON) json_destroy((json_t*)JSON)

NOCH_DEF int json_obj_add (json_obj_t  *obj,  const char *key, json_t *json);
NOCH_DEF int json_list_add(json_list_t *list, json_t *json);

#define JSON_OBJ_ADD(OBJ, KEY, JSON) json_obj_add (OBJ, KEY, (json_t*)(JSON))
#define JSON_LIST_ADD(LIST, JSON)    json_list_add(LIST, (json_t*)(JSON))

NOCH_DEF json_t *json_obj_at (json_obj_t  *this, const char *key);
NOCH_DEF json_t *json_list_at(json_list_t *this, size_t idx);

NOCH_DEF void  json_fprint   (json_t *json, FILE *file);
NOCH_DEF char *json_stringify(json_t *json);

#define JSON_FPRINT(JSON, FILE) json_fprint   ((json_t*)JSON, FILE)
#define JSON_STRINGIFY(JSON)    json_stringify((json_t*)JSON)

NOCH_DEF json_t *json_from_file(const char *path, size_t *row, size_t *col);
NOCH_DEF json_t *json_from_mem (const char *in,   size_t *row, size_t *col);

#ifdef __cplusplus
}
#endif
#endif
