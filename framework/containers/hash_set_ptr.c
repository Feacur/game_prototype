#include "framework/memory.h"
#include "framework/logger.h"
#include "internal/helpers.h"

//
#include "hash_set_ptr.h"

struct Hash_Set_Ptr hash_set_ptr_init(void) {
	return (struct Hash_Set_Ptr){0};
}

void hash_set_ptr_free(struct Hash_Set_Ptr * hash_set) {
	MEMORY_FREE(hash_set->key_hashes);
	MEMORY_FREE(hash_set->marks);
	common_memset(hash_set, 0, sizeof(*hash_set));
}

static uint32_t hash_set_ptr_find_key_index(struct Hash_Set_Ptr * hash_set, size_t key_hash);
void hash_set_ptr_resize(struct Hash_Set_Ptr * hash_set, uint32_t target_capacity) {
	if (target_capacity < hash_set->count) {
		logger_to_console("limiting target resize capacity to the number of elements\n"); DEBUG_BREAK();
		target_capacity = hash_set->count;
	}

	uint32_t const capacity = hash_set->capacity;
	size_t * key_hashes = hash_set->key_hashes;
	uint8_t  * marks      = hash_set->marks;

	hash_set->capacity   = adjust_hash_table_capacity_value(target_capacity);
	hash_set->key_hashes = MEMORY_ALLOCATE_ARRAY(size_t, hash_set->capacity);
	hash_set->marks      = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_set->capacity);

	common_memset(hash_set->marks, HASH_TABLE_MARK_NONE, sizeof(*hash_set->marks) * hash_set->capacity);

	// @note: `hash_set->count` remains as is
	for (uint32_t i = 0; i < capacity; i++) {
		if (marks[i] != HASH_TABLE_MARK_FULL) { continue; }

		uint32_t const key_index = hash_set_ptr_find_key_index(hash_set, key_hashes[i]);
		// if (key_index == INDEX_EMPTY) { DEBUG_BREAK(); continue; }
		// if (key_index >= hash_set->capacity) { DEBUG_BREAK(); continue; }

		hash_set->key_hashes[key_index] = key_hashes[i];
		hash_set->marks[key_index] = HASH_TABLE_MARK_FULL;
	}

	MEMORY_FREE(key_hashes);
	MEMORY_FREE(marks);
}

void hash_set_ptr_clear(struct Hash_Set_Ptr * hash_set) {
	hash_set->count = 0;
	common_memset(hash_set->marks, HASH_TABLE_MARK_NONE, sizeof(*hash_set->marks) * hash_set->capacity);
}

bool hash_set_ptr_get(struct Hash_Set_Ptr * hash_set, size_t key_hash) {
	if (hash_set->count == 0) { return false; }
	uint32_t const key_index = hash_set_ptr_find_key_index(hash_set, key_hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	if (hash_set->marks[key_index] != HASH_TABLE_MARK_FULL) { return false; }
	return true;
}

bool hash_set_ptr_set(struct Hash_Set_Ptr * hash_set, size_t key_hash) {
	if (should_hash_table_grow(hash_set->capacity, hash_set->count)) {
		uint32_t const target_capacity = grow_capacity_value_u32(hash_set->capacity, 1);
		hash_set_ptr_resize(hash_set, target_capacity);
	}

	uint32_t const key_index = hash_set_ptr_find_key_index(hash_set, key_hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	uint8_t const mark = hash_set->marks[key_index];
	bool const is_new = (mark != HASH_TABLE_MARK_FULL);
	if (is_new) { hash_set->count++; }

	hash_set->key_hashes[key_index] = key_hash;
	hash_set->marks[key_index] = HASH_TABLE_MARK_FULL;
	
	return is_new;
}

bool hash_set_ptr_del(struct Hash_Set_Ptr * hash_set, size_t key_hash) {
	if (hash_set->count == 0) { return false; }
	uint32_t const key_index = hash_set_ptr_find_key_index(hash_set, key_hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	if (hash_set->marks[key_index] != HASH_TABLE_MARK_FULL) { return false; }
	hash_set->marks[key_index] = HASH_TABLE_MARK_SKIP;
	hash_set->count--;
	return true;
}

void hash_set_ptr_del_at(struct Hash_Set_Ptr * hash_set, uint32_t key_index) {
	if (key_index >= hash_set->capacity) { DEBUG_BREAK(); return; }
	if (hash_set->marks[key_index] != HASH_TABLE_MARK_FULL) { DEBUG_BREAK(); return; }
	hash_set->marks[key_index] = HASH_TABLE_MARK_SKIP;
	hash_set->count--;
}

bool hash_set_ptr_iterate(struct Hash_Set_Ptr const * hash_set, struct Hash_Set_Ptr_Iterator * iterator) {
	while (iterator->next < hash_set->capacity) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		if (hash_set->marks[index] != HASH_TABLE_MARK_FULL) { continue; }
		iterator->key_hash = hash_set->key_hashes[index];
		return true;
	}
	return false;
}

//

static uint32_t hash_set_ptr_find_key_index(struct Hash_Set_Ptr * hash_set, size_t key_hash) {
	uint32_t empty = INDEX_EMPTY;

	uint32_t const offset = HASH_TABLE_WRAP(key_hash, hash_set->capacity);
	for (uint32_t i = 0; i < hash_set->capacity; i++) {
		uint32_t const index = HASH_TABLE_WRAP(i + offset, hash_set->capacity);

		uint8_t const mark = hash_set->marks[index];
		if (mark == HASH_TABLE_MARK_SKIP) {
			if (empty == INDEX_EMPTY) { empty = index; }
			continue;
		}
		if (mark == HASH_TABLE_MARK_NONE) {
			if (empty == INDEX_EMPTY) { empty = index; }
			break;
		}

		if (hash_set->key_hashes[index] == key_hash) { return index; }
	}

	return empty;
}
