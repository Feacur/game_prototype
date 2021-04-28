#if !defined(GAME_CONTAINTERS_HASHTABLE_U32)
#define GAME_CONTAINTERS_HASHTABLE_U32

#include "framework/common.h"

struct Hash_Table_U32 {
	uint32_t value_size;
	uint32_t capacity, count;
	uint32_t * key_hashes;
	uint8_t * values;
	uint8_t * marks;
};

void hash_table_u32_init(struct Hash_Table_U32 * hash_table, uint32_t value_size);
void hash_table_u32_free(struct Hash_Table_U32 * hash_table);

void hash_table_u32_clear(struct Hash_Table_U32 * hash_table);
void hash_table_u32_ensure_minimum_capacity(struct Hash_Table_U32 * hash_table, uint32_t minimum_capacity);

void * hash_table_u32_get(struct Hash_Table_U32 * hash_table, uint32_t key_hash);
bool hash_table_u32_set(struct Hash_Table_U32 * hash_table, uint32_t key_hash, void const * value);
bool hash_table_u32_del(struct Hash_Table_U32 * hash_table, uint32_t key_hash);

uint32_t hash_table_u32_get_iteration_capacity(struct Hash_Table_U32 * hash_table);
void * hash_table_u32_iterate(struct Hash_Table_U32 * hash_table, uint32_t index);

#endif
