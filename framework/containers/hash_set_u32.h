#if !defined(GAME_CONTAINTERS_HASHSET_U32)
#define GAME_CONTAINTERS_HASHSET_U32

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

void hash_set_u32_init(struct Hash_Set_U32 * hash_set);
void hash_set_u32_free(struct Hash_Set_U32 * hash_set);

void hash_set_u32_clear(struct Hash_Set_U32 * hash_set);
void hash_set_u32_resize(struct Hash_Set_U32 * hash_set, uint32_t target_capacity);

bool hash_set_u32_get(struct Hash_Set_U32 * hash_set, uint32_t key_hash);
bool hash_set_u32_set(struct Hash_Set_U32 * hash_set, uint32_t key_hash);
bool hash_set_u32_del(struct Hash_Set_U32 * hash_set, uint32_t key_hash);
void hash_set_u32_del_at(struct Hash_Set_U32 * hash_set, uint32_t key_index);

bool hash_set_u32_iterate(struct Hash_Set_U32 * hash_set, struct Hash_Set_U32_Iterator * entry);

#endif
