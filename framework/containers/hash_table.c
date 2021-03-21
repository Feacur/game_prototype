#include "framework/memory.h"

#include <string.h>

enum Hash_Table_Mark {
	HASH_TABLE_MARK_NONE,
	HASH_TABLE_MARK_SKIP,
	HASH_TABLE_MARK_FULL,
};

struct Hash_Table {
	uint32_t value_size;
	uint32_t capacity, count;
	uint32_t * keys;
	uint8_t * marks;
	uint8_t * values;
};

#define HASH_TABLE_SHOULD_GROW(count, capacity) ((count) * 3 > (capacity) * 2)

//
#include "hash_table.h"

struct Hash_Table * hash_table_init(uint32_t value_size) {
	struct Hash_Table * hash_table = MEMORY_ALLOCATE(struct Hash_Table);
	*hash_table = (struct Hash_Table){
		.value_size = value_size,
	};
	return hash_table;
}

void hash_table_free(struct Hash_Table * hash_table) {
	MEMORY_FREE(hash_table->keys);
	MEMORY_FREE(hash_table->marks);
	MEMORY_FREE(hash_table->values);

	memset(hash_table, 0, sizeof(*hash_table));
	MEMORY_FREE(hash_table);
}

void hash_table_clear(struct Hash_Table * hash_table) {
	memset(hash_table->marks, HASH_TABLE_MARK_NONE, hash_table->capacity * sizeof(*hash_table->marks));
}

static void hash_table_grow(struct Hash_Table * hash_table);
static uint32_t hash_table_find_key_index(struct Hash_Table * hash_table, uint32_t key);

void * hash_table_get(struct Hash_Table * hash_table, uint32_t key) {
	uint32_t key_index = hash_table_find_key_index(hash_table, key);
	if (hash_table->marks[key_index] == HASH_TABLE_MARK_FULL) {
		return hash_table->values + key_index * hash_table->value_size;
	}
	return NULL;
}

bool hash_table_set(struct Hash_Table * hash_table, uint32_t key, void const * value) {
	if (HASH_TABLE_SHOULD_GROW(hash_table->count + 1, hash_table->capacity)) {
		hash_table_grow(hash_table);
	}

	uint32_t key_index = hash_table_find_key_index(hash_table, key);
	bool const is_new = hash_table->marks[key_index] != HASH_TABLE_MARK_FULL;
	if (is_new) { hash_table->count++; }

	hash_table->keys[key_index] = key;
	hash_table->marks[key_index] = HASH_TABLE_MARK_FULL;
	memcpy(
		hash_table->values + key_index * hash_table->value_size,
		value,
		hash_table->value_size * sizeof(*hash_table->values)
	);
	
	return is_new;
}

static void hash_table_grow(struct Hash_Table * hash_table) {
	uint32_t const capacity = hash_table->capacity;
	uint32_t * keys = hash_table->keys;
	uint8_t * marks = hash_table->marks;
	uint8_t * values = hash_table->values;

	hash_table->capacity = GROW_CAPACITY(hash_table->capacity);
	hash_table->keys = MEMORY_ALLOCATE_ARRAY(uint32_t, hash_table->capacity);
	hash_table->marks = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->capacity);
	hash_table->values = MEMORY_ALLOCATE_ARRAY(uint8_t, hash_table->capacity * hash_table->value_size);

	memset(hash_table->marks, HASH_TABLE_MARK_NONE, hash_table->capacity * sizeof(*hash_table->marks));

	for (uint32_t i = 0; i < capacity; i++) {
		if (marks[i] != HASH_TABLE_MARK_FULL) { continue; }

		uint32_t key_index = hash_table_find_key_index(hash_table, keys[i]);
		hash_table->keys[key_index] = keys[i];
		hash_table->marks[key_index] = HASH_TABLE_MARK_FULL;
		memcpy(
			hash_table->values + key_index * hash_table->value_size,
			values + i * hash_table->value_size,
			hash_table->value_size * sizeof(*hash_table->values)
		);
	}

	MEMORY_FREE(keys);
	MEMORY_FREE(marks);
	MEMORY_FREE(values);
}

static uint32_t hash_table_find_key_index(struct Hash_Table * hash_table, uint32_t key) {
#if GROWTH_FACTOR == 2
	#define WRAP_VALUE(value, range) ((value) & ((range) - 1))
#else
	#define WRAP_VALUE(value, range) ((value) % (range))
#endif

	uint32_t const offset = WRAP_VALUE(key, hash_table->capacity);
	for (uint32_t i = 0; i < hash_table->capacity; i++) {
		uint32_t const index = WRAP_VALUE(i + offset, hash_table->capacity);
		uint8_t const mark = hash_table->marks[index];
		if (mark == HASH_TABLE_MARK_SKIP) { continue; }
		if (mark == HASH_TABLE_MARK_NONE) { return index; }
		if (hash_table->keys[index] == key) { return index; }
	}
	return INDEX_EMPTY;

#undef WRAP_VALUE
}
