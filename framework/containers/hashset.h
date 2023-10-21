#if !defined(FRAMEWORK_CONTAINERS_HASHSET)
#define FRAMEWORK_CONTAINERS_HASHSET

#include "framework/common.h"

struct Hashset_Iterator {
	uint32_t curr, next;
	uint32_t hash;
	void const * key;
};

// @idea: hash the key automatically as bytes array?
struct Hashset {
	Hasher * get_hash;
	uint32_t key_size;
	uint32_t capacity, count;
	uint32_t * hashes;
	void * keys;
	uint8_t * marks;
};

struct Hashset hashset_init(Hasher * get_hash, uint32_t key_size);
void hashset_free(struct Hashset * hashset);

void hashset_clear(struct Hashset * hashset);
void hashset_ensure(struct Hashset * hashset, uint32_t capacity);

bool hashset_get(struct Hashset const * hashset, void const * key);
bool hashset_set(struct Hashset * hashset, void const * key);
bool hashset_del(struct Hashset * hashset, void const * key);

uint32_t hashset_get_count(struct Hashset * hashset);
void * hashset_get_at(struct Hashset * hashset, uint32_t index);
void hashset_del_at(struct Hashset * hashset, uint32_t index);

bool hashset_iterate(struct Hashset const * hashset, struct Hashset_Iterator * iterator);

#define FOR_HASHSET(data, it) for ( \
	struct Hashset_Iterator it = {0}; \
	hashset_iterate(data, &it); \
) \

#endif
