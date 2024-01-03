#include "framework/formatter.h"
#include "framework/systems/memory.h"

#include "internal/helpers.h"


//
#include "hashmap.h"

static void * hashmap_get_key_at_unsafe(struct Hashmap const * hashmap, uint32_t index) {
	size_t const offset = hashmap->key_size * index;
	return (uint8_t *)hashmap->keys + offset;
}

static void * hashmap_get_val_at_unsafe(struct Hashmap const * hashmap, uint32_t index) {
	// @note: places that call it already do the check
	// if (index >= hashmap->capacity) { return NULL; }
	size_t const offset = hashmap->value_size * index;
	return (uint8_t *)hashmap->values + offset;
}

struct Hashmap hashmap_init(Hasher * get_hash, uint32_t key_size, uint32_t value_size) {
	return (struct Hashmap){
		.get_hash = get_hash,
		.key_size = key_size,
		.value_size = value_size,
	};
}

void hashmap_free(struct Hashmap * hashmap) {
	if (hashmap->allocate == NULL) { return; }
	hashmap->allocate(hashmap->hashes, 0);
	hashmap->allocate(hashmap->keys,   0);
	hashmap->allocate(hashmap->values, 0);
	hashmap->allocate(hashmap->marks,  0);
	zero_out(AMP_(hashmap));
}

void hashmap_clear(struct Hashmap * hashmap, bool deallocate) {
	if (hashmap->allocate == NULL) { return; }
	if (deallocate) {
		hashmap->allocate(hashmap->hashes, 0); hashmap->hashes = NULL;
		hashmap->allocate(hashmap->keys,   0); hashmap->keys   = NULL;
		hashmap->allocate(hashmap->values, 0); hashmap->values = NULL;
		hashmap->allocate(hashmap->marks,  0); hashmap->marks  = NULL;
		hashmap->capacity = 0;
	}
	hashmap->count = 0;
	zero_out((struct CArray_Mut){
		.size = sizeof(*hashmap->marks) * hashmap->capacity,
		.data = hashmap->marks,
	});
}

static uint32_t hashmap_find_key_index(struct Hashmap const * hashmap, void const * key, uint32_t hash);
void hashmap_ensure(struct Hashmap * hashmap, uint32_t capacity) {
	if (!growth_hash_check(hashmap->capacity, capacity)) { return; }
	if (hashmap->allocate == NULL) {
		hashmap->allocate = DEFAULT_REALLOCATOR;
	}

	capacity = growth_hash_adjust(hashmap->capacity, capacity);

	if (capacity <= hashmap->capacity) {
		ERR("can't grow");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	uint32_t * hashes = hashmap->hashes;
	uint8_t  * keys   = hashmap->keys;
	uint8_t  * values = hashmap->values;
	uint8_t  * marks  = hashmap->marks;

	hashmap->hashes = hashmap->allocate(NULL, sizeof(uint32_t) * capacity);
	hashmap->keys   = hashmap->allocate(NULL, sizeof(uint8_t)  * capacity * hashmap->key_size);
	hashmap->values = hashmap->allocate(NULL, sizeof(uint8_t)  * capacity * hashmap->value_size);
	hashmap->marks  = hashmap->allocate(NULL, sizeof(uint8_t)  * capacity);

	zero_out((struct CArray_Mut){
		.size = sizeof(*hashmap->marks) * capacity,
		.data = hashmap->marks,
	});

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
			hashmap_get_key_at_unsafe(hashmap, key_index),
			ht_key,
			hashmap->key_size
		);
		common_memcpy(
			hashmap_get_val_at_unsafe(hashmap, key_index),
			values + hashmap->value_size * i,
			hashmap->value_size
		);
		hashmap->marks[key_index] = HASH_MARK_FULL;
	}

	hashmap->allocate(hashes, 0);
	hashmap->allocate(keys,   0);
	hashmap->allocate(values, 0);
	hashmap->allocate(marks,  0);
}

void * hashmap_get(struct Hashmap const * hashmap, void const * key) {
	if (key == NULL) {
		ERR("key should be non-null");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}

	if (hashmap->count == 0) { return NULL; }
	uint32_t const hash = hashmap->get_hash(key);
	uint32_t const key_index = hashmap_find_key_index(hashmap, key, hash);
	// if (key_index == INDEX_EMPTY) { return NULL; }
	if (hashmap->marks[key_index] != HASH_MARK_FULL) { return NULL; }
	return hashmap_get_val_at_unsafe(hashmap, key_index);
}

bool hashmap_set(struct Hashmap * hashmap, void const * key, void const * value) {
	if (key == NULL) {
		ERR("key should be non-null");
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
		hashmap_get_key_at_unsafe(hashmap, key_index),
		key,
		hashmap->key_size
	);
	common_memcpy(
		hashmap_get_val_at_unsafe(hashmap, key_index),
		value,
		hashmap->value_size
	);
	hashmap->marks[key_index] = HASH_MARK_FULL;
	
	return is_new;
}

bool hashmap_del(struct Hashmap * hashmap, void const * key) {
	if (key == NULL) {
		ERR("key should be non-null");
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

uint32_t hashmap_get_count(struct Hashmap const * hashmap) {
	return hashmap->count;
}

void * hashmap_get_key_at(struct Hashmap const * hashmap, uint32_t index) {
	if (index >= hashmap->capacity)              { REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL; }
	if (hashmap->marks[index] != HASH_MARK_FULL) { return NULL; }
	return hashmap_get_key_at_unsafe(hashmap, index);
}

void * hashmap_get_val_at(struct Hashmap const * hashmap, uint32_t index) {
	if (index >= hashmap->capacity)              { REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL; }
	if (hashmap->marks[index] != HASH_MARK_FULL) { return NULL; }
	return hashmap_get_val_at_unsafe(hashmap, index);
}

void hashmap_del_at(struct Hashmap * hashmap, uint32_t index) {
	if (index >= hashmap->capacity)              { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (hashmap->marks[index] != HASH_MARK_FULL) { return; }
	hashmap->marks[index] = HASH_MARK_SKIP;
	hashmap->count--;
}

bool hashmap_iterate(struct Hashmap const * hashmap, struct Hashmap_Iterator * iterator) {
	while (iterator->next < hashmap->capacity) {
		uint32_t const index = iterator->next++;
		if (hashmap->marks[index] != HASH_MARK_FULL) { continue; }
		iterator->curr = index;
		iterator->hash  = hashmap->hashes[index];
		iterator->key   = hashmap_get_key_at_unsafe(hashmap, index);
		iterator->value = hashmap_get_val_at_unsafe(hashmap, index);
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

		void const * ht_key = hashmap_get_key_at_unsafe(hashmap, index);
		if (equals(ht_key, key, hashmap->key_size)) { return index; }
	}

	return empty;
}
