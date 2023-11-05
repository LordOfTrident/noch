#ifndef NOCH_HASHMAP_H_HEADER_GUARD
#define NOCH_HASHMAP_H_HEADER_GUARD

/* This library relies on implicit casting of void*, which C++ does not allow. */
#ifdef __cplusplus
#	error "noch/hashmap does not support C++. Use std::unordered_map instead."
#endif

/* Thanks to https://github.com/rxi/map for a lot of C template simulation ideas */

#include <stdbool.h> /* bool, true, false */
#include <stddef.h>  /* size_t */
#include <string.h>  /* memcpy, memset */

#include "internal/def.h"

NOCH_DEF unsigned hashFuncOneAtATime(const char *str);
NOCH_DEF unsigned hashFuncDefault   (const char *str);

#ifndef HASHMAP_DEFAULT_CAP
#	define HASHMAP_DEFAULT_CAP 1024
#endif

typedef struct {
	const char *key;
	unsigned    hash;
	bool        full;
} HashmapBucket;

typedef unsigned (*HashmapHashFunc)(const char*);
typedef void     (*HashmapDestructor)(void*);

typedef struct {
	size_t  cap, valueSize, bucketSize;
	void   *buckets;

	HashmapHashFunc   hash;
	HashmapDestructor destruct;
} Hashmap;

#define FOREACH_IN_HASHMAP(THIS, REF, KEY, BODY)                                         \
	do {                                                                                 \
		for (size_t nochIt_ = 0;                                                         \
		     nochIt_ < (THIS)->base.cap * (THIS)->base.bucketSize;                       \
		     nochIt_ += (THIS)->base.bucketSize) {                                       \
			HashmapBucket *nochBucket_ = (void*)((char*)(THIS)->base.buckets + nochIt_); \
			if (!nochBucket_->full)                                                      \
				continue;                                                                \
			const char *KEY = nochBucket_->key;                                          \
			void *REF = (char*)(nochBucket_) + sizeof(HashmapBucket);                    \
			BODY                                                                         \
		}                                                                                \
	} while (0)

#define HASHMAP(T)    \
	struct {          \
		Hashmap base; \
		T *ref;       \
		T  tmp;       \
	}

#define hashmapInitEx(THIS, CAP, HASH_FUNC, DESTRUCTOR) \
	hashmapInit_(&(THIS)->base, CAP, sizeof(*(THIS)->ref), HASH_FUNC, DESTRUCTOR)
#define hashmapInit(THIS)   hashmapInitEx(THIS, HASHMAP_DEFAULT_CAP, hashFuncDefault, NULL)
#define hashmapDeinit(THIS) hashmapDeinit_(&(THIS)->base)

#define hashmapRemove(THIS, KEY) hashmapRemove_(&(THIS)->base, KEY)
#define hashmapGet(THIS, KEY)    ((THIS)->ref = hashmapGet_(&(THIS)->base, KEY))
#define hashmapSet(THIS, KEY, VAL) \
	((THIS)->tmp = VAL, hashmapSet_(&(THIS)->base, KEY, (void*)&(THIS)->tmp))

NOCH_DEF int hashmapInit_(Hashmap *this, size_t cap, size_t valueSize,
                          HashmapHashFunc hash, HashmapDestructor destruct);
NOCH_DEF void hashmapDeinit_(Hashmap *this);

NOCH_DEF int   hashmapRemove_(Hashmap *this, const char *key);
NOCH_DEF void *hashmapGet_   (Hashmap *this, const char *key);
NOCH_DEF int   hashmapSet_   (Hashmap *this, const char *key, void *value);

typedef HASHMAP(void*)       HashmapPtr;
typedef HASHMAP(int)         HashmapInt;
typedef HASHMAP(size_t)      HashmapSize;
typedef HASHMAP(float)       HashmapFloat;
typedef HASHMAP(double)      HashmapDouble;
typedef HASHMAP(char)        HashmapChar;
typedef HASHMAP(const char*) HashmapString;

#endif
