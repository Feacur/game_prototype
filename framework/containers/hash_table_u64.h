#if !defined(GAME_CONTAINERS_HASHTABLE_U64)
#define GAME_CONTAINERS_HASHTABLE_U64

#include "framework/common.h"

struct Hash_Table_U64_Iterator {
	uint32_t current, next;
	uint64_t key_hash;
	void * value;
};

struct Hash_Table_U64 {
	uint32_t value_size;
	uint32_t capacity, count;
	uint64_t * key_hashes;
	void * values;
	uint8_t * marks;
};

struct Hash_Table_U64 hash_table_u64_init(uint32_t value_size);
void hash_table_u64_free(struct Hash_Table_U64 * hash_table);

void hash_table_u64_clear(struct Hash_Table_U64 * hash_table);
void hash_table_u64_resize(struct Hash_Table_U64 * hash_table, uint32_t target_capacity);

void * hash_table_u64_get(struct Hash_Table_U64 const * hash_table, uint64_t key_hash);
bool hash_table_u64_set(struct Hash_Table_U64 * hash_table, uint64_t key_hash, void const * value);
bool hash_table_u64_del(struct Hash_Table_U64 * hash_table, uint64_t key_hash);
void hash_table_u64_del_at(struct Hash_Table_U64 * hash_table, uint32_t key_index);

bool hash_table_u64_iterate(struct Hash_Table_U64 const * hash_table, struct Hash_Table_U64_Iterator * iterator);

#define FOR_HASH_TABLE_U64(data, it) for ( \
	struct Hash_Table_U64_Iterator it = {0}; \
	hash_table_u64_iterate(data, &it); \
) \

#endif
