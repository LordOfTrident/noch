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

typedef enum {
	JSON_NULL = 0,

	JSON_STR,
	JSON_FLOAT,
	JSON_INT64,
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
#	define JSON_OBJ_CHUNK_SIZE 32
#endif

#ifndef JSON_STRINGIFY_CHUNK_SIZE
#	define JSON_STRINGIFY_CHUNK_SIZE 128
#endif

typedef struct json json_t;

typedef struct {
	char  *buf;
	size_t len;
} json_str_t;

typedef struct {
	json_t **buf;

	size_t cap, size;
} json_list_t;

/* TODO: Make json_obj_t use a hash map */

typedef struct {
	char   **keys;
	json_t **vals;

	size_t cap, size;
} json_obj_t;

/* json_t */
struct json {
	union {
		json_str_t  str;
		double      float_;
		int64_t     int64;
		bool        bool_;
		json_list_t list;
		json_obj_t  obj;
	} as;

	json_type_t type;
};

#define JSON_AS_STR(JSON)                   \
	(NOCH_ASSERT((JSON)->type == JSON_STR), \
	 (JSON)->as.str)

 #define JSON_AS_FLOAT(JSON)                  \
 	(NOCH_ASSERT((JSON)->type == JSON_FLOAT), \
 	 (JSON)->as.float_)

 #define JSON_AS_INT64(JSON)                  \
 	(NOCH_ASSERT((JSON)->type == JSON_INT64), \
 	 (JSON)->as.int64)

 #define JSON_AS_BOOL(JSON)                  \
 	(NOCH_ASSERT((JSON)->type == JSON_BOOL), \
 	 (JSON)->as.bool_)

 #define JSON_AS_LIST(JSON)                  \
 	(NOCH_ASSERT((JSON)->type == JSON_LIST), \
 	 (JSON)->as.list)

 #define JSON_AS_OBJ(JSON)                  \
 	(NOCH_ASSERT((JSON)->type == JSON_OBJ), \
 	 (JSON)->as.obj)

NOCH_DEF json_t *json_null(void);

NOCH_DEF json_t *json_new_str  (const char *str);
NOCH_DEF json_t *json_new_float(double      float_);
NOCH_DEF json_t *json_new_int64(int64_t     int64);
NOCH_DEF json_t *json_new_bool (bool        bool_);

NOCH_DEF json_t *json_new_list(void);
NOCH_DEF json_t *json_new_obj (void);

NOCH_DEF void json_destroy(json_t *json);

NOCH_DEF int json_obj_add (json_t *obj,  const char *key, json_t *json);
NOCH_DEF int json_list_add(json_t *list, json_t *json);

NOCH_DEF json_t *json_obj_at (json_t *obj,  const char *key);
NOCH_DEF json_t *json_list_at(json_t *list, size_t idx);

NOCH_DEF void  json_fprint   (json_t *json, FILE *file);
NOCH_DEF char *json_stringify(json_t *json);

NOCH_DEF json_t *json_from_file(const char *path, size_t *row, size_t *col);
NOCH_DEF json_t *json_from_mem (const char *in,   size_t *row, size_t *col);

#ifdef __cplusplus
}
#endif
#endif
