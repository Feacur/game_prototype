#if !defined(GAME_HASHTABLE)
#define GAME_HASHTABLE

#include "framework/common.h"

struct Hash_Table;

struct Hash_Table * hash_table_init(uint32_t value_size);
void hash_table_free(struct Hash_Table * hash_table);

void hash_table_clear(struct Hash_Table * hash_table);

void * hash_table_get(struct Hash_Table * hash_table, uint32_t key);
bool hash_table_set(struct Hash_Table * hash_table, uint32_t key, void const * value);

#endif
