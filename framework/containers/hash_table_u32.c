#include "framework/memory.h"

#include <stdio.h>
#include <string.h>

// @todo: make this dynamically compilable?
// @todo: make it easier to use another growth factor
#define GROWTH_FACTOR 2
#define HASH_TABLE_SHOULD_GROW(count, capacity) ((count) > (capacity) * 2 / 3)
// #define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

enum Hash_Table_U32_Mark {
	HASH_TABLE_U32_MARK_NONE,
	HASH_TABLE_U32_MARK_SKIP,
	HASH_TABLE_U32_MARK_FULL,
};

struct Hash_Table_U32 {
	uint32_t value_size;
	uint32_t capacity, count;
	uint32_t * key_hashes;
	uint8_t * values;
	uint8_t * marks;
};

#if GROWTH_FACTOR == 2
	// #include "framework/maths.h"
	uint32_t round_up_to_PO2_u32(uint32_t value);
#endif

//
#include "hash_table_u32.h"

struct Hash_Table_U32 * hash_table_u32_init(uint32_t value_size) {
	if (value_size == 0) {
		fprintf(stderr, "value size should be non-zero\n"); DEBUG_BREAK(); return NULL;
	}

	struct Hash_Table_U32 * hash_table = MEMORY_ALLOCATE(NULL, struct Hash_Table_U32);
	*hash_table = (struct Hash_Table_U32){
		.value_size = value_size,
	};
	return hash_table;
}

void hash_table_u32_free(struct Hash_Table_U32 * hash_table) {
	MEMORY_FREE(hash_table, hash_table->key_hashes);
	MEMORY_FREE(hash_table, hash_table->values);
	MEMORY_FREE(hash_table, hash_table->marks);

	memset(hash_table, 0, sizeof(*hash_table));
	MEMORY_FREE(hash_table, hash_table);
}

static uint32_t hash_table_u32_find_key_index(struct Hash_Table_U32 * hash_table, uint32_t key_hash);
void hash_table_u32_ensure_minimum_capacity(struct Hash_Table_U32 * hash_table, uint32_t minimum_capacity) {
	if (minimum_capacity < 8) { minimum_capacity = 8; }
#if GROWTH_FACTOR == 2
	if (minimum_capacity > 0x80000000) {
		minimum_capacity = 0x80000000;
		fprintf(stderr, "requested capacity is too large\n"); DEBUG_BREAK();
	}
	minimum_capacity = round_up_to_PO2_u32(minimum_capacity);
#endif

	uint32_t const capacity = hash_table->capacity;
	uint32_t * key_hashes = hash_table->key_hashes;
	uint8_t  * values     = hash_table->values;
	uint8_t  * marks      = hash_table->marks;

	hash_table->capacity   = minimum_capacity;
	hash_table->key_hashes = MEMORY_ALLOCATE_ARRAY(hash_table, uint32_t, hash_table->capacity);
	hash_table->values     = MEMORY_ALLOCATE_ARRAY(hash_table, uint8_t, hash_table->value_size * hash_table->capacity);
	hash_table->marks      = MEMORY_ALLOCATE_ARRAY(hash_table, uint8_t, hash_table->capacity);

	memset(hash_table->marks, HASH_TABLE_U32_MARK_NONE, sizeof(*hash_table->marks) * hash_table->capacity);

	// @note: .count remains as is
	for (uint32_t i = 0; i < capacity; i++) {
		if (marks[i] != HASH_TABLE_U32_MARK_FULL) { continue; }

		uint32_t const key_index = hash_table_u32_find_key_index(hash_table, key_hashes[i]);
		hash_table->key_hashes[key_index] = key_hashes[i];
		memcpy(
			hash_table->values + hash_table->value_size * key_index,
			values + hash_table->value_size * i,
			hash_table->value_size
		);
		hash_table->marks[key_index] = HASH_TABLE_U32_MARK_FULL;
	}

	MEMORY_FREE(hash_table, key_hashes);
	MEMORY_FREE(hash_table, values);
	MEMORY_FREE(hash_table, marks);
}

