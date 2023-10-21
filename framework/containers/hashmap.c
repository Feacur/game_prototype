#include "framework/memory.h"
#include "framework/logger.h"
#include "internal/helpers.h"

//
#include "hashmap.h"

struct Hashmap hashmap_init(hasher * get_hash, uint32_t key_size, uint32_t value_size) {
	return (struct Hashmap){
		.get_hash = get_hash,
		.key_size = key_size,
		.value_size = value_size,
	};
}

void hashmap_free(struct Hashmap * hashmap) {
	MEMORY_FREE(hashmap->hashes);
	MEMORY_FREE(hashmap->keys);
	MEMORY_FREE(hashmap->values);
	MEMORY_FREE(hashmap->marks);
	common_memset(hashmap, 0, sizeof(*hashmap));
}

static uint32_t hashmap_find_key_index(struct Hashmap const * hashmap, void const * key, uint32_t hash);
void hashmap_ensure(struct Hashmap * hashmap, uint32_t capacity) {
	if (!growth_hash_check(hashmap->capacity, capacity)) { return; }
	capacity = growth_hash_adjust(hashmap->capacity, capacity);

	if (capacity <= hashmap->capacity) {
		logger_to_console("can't grow\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	uint32_t * hashes = hashmap->hashes;
	uint8_t  * keys   = hashmap->keys;
	uint8_t  * values = hashmap->values;
	uint8_t  * marks  = hashmap->marks;

	hashmap->hashes = MEMORY_ALLOCATE_ARRAY(uint32_t, capacity);
	hashmap->keys   = MEMORY_ALLOCATE_ARRAY(uint8_t, hashmap->key_size * capacity);
	hashmap->values = MEMORY_ALLOCATE_ARRAY(uint8_t, hashmap->value_size * capacity);
	hashmap->marks  = MEMORY_ALLOCATE_ARRAY(uint8_t, capacity);

	common_memset(hashmap->marks, HASH_MARK_NONE, sizeof(*hashmap->marks) * capacity);

	// @note: `hashmap->count` remains as is
	uint32_t const prev_capacity = hashmap->capacity;
	hashmap->capacity = capacity;

	for (uint32_t i = 0; i < prev_capacity; i++) {
		if (marks[i] != HASH_MARK_FULL) { continue; }

		void const * ht_key = keys + hashmap->key_size * i;
		uint32_t const key_index = hashmap_find_key_index(hashmap, ht_key, hashes[i]);
		// if (key_index == INDEX_EMPTY) { REPORT_CALLSTACK(); DEBUG_BREAK(); continue; }
		// if (key_index >= capacity)    { REPORT_CALLSTACK(); DEBUG_BREAK(); continue; }

		hashmap->hashes[key_index] = hashes[i];
		common_memcpy(
			(uint8_t *)hashmap->keys + hashmap->key_size * key_index,
			ht_key,
			hashmap->key_size
		);
		common_memcpy(
			(uint8_t *)hashmap->values + hashmap->value_size * key_index,
			values + hashmap->value_size * i,
			hashmap->value_size
		);
		hashmap->marks[key_index] = HASH_MARK_FULL;
	}

	MEMORY_FREE(hashes);
	MEMORY_FREE(keys);
	MEMORY_FREE(values);
	MEMORY_FREE(marks);
}

void hashmap_clear(struct Hashmap * hashmap) {
	hashmap->count = 0;
	common_memset(hashmap->marks, HASH_MARK_NONE, sizeof(*hashmap->marks) * hashmap->capacity);
}

void * hashmap_get(struct Hashmap const * hashmap, void const * key) {
	if (key == NULL) {
		logger_to_console("key should be non-null\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}

	if (hashmap->count == 0) { return NULL; }
	uint32_t const hash = hashmap->get_hash(key);
	uint32_t const key_index = hashmap_find_key_index(hashmap, key, hash);
	// if (key_index == INDEX_EMPTY) { return NULL; }
	if (hashmap->marks[key_index] != HASH_MARK_FULL) { return NULL; }
	return (uint8_t *)hashmap->values + hashmap->value_size * key_index;
}

bool hashmap_set(struct Hashmap * hashmap, void const * key, void const * value) {
	if (key == NULL) {
		logger_to_console("key should be non-null\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return false;
	}

	hashmap_ensure(hashmap, hashmap->count + 1);

	uint32_t const hash = hashmap->get_hash(key);
	uint32_t const key_index = hashmap_find_key_index(hashmap, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	uint8_t const mark = hashmap->marks[key_index];
	bool const is_new = (mark != HASH_MARK_FULL);
	if (is_new) { hashmap->count++; }

	hashmap->hashes[key_index] = hash;
	common_memcpy(
		(uint8_t *)hashmap->keys + hashmap->key_size * key_index,
		key,
		hashmap->key_size
	);
	common_memcpy(
		(uint8_t *)hashmap->values + hashmap->value_size * key_index,
		value,
		hashmap->value_size
	);
	hashmap->marks[key_index] = HASH_MARK_FULL;
	
	return is_new;
}

bool hashmap_del(struct Hashmap * hashmap, void const * key) {
	if (key == NULL) {
		logger_to_console("key should be non-null\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return false;
	}

	if (hashmap->count == 0) { return false; }
	uint32_t const hash = hashmap->get_hash(key);
	uint32_t const key_index = hashmap_find_key_index(hashmap, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	if (hashmap->marks[key_index] != HASH_MARK_FULL) { return false; }
	hashmap->marks[key_index] = HASH_MARK_SKIP;
	hashmap->count--;
	return true;
}

uint32_t hashmap_get_count(struct Hashmap * hashmap) {
	return hashmap->count;
}

void * hashmap_get_key_at(struct Hashmap * hashmap, uint32_t index) {
	if (index >= hashmap->count) {
		logger_to_console("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	size_t const offset = hashmap->key_size * index;
	return (uint8_t *)hashmap->keys + offset;
}

void * hashmap_get_val_at(struct Hashmap * hashmap, uint32_t index) {
	if (index >= hashmap->count) {
		logger_to_console("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	size_t const offset = hashmap->value_size * index;
	return (uint8_t *)hashmap->values + offset;
}

void hashmap_del_at(struct Hashmap * hashmap, uint32_t index) {
	if (index >= hashmap->capacity)              { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (hashmap->marks[index] != HASH_MARK_FULL) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	hashmap->marks[index] = HASH_MARK_SKIP;
	hashmap->count--;
}

bool hashmap_iterate(struct Hashmap const * hashmap, struct Hashmap_Iterator * iterator) {
	while (iterator->next < hashmap->capacity) {
		uint32_t const index = iterator->next++;
		iterator->curr = index;
		if (hashmap->marks[index] != HASH_MARK_FULL) { continue; }
		iterator->hash  = hashmap->hashes[index];
		iterator->key   = (uint8_t *)hashmap->keys   + hashmap->key_size * index;
		iterator->value = (uint8_t *)hashmap->values + hashmap->value_size * index;
		return true;
	}
	return false;
}

//

static uint32_t hashmap_find_key_index(struct Hashmap const * hashmap, void const * key, uint32_t hash) {
	uint32_t empty = INDEX_EMPTY;

	uint32_t const offset = HASH_TABLE_WRAP(hash, hashmap->capacity);
	for (uint32_t i = 0; i < hashmap->capacity; i++) {
		uint32_t const index = HASH_TABLE_WRAP(i + offset, hashmap->capacity);

		uint8_t const mark = hashmap->marks[index];
		if (mark == HASH_MARK_SKIP) {
			if (empty == INDEX_EMPTY) { empty = index; }
			continue;
		}
		if (mark == HASH_MARK_NONE) {
			if (empty == INDEX_EMPTY) { empty = index; }
			break;
		}

		if (hashmap->hashes[index] != hash) { continue; }

		void const * ht_key = (uint8_t *)hashmap->keys + hashmap->key_size * index;
		if (common_memcmp(ht_key, key, hashmap->key_size) == 0) { return index; }
	}

	return empty;
}
