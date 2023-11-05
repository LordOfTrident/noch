#include "internal/alloc.h"
#include "internal/assert.h"

#include "args.h"

NOCH_DEF unsigned hashFuncOneAtATime(const char *str) {
	unsigned hash = 0;
	while (*str != '\0') {
		hash += (unsigned)*str ++;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

NOCH_DEF unsigned hashFuncDefault(const char *str) {
	unsigned hash = 5381;
	while (*str != '\0')
		hash = ((hash << 5) + hash) ^ *str ++;

	return hash;
}

#define HASHMAP_BUCKET_VAL(BUCKET) (void*)((char*)(BUCKET) + sizeof(HashmapBucket))
#define HASHMAP_BUCKET_AT(HASHMAP, IDX) \
	(HashmapBucket*)((char*)this->buckets + ((HASHMAP)->bucketSize * (IDX)))

NOCH_DEF int hashmapInit_(Hashmap *this, size_t cap, size_t valueSize,
                          HashmapHashFunc hash, HashmapDestructor destruct) {
	this->cap        = cap;
	this->valueSize  = valueSize;
	this->bucketSize = sizeof(HashmapBucket) + valueSize;
	this->hash       = hash;
	this->destruct   = destruct;
	this->buckets    = nochAlloc(this->cap * this->bucketSize);
	if (this->buckets == NULL)
		NOCH_OUT_OF_MEM();

	memset(this->buckets, 0, this->cap * this->bucketSize);
	return 0;
}

NOCH_DEF void hashmapDeinit_(Hashmap *this) {
	if (this->destruct != NULL) {
		for (size_t i = 0; i < this->cap * this->valueSize; i += this->valueSize) {
			HashmapBucket *bucket = (HashmapBucket*)((char*)this->buckets + i);
			if (bucket->full)
				this->destruct(HASHMAP_BUCKET_VAL(bucket));
		}
	}

	nochFree(this->buckets);
}

static bool hashmapBucketMatches(HashmapBucket *bucket, unsigned hash, const char *key) {
	if (!bucket->full || bucket->hash != hash)
		return false;
	else
		return strcmp(bucket->key, key) == 0;
}

static HashmapBucket *hashmapFind(Hashmap *this, const char *key) {
	unsigned hash = this->hash(key);
	size_t   idx  = hash % this->cap;

	HashmapBucket *bucket = HASHMAP_BUCKET_AT(this, idx);

	if (!bucket->full)
		return NULL;
	else if (!hashmapBucketMatches(bucket, hash, key)) {
		bool found = false;
		for (size_t i = idx + 1; i != idx; ++ i) {
			if (i >= this->cap)
				i = 0;

			bucket = HASHMAP_BUCKET_AT(this, i);
			if (hashmapBucketMatches(bucket, hash, key)) {
				found = true;
				break;
			}
		}

		if (!found)
			return NULL;
	}

	return bucket;
}

NOCH_DEF int hashmapRemove_(Hashmap *this, const char *key) {
	HashmapBucket *bucket = hashmapFind(this, key);
	if (bucket == NULL)
		return -1;

	bucket->hash = 0;
	bucket->full = false;

	if (this->destruct != NULL)
		this->destruct(HASHMAP_BUCKET_VAL(bucket));

	return 0;
}

NOCH_DEF void *hashmapGet_(Hashmap *this, const char *key) {
	HashmapBucket *bucket = hashmapFind(this, key);
	if (bucket == NULL)
		return NULL;

	return HASHMAP_BUCKET_VAL(bucket);
}

static int hashmapResize(Hashmap *this, size_t newCap) {
	size_t         prev_cap     = this->cap;
	HashmapBucket *prev_buckets = this->buckets;

	this->cap     = newCap;
	this->buckets = nochAlloc(this->cap * this->bucketSize);
	if (this->buckets == NULL)
		NOCH_OUT_OF_MEM();

	memset(this->buckets, 0, this->cap * this->bucketSize);

	for (size_t i = 0; i < prev_cap * this->valueSize; i += this->valueSize) {
		HashmapBucket *bucket = (HashmapBucket*)((char*)prev_buckets + i);
		hashmapSet_(this, bucket->key, HASHMAP_BUCKET_VAL(bucket));
	}

	nochFree(prev_buckets);
	return 0;
}

NOCH_DEF int hashmapSet_(Hashmap *this, const char *key, void *value) {
	unsigned hash = this->hash(key);
	size_t   idx  = hash % this->cap;

	HashmapBucket *bucket = HASHMAP_BUCKET_AT(this, idx);

	if (bucket->full && !hashmapBucketMatches(bucket, hash, key)) {
		bool found = false;
		for (size_t i = idx + 1; i != idx; ++ i) {
			if (i >= this->cap)
				i = 0;

			bucket = HASHMAP_BUCKET_AT(this, i);
			if (!bucket->full) {
				found = true;
				break;
			}
		}

		if (!found) {
			if (hashmapResize(this, this->cap * 2) != 0)
				return -1;

			hashmapSet_(this, key, value);
			return 0;
		}
	}

	if (!bucket->full)
		bucket->full = true;
	else if (this->destruct != NULL)
		this->destruct(HASHMAP_BUCKET_VAL(bucket));

	bucket->key  = key;
	bucket->hash = hash;
	memcpy(HASHMAP_BUCKET_VAL(bucket), value, this->valueSize);
	return 0;
}

#undef HASHMAP_BUCKET_VAL
#undef HASHMAP_BUCKET_AT
