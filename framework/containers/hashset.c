#include "framework/memory.h"
#include "framework/formatter.h"
#include "internal/helpers.h"

//
#include "hashset.h"

struct Hashset hashset_init(Hasher * get_hash, uint32_t key_size) {
	return (struct Hashset){
		.get_hash = get_hash,
		.key_size = key_size,
	};
}

void hashset_free(struct Hashset * hashset) {
	MEMORY_FREE(hashset->hashes);
	MEMORY_FREE(hashset->keys);
	MEMORY_FREE(hashset->marks);
	common_memset(hashset, 0, sizeof(*hashset));
}

static uint32_t hashset_find_key_index(struct Hashset const * hashset, void const * key, uint32_t hash);
void hashset_ensure(struct Hashset * hashset, uint32_t capacity) {
	if (!growth_hash_check(hashset->capacity, capacity)) { return; }
	capacity = growth_hash_adjust(hashset->capacity, capacity);

	if (capacity <= hashset->capacity) {
		LOG("can't grow\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	uint32_t * hashes = hashset->hashes;
	uint8_t  * keys   = hashset->keys;
	uint8_t  * marks  = hashset->marks;

	hashset->hashes = MEMORY_ALLOCATE_ARRAY(uint32_t, capacity);
	hashset->keys   = MEMORY_ALLOCATE_ARRAY(uint8_t, hashset->key_size * capacity);
	hashset->marks  = MEMORY_ALLOCATE_ARRAY(uint8_t, capacity);

	common_memset(hashset->marks, HASH_MARK_NONE, sizeof(*hashset->marks) * capacity);

	// @note: `hashset->count` remains as is
	uint32_t const prev_capacity = hashset->capacity;
	hashset->capacity = capacity;

	for (uint32_t i = 0; i < prev_capacity; i++) {
		if (marks[i] != HASH_MARK_FULL) { continue; }

		void const * ht_key = keys + hashset->key_size * i;
		uint32_t const key_index = hashset_find_key_index(hashset, ht_key, hashes[i]);
		// if (key_index == INDEX_EMPTY) { REPORT_CALLSTACK(); DEBUG_BREAK(); continue; }
		// if (key_index >= capacity)    { REPORT_CALLSTACK(); DEBUG_BREAK(); continue; }

		hashset->hashes[key_index] = hashes[i];
		common_memcpy(
			(uint8_t *)hashset->keys + hashset->key_size * key_index,
			ht_key,
			hashset->key_size
		);
		hashset->marks[key_index] = HASH_MARK_FULL;
	}

	MEMORY_FREE(hashes);
	MEMORY_FREE(keys);
	MEMORY_FREE(marks);
}

void hashset_clear(struct Hashset * hashset) {
	hashset->count = 0;
	common_memset(hashset->marks, HASH_MARK_NONE, sizeof(*hashset->marks) * hashset->capacity);
}

bool hashset_get(struct Hashset const * hashset, void const * key) {
	if (key == NULL) {
		LOG("key should be non-null\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return false;
	}

	if (hashset->count == 0) { return false; }
	uint32_t const hash = hashset->get_hash(key);
	uint32_t const key_index = hashset_find_key_index(hashset, key, hash);
	// if (key_index == INDEX_EMPTY) { return NULL; }
	if (hashset->marks[key_index] != HASH_MARK_FULL) { return false; }
	return true;
}

bool hashset_set(struct Hashset * hashset, void const * key) {
	if (key == NULL) {
		LOG("key should be non-null\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return false;
	}

	hashset_ensure(hashset, hashset->count + 1);

	uint32_t const hash = hashset->get_hash(key);
	uint32_t const key_index = hashset_find_key_index(hashset, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	uint8_t const mark = hashset->marks[key_index];
	bool const is_new = (mark != HASH_MARK_FULL);
	if (is_new) { hashset->count++; }

	hashset->hashes[key_index] = hash;
	common_memcpy(
		(uint8_t *)hashset->keys + hashset->key_size * key_index,
		key,
		hashset->key_size
	);
	hashset->marks[key_index] = HASH_MARK_FULL;
	
	return is_new;
}

bool hashset_del(struct Hashset * hashset, void const * key) {
	if (key == NULL) {
		LOG("key should be non-null\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return false;
	}

	if (hashset->count == 0) { return false; }
	uint32_t const hash = hashset->get_hash(key);
	uint32_t const key_index = hashset_find_key_index(hashset, key, hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	if (hashset->marks[key_index] != HASH_MARK_FULL) { return false; }
	hashset->marks[key_index] = HASH_MARK_SKIP;
	hashset->count--;
	return true;
}

uint32_t hashset_get_count(struct Hashset * hashset) {
	return hashset->count;
}

void * hashset_get_at(struct Hashset * hashset, uint32_t index) {
	if (index >= hashset->count) {
		LOG("out of bounds\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	size_t const offset = hashset->key_size * index;
	return (uint8_t *)hashset->keys + offset;
}

void hashset_del_at(struct Hashset * hashset, uint32_t index) {
	if (index >= hashset->capacity)              { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (hashset->marks[index] != HASH_MARK_FULL) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	hashset->marks[index] = HASH_MARK_SKIP;
	hashset->count--;
}

bool hashset_iterate(struct Hashset const * hashset, struct Hashset_Iterator * iterator) {
	while (iterator->next < hashset->capacity) {
		uint32_t const index = iterator->next++;
		iterator->curr = index;
		if (hashset->marks[index] != HASH_MARK_FULL) { continue; }
		iterator->hash  = hashset->hashes[index];
		iterator->key   = (uint8_t *)hashset->keys   + hashset->key_size * index;
		return true;
	}
	return false;
}

//

static uint32_t hashset_find_key_index(struct Hashset const * hashset, void const * key, uint32_t hash) {
	uint32_t empty = INDEX_EMPTY;

	uint32_t const offset = HASH_TABLE_WRAP(hash, hashset->capacity);
	for (uint32_t i = 0; i < hashset->capacity; i++) {
		uint32_t const index = HASH_TABLE_WRAP(i + offset, hashset->capacity);

		uint8_t const mark = hashset->marks[index];
		if (mark == HASH_MARK_SKIP) {
			if (empty == INDEX_EMPTY) { empty = index; }
			continue;
		}
		if (mark == HASH_MARK_NONE) {
			if (empty == INDEX_EMPTY) { empty = index; }
			break;
		}

		if (hashset->hashes[index] != hash) { continue; }

		void const * ht_key = (uint8_t *)hashset->keys + hashset->key_size * index;
		if (common_memcmp(ht_key, key, hashset->key_size) == 0) { return index; }
	}

	return empty;
}
