#if !defined(GAME_CONTAINERS_HASHSET_U32)
#define GAME_CONTAINERS_HASHSET_U32

#include "framework/common.h"

struct Hash_Set_U32_Iterator {
	uint32_t current, next;
	uint32_t key_hash;
};

struct Hash_Set_U32 {
	uint32_t capacity, count;
	uint32_t * key_hashes;
	uint8_t * marks;
};

struct Hash_Set_U32 hash_set_u32_init(void);
void hash_set_u32_free(struct Hash_Set_U32 * hash_set);

void hash_set_u32_clear(struct Hash_Set_U32 * hash_set);
void hash_set_u32_resize(struct Hash_Set_U32 * hash_set, uint32_t target_capacity);

bool hash_set_u32_get(struct Hash_Set_U32 * hash_set, uint32_t key_hash);
bool hash_set_u32_set(struct Hash_Set_U32 * hash_set, uint32_t key_hash);
bool hash_set_u32_del(struct Hash_Set_U32 * hash_set, uint32_t key_hash);
void hash_set_u32_del_at(struct Hash_Set_U32 * hash_set, uint32_t key_index);

bool hash_set_u32_iterate(struct Hash_Set_U32 * hash_set, struct Hash_Set_U32_Iterator * iterator);

#define FOR_HASH_SET_U32(data, it) for ( \
	struct Hash_Set_U32_Iterator it = {0}; \
	hash_set_u32_iterate(data, &it); \
) \

#endif
