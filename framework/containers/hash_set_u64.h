#if !defined(FRAMEWORK_CONTAINERS_HASHSET_U64)
#define FRAMEWORK_CONTAINERS_HASHSET_U64

#include "framework/common.h"

struct Hash_Set_U64_Iterator {
	uint32_t current, next;
	uint64_t key_hash;
};

struct Hash_Set_U64 {
	uint32_t capacity, count;
	uint64_t * key_hashes;
	uint8_t * marks;
};

struct Hash_Set_U64 hash_set_u64_init(void);
void hash_set_u64_free(struct Hash_Set_U64 * hash_set);

void hash_set_u64_clear(struct Hash_Set_U64 * hash_set);
void hash_set_u64_resize(struct Hash_Set_U64 * hash_set, uint32_t target_capacity);

bool hash_set_u64_get(struct Hash_Set_U64 * hash_set, uint64_t key_hash);
bool hash_set_u64_set(struct Hash_Set_U64 * hash_set, uint64_t key_hash);
bool hash_set_u64_del(struct Hash_Set_U64 * hash_set, uint64_t key_hash);
void hash_set_u64_del_at(struct Hash_Set_U64 * hash_set, uint32_t key_index);

bool hash_set_u64_iterate(struct Hash_Set_U64 const * hash_set, struct Hash_Set_U64_Iterator * iterator);

#define FOR_HASH_SET_U64(data, it) for ( \
	struct Hash_Set_U64_Iterator it = {0}; \
	hash_set_u64_iterate(data, &it); \
) \

#endif
