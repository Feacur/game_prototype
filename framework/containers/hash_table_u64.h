#if !defined(GAME_CONTAINTERS_HASHTABLE_U64)
#define GAME_CONTAINTERS_HASHTABLE_U64

#include "framework/common.h"

struct Hash_Table_U64 {
	uint32_t value_size;
	uint32_t capacity, count;
	uint64_t * key_hashes;
	uint8_t * values;
	uint8_t * marks;
};

void hash_table_u64_init(struct Hash_Table_U64 * hash_table, uint32_t value_size);
void hash_table_u64_free(struct Hash_Table_U64 * hash_table);

void hash_table_u64_clear(struct Hash_Table_U64 * hash_table);
void hash_table_u64_resize(struct Hash_Table_U64 * hash_table, uint32_t target_capacity);

void * hash_table_u64_get(struct Hash_Table_U64 * hash_table, uint64_t key_hash);
bool hash_table_u64_set(struct Hash_Table_U64 * hash_table, uint64_t key_hash, void const * value);
bool hash_table_u64_del(struct Hash_Table_U64 * hash_table, uint64_t key_hash);

uint32_t hash_table_u64_get_iteration_capacity(struct Hash_Table_U64 * hash_table);
void * hash_table_u64_iterate(struct Hash_Table_U64 * hash_table, uint32_t index);

#endif
