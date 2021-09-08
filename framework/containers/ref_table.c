#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "ref_table.h"

void ref_table_init(struct Ref_Table * ref_table, uint32_t value_size) {
	*ref_table = (struct Ref_Table){
		.value_size = value_size,
	};
}

void ref_table_free(struct Ref_Table * ref_table) {
	MEMORY_FREE(ref_table, ref_table->values);
	MEMORY_FREE(ref_table, ref_table->dense);
	MEMORY_FREE(ref_table, ref_table->sparse);

	common_memset(ref_table, 0, sizeof(*ref_table));
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

void ref_table_resize(struct Ref_Table * ref_table, uint32_t target_capacity) {
	if ((ref_table->count > 0) && (target_capacity < ref_table->capacity)) {
		logger_to_console("limiting target resize capacity\n"); DEBUG_BREAK();
		uint32_t largest_ref_id = 0;
		for (uint32_t i = 0; i < ref_table->count; i++) {
			uint32_t const id = ref_table->dense[i];
			if (largest_ref_id < id) {
				largest_ref_id = id;
			}
		}
		target_capacity = largest_ref_id + 1;
	}

	uint32_t const capacity = ref_table->capacity;

	ref_table->capacity = target_capacity;
	ref_table->values   = MEMORY_REALLOCATE_ARRAY(ref_table, ref_table->values, ref_table->value_size * ref_table->capacity);
	ref_table->dense    = MEMORY_REALLOCATE_ARRAY(ref_table, ref_table->dense, ref_table->capacity);
	ref_table->sparse   = MEMORY_REALLOCATE_ARRAY(ref_table, ref_table->sparse, ref_table->capacity);

	for (uint32_t i = capacity; i < ref_table->capacity; i++) {
		ref_table->dense[i] = INDEX_EMPTY;
		ref_table->sparse[i] = (struct Ref){
			.id = i + 1,
		};
	}
}

static void ref_table_ensure_capacity(struct Ref_Table * ref_table, uint32_t target_count) {
	if (ref_table->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(ref_table->capacity, target_count - ref_table->capacity);
		ref_table_resize(ref_table, target_capacity);
	}
}

struct Ref ref_table_aquire(struct Ref_Table * ref_table, void const * value) {
	ref_table_ensure_capacity(ref_table, ref_table->count + 1);

	uint32_t const ref_id = ref_table->free_sparse_index;

	// update the free list
	struct Ref * entry = ref_table->sparse + ref_id;
	ref_table->free_sparse_index = entry->id;

	// claim the entry
	entry->id = ref_table->count;
	ref_table->dense[ref_table->count] = ref_id;
	if (value != NULL) {
		common_memcpy(
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

		common_memcpy(
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

	common_memcpy(
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
	if (index >= ref_table->count) { return ref_empty; }
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

bool ref_table_iterate(struct Ref_Table * ref_table, struct Ref_Table_Iterator * iterator) {
	while (iterator->next < ref_table->count) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		uint32_t const ref_id = ref_table->dense[index];
		iterator->ref = (struct Ref){
			.id = ref_id,
			.gen = ref_table->sparse[ref_id].gen,
		};
		iterator->value = ref_table->values + ref_table->value_size * index;
		return true;
	}
	return false;
}

//

struct Ref const ref_empty = {.id = INDEX_EMPTY,};
