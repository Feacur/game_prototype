#if !defined(FRAMEWORK_CONTAINERS_HASHTABLE)
#define FRAMEWORK_CONTAINERS_HASHTABLE

#include "framework/common.h"

struct Hashtable_Iterator {
	uint32_t current, next;
	uint32_t hash;
	void const * key;
	void * value;
};

// @idea: hash the key automatically as bytes array?
struct Hashtable {
	hasher * get_hash;
	uint32_t key_size, value_size;
	uint32_t capacity, count;
	uint32_t * hashes;
	void * keys;
	void * values;
	uint8_t * marks;
};

struct Hashtable hashtable_init(hasher * get_hash, uint32_t key_size, uint32_t value_size);
void hashtable_free(struct Hashtable * hashtable);

void hashtable_clear(struct Hashtable * hashtable);
void hashtable_resize(struct Hashtable * hashtable, uint32_t target_capacity);

void * hashtable_get(struct Hashtable const * hashtable, void const * key);
bool hashtable_set(struct Hashtable * hashtable, void const * key, void const * value);
bool hashtable_del(struct Hashtable * hashtable, void const * key);
void hashtable_del_at(struct Hashtable * hashtable, uint32_t key_index);

bool hashtable_iterate(struct Hashtable const * hashtable, struct Hashtable_Iterator * iterator);

#define FOR_HASHTABLE(data, it) for ( \
	struct Hashtable_Iterator it = {0}; \
	hashtable_iterate(data, &it); \
) \

#endif
