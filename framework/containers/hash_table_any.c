#include "framework/memory.h"

#include <stdio.h>
#include <string.h>

// @todo: make this dynamically compilable?
// @todo: make it easier to use another growth factor
#define GROWTH_FACTOR 2
#define HASH_TABLE_SHOULD_GROW(count, capacity) ((count) > (capacity) * 2 / 3)
// #define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

enum Hash_Table_Any_Mark {
	HASH_TABLE_ANY_MARK_NONE,
	HASH_TABLE_ANY_MARK_SKIP,
	HASH_TABLE_ANY_MARK_FULL,
};

struct Hash_Table_Any {
	uint32_t key_size, value_size;
	uint32_t capacity, count;
	uint32_t * hashes;
	uint8_t * keys;
	uint8_t * values;
	uint8_t * marks;
};

#if GROWTH_FACTOR == 2
	// #include "framework/maths.h"
	uint32_t round_up_to_PO2_u32(uint32_t value);
#endif

//
#include "hash_table_any.h"

struct Hash_Table_Any * hash_table_any_init(uint32_t key_size, uint32_t value_size) {
	if (key_size == 0) {
		key_size = 1;
		fprintf(stderr, "key size should be non-zero\n"); DEBUG_BREAK(); return NULL;
	}

	if (value_size == 0) {
		value_size = 1;
		fprintf(stderr, "value size should be non-zero\n"); DEBUG_BREAK(); return NULL;
	}

	struct Hash_Table_Any * hash_table = MEMORY_ALLOCATE(struct Hash_Table_Any);
	*hash_table = (struct Hash_Table_Any){
		.key_size = key_size,
		.value_size = value_size,
	};
	return hash_table;
}

void hash_table_any_free(struct Hash_Table_Any * hash_table) {
	MEMORY_FREE(hash_table->hashes);
	MEMORY_FREE(hash_table->keys);
	MEMORY_FREE(hash_table->values);
	MEMORY_FREE(hash_table->marks);

	memset(hash_table, 0, sizeof(*hash_table));
	MEMORY_FREE(hash_table);
}

static uint32_t hash_table_any_find_key_index(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash);
void hash_table_any_ensure_minimum_capacity(struct Hash_Table_Any * hash_table, uint32_t minimum_capacity) {
	if (minimum_capacity < 8) { minimum_capacity = 8; }
#if GROWTH_FACTOR == 2
	if (minimum_capacity > 0x80000000) {
		minimum_capacity = 0x80000000;
		fprintf(stderr, "requested capacity is too large\n"); DEBUG_BREAK();
	}
	minimum_capacity = round_up_to_PO2_u32(minimum_capacity);
#endif

	uint32_t const capacity = hash_table->capacity;
	uint32_t * hashes = hash_table->hashes;
	uint8_t * keys = hash_table->keys;
	uint8_t * values = hash_table->values;
	uint8_t * marks = hash_table->marks;

	hash_table->capacity = minimum_capacity;
	hash_table->hashes = MEMORY_ALLOCATE_ARRAY(uint32_t, hash_table->capacity);
	hash_table->keys = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->capacity * hash_table->key_size);
	hash_table->values = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->capacity * hash_table->value_size);
	hash_table->marks = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->capacity);

	memset(hash_table->marks, HASH_TABLE_ANY_MARK_NONE, hash_table->capacity * sizeof(*hash_table->marks));

	// @note: .count remains as is
	for (uint32_t i = 0; i < capacity; i++) {
		if (marks[i] != HASH_TABLE_ANY_MARK_FULL) { continue; }

		void const * ht_key = keys + i * hash_table->key_size;
		uint32_t const key_index = hash_table_any_find_key_index(hash_table, ht_key, hashes[i]);
		hash_table->hashes[key_index] = hashes[i];
		memcpy(
			hash_table->keys + key_index * hash_table->key_size,
			ht_key,
			hash_table->key_size
		);
		memcpy(
			hash_table->values + key_index * hash_table->value_size,
			values + i * hash_table->value_size,
			hash_table->value_size
		);
		hash_table->marks[key_index] = HASH_TABLE_ANY_MARK_FULL;
	}

	MEMORY_FREE(hashes);
	MEMORY_FREE(keys);
	MEMORY_FREE(values);
	MEMORY_FREE(marks);
}

void hash_table_any_clear(struct Hash_Table_Any * hash_table) {
	hash_table->count = 0;
	memset(hash_table->marks, HASH_TABLE_ANY_MARK_NONE, hash_table->capacity * sizeof(*hash_table->marks));
}

void * hash_table_any_get(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash) {
	if (hash_table->count == 0) { return NULL; }
	uint32_t const key_index = hash_table_any_find_key_index(hash_table, key, hash);
	if (hash_table->marks[key_index] == HASH_TABLE_ANY_MARK_FULL) {
		return hash_table->values + key_index * hash_table->value_size;
	}
	return NULL;
}

bool hash_table_any_set(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash, void const * value) {
	if (HASH_TABLE_SHOULD_GROW(hash_table->count + 1, hash_table->capacity)) {
		// hash_table_any_ensure_minimum_capacity(hash_table, GROW_CAPACITY(hash_table->capacity));
		hash_table_any_ensure_minimum_capacity(hash_table, hash_table->capacity + 1);
	}

	uint32_t const key_index = hash_table_any_find_key_index(hash_table, key, hash);
	bool const is_new = hash_table->marks[key_index] != HASH_TABLE_ANY_MARK_FULL;
	if (is_new) { hash_table->count++; }

	hash_table->hashes[key_index] = hash;
	memcpy(
		hash_table->keys + key_index * hash_table->key_size,
		key,
		hash_table->key_size
	);
	memcpy(
		hash_table->values + key_index * hash_table->value_size,
		value,
		hash_table->value_size
	);
	hash_table->marks[key_index] = HASH_TABLE_ANY_MARK_FULL;
	
	return is_new;
}

bool hash_table_any_del(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash) {
	if (hash_table->count == 0) { return false; }
	uint32_t const key_index = hash_table_any_find_key_index(hash_table, key, hash);
	if (hash_table->marks[key_index] != HASH_TABLE_ANY_MARK_FULL) { return false; }
	hash_table->marks[key_index] = HASH_TABLE_ANY_MARK_SKIP;
	return true;
}

//

static uint32_t hash_table_any_find_key_index(struct Hash_Table_Any * hash_table, void const * key, uint32_t hash) {
#if GROWTH_FACTOR == 2
	#define WRAP_VALUE(value, range) ((value) & ((range) - 1))
#else
	#define WRAP_VALUE(value, range) ((value) % (range))
#endif

	uint32_t const offset = WRAP_VALUE(hash, hash_table->capacity);
	for (uint32_t i = 0; i < hash_table->capacity; i++) {
		uint32_t const index = WRAP_VALUE(i + offset, hash_table->capacity);

		uint8_t const mark = hash_table->marks[index];
		if (mark == HASH_TABLE_ANY_MARK_SKIP) { continue; }
		if (mark == HASH_TABLE_ANY_MARK_NONE) { return index; }

		if (hash_table->hashes[index] != hash) { continue; }

		void const * ht_key = hash_table->keys + index * hash_table->key_size;
		if (memcmp(ht_key, key, hash_table->key_size) == 0) { return index; }
	}
	return INDEX_EMPTY;

#undef WRAP_VALUE
}

#undef GROWTH_FACTOR
#undef HASH_TABLE_SHOULD_GROW
// #undef GROW_CAPACITY