void hash_table_u32_clear(struct Hash_Table_U32 * hash_table) {
	hash_table->count = 0;
	memset(hash_table->marks, HASH_TABLE_U32_MARK_NONE, sizeof(*hash_table->marks) * hash_table->capacity);
}

void * hash_table_u32_get(struct Hash_Table_U32 * hash_table, uint32_t key_hash) {
	if (hash_table->count == 0) { return NULL; }
	uint32_t const key_index = hash_table_u32_find_key_index(hash_table, key_hash);
	// if (key_index == INDEX_EMPTY) { return NULL; }
	if (hash_table->marks[key_index] != HASH_TABLE_U32_MARK_FULL) { return NULL; }
	return hash_table->values + hash_table->value_size * key_index;
}

bool hash_table_u32_set(struct Hash_Table_U32 * hash_table, uint32_t key_hash, void const * value) {
	if (HASH_TABLE_SHOULD_GROW(hash_table->count + 1, hash_table->capacity)) {
		// hash_table_u32_ensure_minimum_capacity(hash_table, GROW_CAPACITY(hash_table->capacity));
		hash_table_u32_ensure_minimum_capacity(hash_table, hash_table->capacity + 1);
	}

	uint32_t const key_index = hash_table_u32_find_key_index(hash_table, key_hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	enum Hash_Table_U32_Mark const mark = hash_table->marks[key_index];
	bool const is_new = (mark == HASH_TABLE_U32_MARK_NONE);
	if (is_new) { hash_table->count++; }

	hash_table->key_hashes[key_index] = key_hash;
	memcpy(
		hash_table->values + hash_table->value_size * key_index,
		value,
		hash_table->value_size
	);
	hash_table->marks[key_index] = HASH_TABLE_U32_MARK_FULL;
	
	return is_new;
}

bool hash_table_u32_del(struct Hash_Table_U32 * hash_table, uint32_t key_hash) {
	if (hash_table->count == 0) { return false; }
	uint32_t const key_index = hash_table_u32_find_key_index(hash_table, key_hash);
	// if (key_index == INDEX_EMPTY) { return false; }
	if (hash_table->marks[key_index] != HASH_TABLE_U32_MARK_FULL) { return false; }
	hash_table->marks[key_index] = HASH_TABLE_U32_MARK_SKIP;
	return true;
}

uint32_t hash_table_u32_get_iteration_capacity(struct Hash_Table_U32 * hash_table) {
	return hash_table->capacity;
}

void * hash_table_u32_iterate(struct Hash_Table_U32 * hash_table, uint32_t index) {
	if (hash_table->marks[index] != HASH_TABLE_U32_MARK_FULL) { return NULL; }
	return hash_table->values + hash_table->value_size * index;
}

//

static uint32_t hash_table_u32_find_key_index(struct Hash_Table_U32 * hash_table, uint32_t key_hash) {
#if GROWTH_FACTOR == 2
	#define WRAP_VALUE(value, range) ((value) & ((range) - 1))
#else
	#define WRAP_VALUE(value, range) ((value) % (range))
#endif

	uint32_t empty = INDEX_EMPTY;

	uint32_t const offset = WRAP_VALUE(key_hash, hash_table->capacity);
	for (uint32_t i = 0; i < hash_table->capacity; i++) {
		uint32_t const index = WRAP_VALUE(i + offset, hash_table->capacity);

		uint8_t const mark = hash_table->marks[index];
		if (mark == HASH_TABLE_U32_MARK_SKIP) {
			if (empty == INDEX_EMPTY) { empty = index; }
			continue;
		}
		if (mark == HASH_TABLE_U32_MARK_NONE) {
			if (empty == INDEX_EMPTY) { empty = index; }
			break;
		}

		if (hash_table->key_hashes[index] == key_hash) { return index; }
	}

	return empty;

#undef WRAP_VALUE
}

#undef GROWTH_FACTOR
#undef HASH_TABLE_SHOULD_GROW
// #undef GROW_CAPACITY
