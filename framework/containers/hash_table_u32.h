#if !defined(GAME_HASHTABLE_U32)
#define GAME_HASHTABLE_U32

#include "framework/common.h"

// @idea: expose the structure? will it reduce indirection?
struct Hash_Table_U32;

struct Hash_Table_U32 * hash_table_u32_init(uint32_t value_size);
void hash_table_u32_free(struct Hash_Table_U32 * hash_table);

void hash_table_u32_clear(struct Hash_Table_U32 * hash_table);
void hash_table_u32_ensure_minimum_capacity(struct Hash_Table_U32 * hash_table, uint32_t minimum_capacity);

void * hash_table_u32_get(struct Hash_Table_U32 * hash_table, uint32_t key_hash);
bool hash_table_u32_set(struct Hash_Table_U32 * hash_table, uint32_t key_hash, void const * value);
bool hash_table_u32_del(struct Hash_Table_U32 * hash_table, uint32_t key_hash);

#endif
