#if !defined(FRAMEWORK_CONTAINERS_HASHSET_PTR)
#define FRAMEWORK_CONTAINERS_HASHSET_PTR

#include "framework/common.h"

struct Hash_Set_Ptr_Iterator {
	uint32_t current, next;
	size_t key_hash;
};

struct Hash_Set_Ptr {
	uint32_t capacity, count;
	size_t * key_hashes;
	uint8_t * marks;
};

struct Hash_Set_Ptr hash_set_ptr_init(void);
void hash_set_ptr_free(struct Hash_Set_Ptr * hash_set);

void hash_set_ptr_clear(struct Hash_Set_Ptr * hash_set);
void hash_set_ptr_resize(struct Hash_Set_Ptr * hash_set, uint32_t target_capacity);

bool hash_set_ptr_get(struct Hash_Set_Ptr * hash_set, size_t key_hash);
bool hash_set_ptr_set(struct Hash_Set_Ptr * hash_set, size_t key_hash);
bool hash_set_ptr_del(struct Hash_Set_Ptr * hash_set, size_t key_hash);
void hash_set_ptr_del_at(struct Hash_Set_Ptr * hash_set, uint32_t key_index);

bool hash_set_ptr_iterate(struct Hash_Set_Ptr const * hash_set, struct Hash_Set_Ptr_Iterator * iterator);

#define FOR_HASH_SET_PTR(data, it) for ( \
	struct Hash_Set_Ptr_Iterator it = {0}; \
	hash_set_ptr_iterate(data, &it); \
) \

#endif
