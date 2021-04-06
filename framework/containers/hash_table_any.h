#if !defined(GAME_HASHTABLE_ANY)
#define GAME_HASHTABLE_ANY

#include "framework/common.h"

// @idea: expose the structure? will it reduce indirection?
// @idea: hash the key automatically as bytes array?
struct Hash_Table_Any;

struct Hash_Table_Any * hash_table_any_init(uint32_t key_size, uint32_t value_size);
void hash_table_any_free(struct Hash_Table_Any * hash_table);

void hash_table_any_clear(struct Hash_Table_Any * hash_table);
void hash_table_any_ensure_minimum_capacity(struct Hash_Table_Any * hash_table, uint32_t minimum_capacity);

void * hash_table_any_get(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash);
bool hash_table_any_set(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash, void const * value);
bool hash_table_any_del(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash);

uint32_t hash_table_any_get_iteration_capacity(struct Hash_Table_Any * hash_table);
void * hash_table_any_iterate(struct Hash_Table_Any * hash_table, uint32_t index);

#endif
