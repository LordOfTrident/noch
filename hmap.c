#ifdef __cplusplus
extern "C" {
#endif

#include "internal/private.h"
#include "internal/alloc.h"
#include "internal/assert.h"
#include "internal/err.c"

#include "args.h"

#define hmap_find NOCH_PRIVATE(hmap_find)

#define HMAPE_SIZE(THIS)   (sizeof(hmape_t) + (THIS)->valsz)
#define HMAPE_VAL(THIS, E) (void*)((char*)(E) + sizeof(hmape_t))

NOCH_DEF void hmap_init(hmap_t *this, size_t cap, size_t valsz,
                        hmap_hash_t hash, hmap_free_t free) {
	this->cap   = cap;
	this->valsz = valsz;
	this->hash  = hash;
	this->free  = free;
	this->buf   = NOCH_ALLOC(this->cap * HMAPE_SIZE(this));
	NOCH_CHECK_ALLOC(this->buf);

	memset(this->buf, 0, this->cap * HMAPE_SIZE(this));
}

NOCH_DEF void hmap_deinit(hmap_t *this) {
	if (this->free != NULL) {
		for (size_t i = 0; i < this->cap; ++ i) {
			if (this->buf[i].full)
				this->free(HMAPE_VAL(this, &this->buf[i]));
		}
	}

	NOCH_FREE(this->buf);
}

static hmape_t *hmap_find(hmap_t *this, const char *key) {
	unsigned hash = this->hash(key);
	size_t   idx  = hash % this->cap;

	if (!this->buf[idx].full)
		return NULL;
	else if (this->buf[idx].hash != hash) {
		bool found = false;
		for (size_t i = idx + 1; i != idx; ++ i) {
			if (i > this->cap)
				i = 0;

			if (this->buf[i].hash == hash) {
				idx   = i;
				found = true;
				break;
			}
		}

		if (!found)
			return NULL;
	}

	return &this->buf[idx];
}

NOCH_DEF int hmap_remove(hmap_t *this, const char *key) {
	hmape_t *e = hmap_find(this, key);
	if (e == NULL)
		return -1;

	e->hash = 0;
	e->full = false;

	if (this->free != NULL)
		this->free(HMAPE_VAL(this, e));

	return 0;
}

NOCH_DEF void *hmap_get(hmap_t *this, const char *key) {
	hmape_t *e = hmap_find(this, key);
	if (e == NULL)
		return NULL;

	return HMAPE_VAL(this, e);
}

NOCH_DEF void hmap_set(hmap_t *this, const char *key, void *val) {
	unsigned hash = this->hash(key);
	size_t   idx  = hash % this->cap;

	if (this->buf[idx].hash != hash && this->buf[idx].full) {
		bool found = false;
		for (size_t i = idx + 1; i != idx; ++ i) {
			if (i >= this->cap)
				i = 0;

			if (!this->buf[i].full) {
				idx   = i;
				found = true;
				break;
			}
		}

		if (!found) {
			size_t   prev_cap = this->cap;
			hmape_t *prev_buf = this->buf;

			this->cap *= 2;
			this->buf  = NOCH_ALLOC(this->cap * HMAPE_SIZE(this));
			NOCH_CHECK_ALLOC(this->buf);

			memset(this->buf, 0, this->cap * HMAPE_SIZE(this));

			for (size_t i = 0; i < prev_cap; ++ i)
				hmap_set(this, prev_buf[i].key, HMAPE_VAL(this, &prev_buf[i]));

			NOCH_FREE(prev_buf);
			hmap_set(this, key, val);
			return;
		}
	}

	if (!this->buf[idx].full)
		this->buf[idx].full = true;
	else if (this->free != NULL)
		this->free(HMAPE_VAL(this, &this->buf[idx]));

	this->buf[idx].key  = key;
	this->buf[idx].hash = hash;
	memcpy(HMAPE_VAL(this, &this->buf[idx]), val, this->valsz);
}

#undef HMAPE_SIZE
#undef HMAPE_VAL

NOCH_DEF unsigned default_hash(const char *str) {
	unsigned hash = 5381;
	while (*str != '\0')
		hash = ((hash << 5) + hash) ^ *str ++;

	return hash;
}

#undef hmap_find

#ifdef __cplusplus
}
#endif
