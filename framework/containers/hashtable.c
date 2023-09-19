#include "framework/memory.h"
#include "framework/logger.h"
#include "internal/helpers.h"

//
#include "hashtable.h"

struct Hashtable hashtable_init(hasher * get_hash, uint32_t key_size, uint32_t value_size) {
	return (struct Hashtable){
		.get_hash = get_hash,
		.key_size = key_size,
		.value_size = value_size,
	};
}

void hashtable_free(struct Hashtable * hashtable) {
	MEMORY_FREE(hashtable->hashes);
	MEMORY_FREE(hashtable->keys);
	MEMORY_FREE(hashtable->values);
	MEMORY_FREE(hashtable->marks);
	common_memset(hashtable, 0, sizeof(*hashtable));
}

static uint32_t hashtable_find_key_index(struct Hashtable const * hashtable, void const * key, uint32_t hash);
void hashtable_resize(struct Hashtable * hashtable, uint32_t target_capacity) {
	if (target_capacity < hashtable->count) {
		logger_to_console("limiting target resize capacity to the number of elements\n"); DEBUG_BREAK();
		target_capacity = hashtable->count;
	}

	uint32_t const capacity = hashtable->capacity;
	uint32_t * hashes = hashtable->hashes;
	uint8_t  * keys   = hashtable->keys;
	uint8_t  * values = hashtable->values;
	uint8_t  * marks  = hashtable->marks;

	hashtable->capacity = adjust_hashes_capacity_value(target_capacity);
	hashtable->hashes   = MEMORY_ALLOCATE_ARRAY(uint32_t, hashtable->capacity);
	hashtable->keys     = MEMORY_ALLOCATE_ARRAY(uint8_t, hashtable->key_size * hashtable->capacity);
	hashtable->values   = MEMORY_ALLOCATE_ARRAY(uint8_t, hashtable->value_size * hashtable->capacity);
	hashtable->marks    = MEMORY_ALLOCATE_ARRAY(uint8_t, hashtable->capacity);

	common_memset(hashtable->marks, HASH_TABLE_MARK_NONE, sizeof(*hashtable->marks) * hashtable->capacity);

	// @note: `hashtable->count` remains as is
	for (uint32_t i = 0; i < capacity; i++) {
		if (marks[i] != HASH_TABLE_MARK_FULL) { continue; }

		void const * ht_key = keys + hashtable->key_size * i;
		uint32_t const key_index = hashtable_find_key_index(hashtable, ht_key, hashes[i]);
		// if (key_index == INDEX_EMPTY) { DEBUG_BREAK(); continue; }
		// if (key_index >= hashtable->capacity) { DEBUG_BREAK(); continue; }

		hashtable->hashes[key_index] = hashes[i];
		common_memcpy(
			(uint8_t *)hashtable->keys + hashtable->key_size * key_index,
			ht_key,
			hashtable->key_size
		);
		common_memcpy(
			(uint8_t *)hashtable->values + hashtable->value_size * key_index,
			values + hashtable->value_size * i,
			hashtable->value_size
		);
		hashtable->marks[key_index] = HASH_TABLE_MARK_FULL;
	}

	MEMORY_FREE(hashes);
	MEMORY_FREE(keys);
	MEMORY_FREE(values);
	MEMORY_FREE(marks);
}

void hashtable_clear(struct Hashtable * hashtable) {
	hashtable->count = 0;
	common_memset(hashtable->marks, HASH_TABLE_MARK_NONE, sizeof(*hashtable->marks) * hashtable->capacity);
}

void * hashtable_get(struct Hashtable const * hashtable, void const * key) {
	if (key == NULL) {
		logger_to_console("hash table key should be non-null\n"); DEBUG_BREAK();
		return NULL;
	}

	if (hashtable->count == 0) { return NULL; }
	uint32_t const hash = hashtable->get_hash(key);
	uint32_t const key_index = hashtable_find_key_index(hashtable, key, hash);
	// if (key_index == INDEX_EMPTY) { return NULL; }
	if (hashtable->marks[key_index] != HASH_TABLE_MARK_FULL) { return NULL; }
	return (uint8_t *)hashtable->values + hashtable->value_size * key_index;
}

