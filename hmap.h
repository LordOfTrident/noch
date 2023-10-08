#ifndef NOCH_HMAP_H_HEADER_GUARD
#define NOCH_HMAP_H_HEADER_GUARD

/* This library does NOT support C++
 *
 * Thanks to https://github.com/rxi/map for a lot of C template simulation ideas
 */

#ifndef __cplusplus
#	include <stdbool.h> /* bool, true, false */
#endif

#include <stddef.h> /* size_t */
#include <string.h> /* memcpy, memset */

#include "internal/def.h"
#include "internal/err.h"

#ifndef HMAP_DEFAULT_CAP
#	define HMAP_DEFAULT_CAP 1024
#endif

typedef struct {
	const char *key;
	unsigned    hash;
	bool        full;
} hmape_t;

typedef unsigned (*hmap_hash_t)(const char*);
typedef void     (*hmap_free_t)(void*);

typedef struct {
	size_t      cap, valsz;
	hmape_t    *buf;
	hmap_hash_t hash;
	hmap_free_t free;
} hmap_t;

#define HMAP_T(T) \
	struct {      \
		hmap_t _; \
		T *ref;   \
		T  tmp;   \
	}

#define HMAP_INITX(THIS, CAP, HASH, FREE) \
	hmap_init(&(THIS)->_, CAP, sizeof(*(THIS)->ref), HASH, FREE)

#define HMAP_INIT(THIS)   HMAP_INITX(THIS, HMAP_DEFAULT_CAP, default_hash, NULL)
#define HMAP_DEINIT(THIS) hmap_deinit(&(THIS)->_)

#define HMAP_REMOVE(THIS, KEY) hmap_remove(&(THIS)->_, KEY)
#define HMAP_GET(THIS, KEY)    ((THIS)->ref = hmap_get(&(THIS)->_, KEY))
#define HMAP_SET(THIS, KEY, VAL) \
	((THIS)->tmp = VAL,          \
	 hmap_set(&(THIS)->_, KEY, (void*)&(THIS)->tmp))

NOCH_DEF void hmap_init(hmap_t *this, size_t cap, size_t valsz, hmap_hash_t hash, hmap_free_t free);
NOCH_DEF void hmap_deinit(hmap_t *this);

NOCH_DEF int   hmap_remove(hmap_t *this, const char *key);
NOCH_DEF void *hmap_get   (hmap_t *this, const char *key);
NOCH_DEF void  hmap_set   (hmap_t *this, const char *key, void *val);

NOCH_DEF unsigned default_hash(const char *str);

typedef HMAP_T(void*)       hmap_void_t;
typedef HMAP_T(int)         hmap_int_t;
typedef HMAP_T(size_t)      hmap_size_t;
typedef HMAP_T(float)       hmap_float_t;
typedef HMAP_T(double)      hmap_double_t;
typedef HMAP_T(char)        hmap_char_t;
typedef HMAP_T(const char*) hmap_cstr_t;

#endif
