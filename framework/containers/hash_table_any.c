#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "hash_table_any.h"

struct Hash_Table_Any hash_table_any_init(uint32_t key_size, uint32_t value_size) {
	return (struct Hash_Table_Any){
		.key_size = key_size,
		.value_size = value_size,
	};
}

void hash_table_any_free(struct Hash_Table_Any * hash_table) {
	MEMORY_FREE(hash_table->hashes);
	MEMORY_FREE(hash_table->keys);
	MEMORY_FREE(hash_table->values);
	MEMORY_FREE(hash_table->marks);

	common_memset(hash_table, 0, sizeof(*hash_table));
}

static uint32_t hash_table_any_find_key_index(struct Hash_Table_Any const * hash_table, void const * key, uint32_t hash);
void hash_table_any_resize(struct Hash_Table_Any * hash_table, uint32_t target_capacity) {
	if (target_capacity < hash_table->count) {
		logger_to_console("limiting target resize capacity to the number of elements\n"); DEBUG_BREAK();
		target_capacity = hash_table->count;
	}

	uint32_t const capacity = hash_table->capacity;
	uint32_t * hashes = hash_table->hashes;
	uint8_t  * keys   = hash_table->keys;
	uint8_t  * values = hash_table->values;
	uint8_t  * marks  = hash_table->marks;

	hash_table->capacity = adjust_hash_table_capacity_value(target_capacity);
	hash_table->hashes   = MEMORY_ALLOCATE_ARRAY(uint32_t, hash_table->capacity);
	hash_table->keys     = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->key_size * hash_table->capacity);
	hash_table->values   = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->value_size * hash_table->capacity);
	hash_table->marks    = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->capacity);

	common_memset(hash_table->marks, HASH_TABLE_MARK_NONE, sizeof(*hash_table->marks) * hash_table->capacity);

	// @note: `hash_table->count` remains as is
	for (uint32_t i = 0; i < capacity; i++) {
		if (marks[i] != HASH_TABLE_MARK_FULL) { continue; }

		void const * ht_key = keys + hash_table->key_size * i;
		uint32_t const key_index = hash_table_any_find_key_index(hash_table, ht_key, hashes[i]);
		// if (key_index == INDEX_EMPTY) { DEBUG_BREAK(); continue; }
		// if (key_index >= hash_table->capacity) { DEBUG_BREAK(); continue; }

		hash_table->hashes[key_index] = hashes[i];
		common_memcpy(
			(uint8_t *)hash_table->keys + hash_table->key_size * key_index,
			ht_key,
			hash_table->key_size
		);
		common_memcpy(
			(uint8_t *)hash_table->values + hash_table->value_size * key_index,
			values + hash_table->value_size * i,
			hash_table->value_size
		);
		hash_table->marks[key_index] = HASH_TABLE_MARK_FULL;
	}

	MEMORY_FREE(hashes);
	MEMORY_FREE(keys);
	MEMORY_FREE(values);
	MEMORY_FREE(marks);
}

void hash_table_any_clear(struct Hash_Table_Any * hash_table) {
	hash_table->count = 0;
	common_memset(hash_table->marks, HASH_TABLE_MARK_NONE, sizeof(*hash_table->marks) * hash_table->capacity);
}

void * hash_table_any_get(struct Hash_Table_Any const * hash_table, void const * key, uint32_t hash) {
	if (key == NULL) {
		logger_to_console("hash table key should be non-null\n"); DEBUG_BREAK();
		return false;
	}

	if (hash_table->count == 0) { return NULL; }
	uint32_t const key_index = hash_table_any_find_key_index(hash_table, key, hash);
	// if (key_index == INDEX_EMPTY) { return NULL; }
	if (hash_table->marks[key_index] != HASH_TABLE_MARK_FULL) { return NULL; }
	return (uint8_t *)hash_table->values + hash_table->value_size * key_index;
}

bool hash_table_any_set(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash, void const * value) {
	if (key == NULL) {
		logger_to_console("hash table key should be non-null\n"); DEBUG_BREAK();
		return false;
	}

	if (should_hash_table_grow(hash_table->capacity, hash_table->count)) {
		uint32_t const target_capacity = grow_capacity_value_u32(hash_table->capacity, 1);
		hash_table_any_resize(hash_table, target_capacity);
	}

	uint32_t const key_index = hash_table_any_find_key_index(hash_table, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	uint8_t const mark = hash_table->marks[key_index];
	bool const is_new = (mark != HASH_TABLE_MARK_FULL);
	if (is_new) { hash_table->count++; }

	hash_table->hashes[key_index] = hash;
	common_memcpy(
		(uint8_t *)hash_table->keys + hash_table->key_size * key_index,
		key,
		hash_table->key_size
	);
	if (value != NULL) {
		common_memcpy(
			(uint8_t *)hash_table->values + hash_table->value_size * key_index,
			value,
			hash_table->value_size
		);
	}
	hash_table->marks[key_index] = HASH_TABLE_MARK_FULL;
	
	return is_new;
}

bool hash_table_any_del(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash) {
	if (key == NULL) {
		logger_to_console("hash table key should be non-null\n"); DEBUG_BREAK();
		return false;
	}

	if (hash_table->count == 0) { return false; }
	uint32_t const key_index = hash_table_any_find_key_index(hash_table, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	if (hash_table->marks[key_index] != HASH_TABLE_MARK_FULL) { return false; }
	hash_table->marks[key_index] = HASH_TABLE_MARK_SKIP;
	hash_table->count--;
	return true;
}

void hash_table_any_del_at(struct Hash_Table_Any * hash_table, uint32_t key_index) {
	if (key_index >= hash_table->capacity) { DEBUG_BREAK(); return; }
	if (hash_table->marks[key_index] != HASH_TABLE_MARK_FULL) { DEBUG_BREAK(); return; }
	hash_table->marks[key_index] = HASH_TABLE_MARK_SKIP;
	hash_table->count--;
}

bool hash_table_any_iterate(struct Hash_Table_Any const * hash_table, struct Hash_Table_Any_Iterator * iterator) {
	while (iterator->next < hash_table->capacity) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		if (hash_table->marks[index] != HASH_TABLE_MARK_FULL) { continue; }
		iterator->hash  = hash_table->hashes[index];
		iterator->key   = (uint8_t *)hash_table->keys   + hash_table->key_size * index;
		iterator->value = (uint8_t *)hash_table->values + hash_table->value_size * index;
		return true;
	}
	return false;
}

//

static uint32_t hash_table_any_find_key_index(struct Hash_Table_Any const * hash_table, void const * key, uint32_t hash) {
	uint32_t empty = INDEX_EMPTY;

	uint32_t const offset = HASH_TABLE_WRAP(hash, hash_table->capacity);
	for (uint32_t i = 0; i < hash_table->capacity; i++) {
		uint32_t const index = HASH_TABLE_WRAP(i + offset, hash_table->capacity);

		uint8_t const mark = hash_table->marks[index];
		if (mark == HASH_TABLE_MARK_SKIP) {
			if (empty == INDEX_EMPTY) { empty = index; }
			continue;
		}
		if (mark == HASH_TABLE_MARK_NONE) {
			if (empty == INDEX_EMPTY) { empty = index; }
			break;
		}

		if (hash_table->hashes[index] != hash) { continue; }

		void const * ht_key = (uint8_t *)hash_table->keys + hash_table->key_size * index;
		if (common_memcmp(ht_key, key, hash_table->key_size) == 0) { return index; }
	}

	return empty;
}
