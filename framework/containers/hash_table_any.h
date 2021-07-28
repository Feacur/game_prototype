#if !defined(GAME_CONTAINTERS_HASHTABLE_ANY)
#define GAME_CONTAINTERS_HASHTABLE_ANY

#include "framework/common.h"

struct Hash_Table_Any_Iterator {
	uint32_t current, next;
	uint32_t hash;
	void const * key;
	void * value;
};

// @idea: hash the key automatically as bytes array?
struct Hash_Table_Any {
	uint32_t key_size, value_size;
	uint32_t capacity, count;
	uint32_t * hashes;
	uint8_t * keys;
	uint8_t * values;
	uint8_t * marks;
};

void hash_table_any_init(struct Hash_Table_Any * hash_table, uint32_t key_size, uint32_t value_size);
void hash_table_any_free(struct Hash_Table_Any * hash_table);

void hash_table_any_clear(struct Hash_Table_Any * hash_table);
void hash_table_any_resize(struct Hash_Table_Any * hash_table, uint32_t target_capacity);

void * hash_table_any_get(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash);
bool hash_table_any_set(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash, void const * value);
bool hash_table_any_del(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash);
void hash_table_any_del_at(struct Hash_Table_Any * hash_table, uint32_t key_index);

bool hash_table_any_iterate(struct Hash_Table_Any * hash_table, struct Hash_Table_Any_Iterator * iterator);

#endif
