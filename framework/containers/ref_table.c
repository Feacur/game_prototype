#include "framework/memory.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// @todo: use 2/3 as a growth factor here? no real need to be a power of 2
#define GROWTH_FACTOR 2
// #define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

#if GROWTH_FACTOR == 2
	// #include "framework/maths.h"
	uint32_t round_up_to_PO2_u32(uint32_t value);
#endif

//
#include "ref_table.h"

void ref_table_init(struct Ref_Table * ref_table, uint32_t value_size) {
	if (value_size == 0) {
		fprintf(stderr, "value size should be non-zero\n"); DEBUG_BREAK(); return;
	}

	*ref_table = (struct Ref_Table){
		.value_size = value_size,
	};
}

void ref_table_free(struct Ref_Table * ref_table) {
	MEMORY_FREE(ref_table, ref_table->values);
	MEMORY_FREE(ref_table, ref_table->dense);
	MEMORY_FREE(ref_table, ref_table->sparse);

	memset(ref_table, 0, sizeof(*ref_table));
}

void ref_table_clear(struct Ref_Table * ref_table) {
	ref_table->count = 0;
	ref_table->free_sparse_index = 0;
	for (uint32_t i = 0; i < ref_table->capacity; i++) {
		ref_table->dense[i] = INDEX_EMPTY;
		ref_table->sparse[i] = (struct Ref){
			.id = i + 1,
		};
	}
}

void ref_table_ensure_minimum_capacity(struct Ref_Table * ref_table, uint32_t minimum_capacity) {
	uint32_t const capacity = ref_table->capacity;

	if (minimum_capacity < 8) { minimum_capacity = 8; }
#if GROWTH_FACTOR == 2
	if (minimum_capacity > 0x80000000) {
		minimum_capacity = 0x80000000;
		fprintf(stderr, "requested capacity is too large\n"); DEBUG_BREAK();
	}
	minimum_capacity = round_up_to_PO2_u32(minimum_capacity);
#endif

	ref_table->capacity = minimum_capacity;
	ref_table->values = MEMORY_REALLOCATE_ARRAY(ref_table, ref_table->values, ref_table->value_size * ref_table->capacity);
	ref_table->dense = MEMORY_REALLOCATE_ARRAY(ref_table, ref_table->dense, ref_table->capacity);
	ref_table->sparse = MEMORY_REALLOCATE_ARRAY(ref_table, ref_table->sparse, ref_table->capacity);

	for (uint32_t i = capacity; i < ref_table->capacity; i++) {
		ref_table->dense[i] = INDEX_EMPTY;
		ref_table->sparse[i] = (struct Ref){
			.id = i + 1,
		};
	}
}

struct Ref ref_table_aquire(struct Ref_Table * ref_table, void const * value) {
	uint32_t const ref_id = ref_table->free_sparse_index;

	if (ref_id >= ref_table->capacity) {
		ref_table_ensure_minimum_capacity(ref_table, ref_table->capacity + 1);
	}

	// update the free list
	struct Ref * entry = ref_table->sparse + ref_id;
	ref_table->free_sparse_index = entry->id;

	// claim the entry
	entry->id = ref_table->count;
	ref_table->dense[ref_table->count] = ref_id;
	if (value != NULL) {
		memcpy(
			ref_table->values + ref_table->value_size * ref_table->count,
			value,
			ref_table->value_size
		);
	}

	//
	ref_table->count++;
	return (struct Ref){
		.id = ref_id,
		.gen = entry->gen,
	};
}

void ref_table_discard(struct Ref_Table * ref_table, struct Ref ref) {
	if (ref.id >= ref_table->capacity) { DEBUG_BREAK(); return; }

	struct Ref * entry = ref_table->sparse + ref.id;
	if (ref.id  != ref_table->dense[entry->id]) { DEBUG_BREAK(); return; }
	if (ref.gen != entry->gen)                  { DEBUG_BREAK(); return; }

	// invalidate the entry
	ref_table->count--;
	entry->gen++;

	// keep the `dense` and `values` arrays integrities
	if (entry->id < ref_table->count) {
		uint32_t const replacement_sparse_index = ref_table->dense[ref_table->count];

		struct Ref * replacement_entry = ref_table->sparse + replacement_sparse_index;
		if (replacement_entry->id != ref_table->count) { DEBUG_BREAK(); }

		replacement_entry->id = entry->id;
		ref_table->dense[entry->id] = replacement_sparse_index;
		ref_table->dense[ref_table->count] = INDEX_EMPTY;

		memcpy(
			ref_table->values + ref_table->value_size * entry->id,
			ref_table->values + ref_table->value_size * ref_table->count,
			ref_table->value_size
		);
	}

	// update the free list
	entry->id = ref_table->free_sparse_index;
	ref_table->free_sparse_index = ref.id;
}

void * ref_table_get(struct Ref_Table * ref_table, struct Ref ref) {
	if (ref.id >= ref_table->capacity) { return NULL; }

	struct Ref const * entry = ref_table->sparse + ref.id;
	if (ref.id  != ref_table->dense[entry->id]) { return NULL; }
	if (ref.gen != entry->gen)                  { return NULL; }

	return ref_table->values + ref_table->value_size * entry->id;
}

void ref_table_set(struct Ref_Table * ref_table, struct Ref ref, void const * value) {
	if (ref.id >= ref_table->capacity) { DEBUG_BREAK(); return; }

	struct Ref const * entry = ref_table->sparse + ref.id;
	if (ref.id  != ref_table->dense[entry->id]) { DEBUG_BREAK(); return; }
	if (ref.gen != entry->gen)                  { DEBUG_BREAK(); return; }

	memcpy(
		ref_table->values + ref_table->value_size * entry->id,
		value,
		ref_table->value_size
	);
}

bool ref_table_exists(struct Ref_Table * ref_table, struct Ref ref) {
	if (ref.id >= ref_table->capacity) { return false; }

	struct Ref const * entry = ref_table->sparse + ref.id;
	if (ref.id  != ref_table->dense[entry->id]) { return false; }
	if (ref.gen != entry->gen)                  { return false; }

	return true;
}

uint32_t ref_table_get_count(struct Ref_Table * ref_table) {
	return ref_table->count;
}

struct Ref ref_table_ref_at(struct Ref_Table * ref_table, uint32_t index) {
	if (index >= ref_table->count) { return (struct Ref){.id = INDEX_EMPTY}; }
	uint32_t const sparse_index = ref_table->dense[index];
	return (struct Ref){
		.id = sparse_index,
		.gen = ref_table->sparse[sparse_index].gen,
	};
}

void * ref_table_value_at(struct Ref_Table * ref_table, uint32_t index) {
	if (index >= ref_table->count) { return NULL; }
	return ref_table->values + ref_table->value_size * index;
}

#undef GROWTH_FACTOR
// #undef GROW_CAPACITY
