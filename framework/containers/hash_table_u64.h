#if !defined(GAME_HASHTABLE_U64)
#define GAME_HASHTABLE_U64

#include "framework/common.h"

// @idea: expose the structure? will it reduce indirection?
struct Hash_Table_U64;

struct Hash_Table_U64 * hash_table_u64_init(uint32_t value_size);
void hash_table_u64_free(struct Hash_Table_U64 * hash_table);

void hash_table_u64_clear(struct Hash_Table_U64 * hash_table);
void hash_table_u64_ensure_minimum_capacity(struct Hash_Table_U64 * hash_table, uint32_t minimum_capacity);

void * hash_table_u64_get(struct Hash_Table_U64 * hash_table, uint64_t key_hash);
bool hash_table_u64_set(struct Hash_Table_U64 * hash_table, uint64_t key_hash, void const * value);
bool hash_table_u64_del(struct Hash_Table_U64 * hash_table, uint64_t key_hash);

uint32_t hash_table_u64_get_iteration_capacity(struct Hash_Table_U64 * hash_table);
void * hash_table_u64_iterate(struct Hash_Table_U64 * hash_table, uint32_t index);

#endif
