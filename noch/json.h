#ifndef NOCH_JSON_H_HEADER_GUARD
#define NOCH_JSON_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>   /* fread, fprintf, fopen, fclose, FILE, fseek, SEEK_END, rewind, ftell */
#include <stdint.h>  /* int64_t, uint32_t, uint16_t */
#include <stdlib.h>  /* atoll, atof, strtol */
#include <stdarg.h>  /* va_list, va_start, va_end, vsnprintf */
#include <string.h>  /* memset, strlen, strcpy */
#include <ctype.h>   /* isspace, isalnum, isalpha, isdigit, isxdigit */
#include <stdbool.h> /* bool, true, false */

#include "internal/def.h"
#include "internal/error.h"

enum {
	JSON_NULL = 0,
	JSON_STRING,
	JSON_FLOAT,
	JSON_INT,
	JSON_BOOL,
	JSON_LIST,
	JSON_OBJ,

	JSON_TYPES_COUNT,
};

const char *jsonTypeToString(int type);

#ifndef JSON_LIST_CHUNK_SIZE
#	define JSON_LIST_CHUNK_SIZE 64
#endif

#ifndef JSON_OBJ_CHUNK_SIZE
#	define JSON_OBJ_CHUNK_SIZE 64
#endif

typedef struct {
	int type;
} Json;

typedef struct {
	Json base;

	char  *data;
	size_t length;
} JsonString;

typedef struct {
	Json base;

	double value;
} JsonFloat;

typedef struct {
	Json base;

	int64_t value;
} JsonInt;

typedef struct {
	Json base;

	bool value;
} JsonBool;

typedef struct {
	Json base;

	Json **buf;
	size_t cap, size;
} JsonList;

#define FOREACH_IN_JSON_LIST(THIS, VAR, BODY)                          \
	do {                                                               \
		for (size_t nochIt_ = 0; nochIt_ < (THIS)->size; ++ nochIt_) { \
			Json *VAR = (THIS)->buf[nochIt_];                          \
			BODY                                                       \
		}                                                              \
	} while (0)

typedef struct {
	char *key;
	Json *value;
} JsonObjBucket;

/* TODO: Make JsonObj use a hash map */
typedef struct {
	Json base;

	JsonObjBucket *buckets;
	size_t         cap, size;
} JsonObj;

#define FOREACH_IN_JSON_OBJ(THIS, VALUE, KEY, BODY)                    \
	do {                                                               \
		for (size_t nochIt_ = 0; nochIt_ < (THIS)->size; ++ nochIt_) { \
			if ((THIS)->buckets[nochIt_].key == NULL)                  \
				continue;                                              \
			                                                           \
			const char *KEY   = (THIS)->buckets[nochIt_].key;          \
			Json       *VALUE = (THIS)->buckets[nochIt_].value;        \
			BODY                                                       \
		}                                                              \
	} while (0)

#define JSON_STRING(JSON) (nochAssert((JSON)->type == JSON_STRING), (JsonString*)(JSON))
#define JSON_FLOAT(JSON)  (nochAssert((JSON)->type == JSON_FLOAT),  (JsonFloat*) (JSON))
#define JSON_INT(JSON)    (nochAssert((JSON)->type == JSON_INT),    (JsonInt*)   (JSON))
#define JSON_BOOL(JSON)   (nochAssert((JSON)->type == JSON_BOOL),   (JsonBool*)  (JSON))
#define JSON_LIST(JSON)   (nochAssert((JSON)->type == JSON_LIST),   (JsonList*)  (JSON))
#define JSON_OBJ(JSON)    (nochAssert((JSON)->type == JSON_OBJ),    (JsonObj*)   (JSON))

NOCH_DEF Json *jsonNull(void);

NOCH_DEF JsonString *jsonNewString(const char *value);
NOCH_DEF JsonFloat  *jsonNewFloat (double      value);
NOCH_DEF JsonInt    *jsonNewInt   (int64_t     value);
NOCH_DEF JsonBool   *jsonNewBool  (bool        value);
NOCH_DEF JsonList   *jsonNewList  (void);
NOCH_DEF JsonObj    *jsonNewObj   (void);

NOCH_DEF Json **jsonObjAt (JsonObj  *this, const char *key);
NOCH_DEF Json **jsonListAt(JsonList *this, size_t      idx);

#define jsonObjSet(THIS, KEY, JSON) jsonObjSet_ (THIS, KEY, (Json*)JSON)
#define jsonListPush(THIS, JSON)    jsonListPush_(THIS, (Json*)JSON)

NOCH_DEF Json **jsonObjSet_  (JsonObj  *this, const char *key, Json *json);
NOCH_DEF int    jsonObjRemove(JsonObj  *this, const char *key);
NOCH_DEF Json **jsonListPush_(JsonList *this, Json *json);
NOCH_DEF void   jsonListPop  (JsonList *this);

#define jsonDestroy(THIS)      jsonDestroy_  ((Json*)THIS)
#define jsonPrintF(THIS, FILE) jsonPrintF_   ((Json*)THIS, FILE)
#define jsonStringify(THIS)    jsonStringify_((Json*)THIS)

NOCH_DEF void  jsonDestroy_  (Json *this);
NOCH_DEF void  jsonPrintF_   (Json *this, FILE *file);
NOCH_DEF char *jsonStringify_(Json *this);

NOCH_DEF Json *jsonFromFile  (const char *path);
NOCH_DEF Json *jsonFromString(const char *str);

#ifdef __cplusplus
}
#endif
#endif