bool hashtable_set(struct Hashtable * hashtable, void const * key, void const * value) {
	if (key == NULL) {
		logger_to_console("hash table key should be non-null\n"); DEBUG_BREAK();
		return false;
	}

	if (should_hashes_grow(hashtable->capacity, hashtable->count)) {
		uint32_t const target_capacity = grow_capacity_value_u32(hashtable->capacity, 1);
		hashtable_resize(hashtable, target_capacity);
	}

	uint32_t const hash = hashtable->get_hash(key);
	uint32_t const key_index = hashtable_find_key_index(hashtable, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	uint8_t const mark = hashtable->marks[key_index];
	bool const is_new = (mark != HASH_TABLE_MARK_FULL);
	if (is_new) { hashtable->count++; }

	hashtable->hashes[key_index] = hash;
	common_memcpy(
		(uint8_t *)hashtable->keys + hashtable->key_size * key_index,
		key,
		hashtable->key_size
	);
	common_memcpy(
		(uint8_t *)hashtable->values + hashtable->value_size * key_index,
		value,
		hashtable->value_size
	);
	hashtable->marks[key_index] = HASH_TABLE_MARK_FULL;
	
	return is_new;
}

bool hashtable_del(struct Hashtable * hashtable, void const * key) {
	if (key == NULL) {
		logger_to_console("hash table key should be non-null\n"); DEBUG_BREAK();
		return false;
	}

	if (hashtable->count == 0) { return false; }
	uint32_t const hash = hashtable->get_hash(key);
	uint32_t const key_index = hashtable_find_key_index(hashtable, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	if (hashtable->marks[key_index] != HASH_TABLE_MARK_FULL) { return false; }
	hashtable->marks[key_index] = HASH_TABLE_MARK_SKIP;
	hashtable->count--;
	return true;
}

void hashtable_del_at(struct Hashtable * hashtable, uint32_t key_index) {
	if (key_index >= hashtable->capacity) { DEBUG_BREAK(); return; }
	if (hashtable->marks[key_index] != HASH_TABLE_MARK_FULL) { DEBUG_BREAK(); return; }
	hashtable->marks[key_index] = HASH_TABLE_MARK_SKIP;
	hashtable->count--;
}

bool hashtable_iterate(struct Hashtable const * hashtable, struct Hashtable_Iterator * iterator) {
	while (iterator->next < hashtable->capacity) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		if (hashtable->marks[index] != HASH_TABLE_MARK_FULL) { continue; }
		iterator->hash  = hashtable->hashes[index];
		iterator->key   = (uint8_t *)hashtable->keys   + hashtable->key_size * index;
		iterator->value = (uint8_t *)hashtable->values + hashtable->value_size * index;
		return true;
	}
	return false;
}

//

static uint32_t hashtable_find_key_index(struct Hashtable const * hashtable, void const * key, uint32_t hash) {
	uint32_t empty = INDEX_EMPTY;

	uint32_t const offset = HASH_TABLE_WRAP(hash, hashtable->capacity);
	for (uint32_t i = 0; i < hashtable->capacity; i++) {
		uint32_t const index = HASH_TABLE_WRAP(i + offset, hashtable->capacity);

		uint8_t const mark = hashtable->marks[index];
		if (mark == HASH_TABLE_MARK_SKIP) {
			if (empty == INDEX_EMPTY) { empty = index; }
			continue;
		}
		if (mark == HASH_TABLE_MARK_NONE) {
			if (empty == INDEX_EMPTY) { empty = index; }
			break;
		}

		if (hashtable->hashes[index] != hash) { continue; }

		void const * ht_key = (uint8_t *)hashtable->keys + hashtable->key_size * index;
		if (common_memcmp(ht_key, key, hashtable->key_size) == 0) { return index; }
	}

	return empty;
}
