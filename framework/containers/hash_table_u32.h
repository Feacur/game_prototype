#if !defined(FRAMEWORK_CONTAINERS_HASHTABLE_U32)
#define FRAMEWORK_CONTAINERS_HASHTABLE_U32

#include "framework/common.h"

struct Hash_Table_U32_Iterator {
	uint32_t current, next;
	uint32_t key_hash;
	void * value;
};

struct Hash_Table_U32 {
	uint32_t value_size;
	uint32_t capacity, count;
	uint32_t * key_hashes;
	void * values;
	uint8_t * marks;
};

struct Hash_Table_U32 hash_table_u32_init(uint32_t value_size);
void hash_table_u32_free(struct Hash_Table_U32 * hash_table);

void hash_table_u32_clear(struct Hash_Table_U32 * hash_table);
void hash_table_u32_resize(struct Hash_Table_U32 * hash_table, uint32_t target_capacity);

void * hash_table_u32_get(struct Hash_Table_U32 const * hash_table, uint32_t key_hash);
bool hash_table_u32_set(struct Hash_Table_U32 * hash_table, uint32_t key_hash, void const * value);
bool hash_table_u32_del(struct Hash_Table_U32 * hash_table, uint32_t key_hash);
void hash_table_u32_del_at(struct Hash_Table_U32 * hash_table, uint32_t key_index);

bool hash_table_u32_iterate(struct Hash_Table_U32 const * hash_table, struct Hash_Table_U32_Iterator * iterator);

#define FOR_HASH_TABLE_U32(data, it) for ( \
	struct Hash_Table_U32_Iterator it = {0}; \
	hash_table_u32_iterate(data, &it); \
) \

#endif
