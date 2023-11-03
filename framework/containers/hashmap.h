#if !defined(FRAMEWORK_CONTAINERS_hashmap)
#define FRAMEWORK_CONTAINERS_hashmap

#include "framework/common.h"

struct Hashmap_Iterator {
	uint32_t curr, next;
	uint32_t hash;
	void const * key;
	void * value;
};

// @idea: hash the key automatically as bytes array?
struct Hashmap {
	Hasher * get_hash;
	uint32_t key_size, value_size;
	uint32_t capacity, count;
	uint32_t * hashes;
	void * keys;
	void * values;
	uint8_t * marks;
};

struct Hashmap hashmap_init(Hasher * get_hash, uint32_t key_size, uint32_t value_size);
void hashmap_free(struct Hashmap * hashmap);

void hashmap_clear(struct Hashmap * hashmap);
void hashmap_ensure(struct Hashmap * hashmap, uint32_t capacity);

void * hashmap_get(struct Hashmap const * hashmap, void const * key);
bool hashmap_set(struct Hashmap * hashmap, void const * key, void const * value);
bool hashmap_del(struct Hashmap * hashmap, void const * key);


uint32_t hashmap_get_count(struct Hashmap const * hashmap);
void * hashmap_get_key_at(struct Hashmap const * hashmap, uint32_t index);
void * hashmap_get_val_at(struct Hashmap const * hashmap, uint32_t index);
void hashmap_del_at(struct Hashmap * hashmap, uint32_t index);

bool hashmap_iterate(struct Hashmap const * hashmap, struct Hashmap_Iterator * iterator);

#define FOR_HASHMAP(data, it) for ( \
	struct Hashmap_Iterator it = {0}; \
	hashmap_iterate(data, &it); \
) \

#endif
