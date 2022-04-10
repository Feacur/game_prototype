#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

// @note: `id` of external `struct Ref` is expected to be `sparse index + 1`

//
#include "ref_table.h"

struct Ref_Table ref_table_init(uint32_t value_size) {
	return (struct Ref_Table){
		.buffer = array_any_init(value_size),
	};
}

void ref_table_free(struct Ref_Table * ref_table) {
	array_any_free(&ref_table->buffer);
	MEMORY_FREE(ref_table->dense);
	MEMORY_FREE(ref_table->sparse);
	common_memset(ref_table, 0, sizeof(*ref_table));
}

void ref_table_clear(struct Ref_Table * ref_table) {
	array_any_clear(&ref_table->buffer);
	ref_table->free_sparse_index = 0;
	common_memset(ref_table->dense, 0, sizeof(*ref_table->dense) * ref_table->buffer.capacity);
	for (uint32_t i = 0; i < ref_table->buffer.capacity; i++) {
		ref_table->sparse[i] = (struct Ref){.id = i + 1,};
	}
}

void ref_table_resize(struct Ref_Table * ref_table, uint32_t target_capacity) {
	if ((ref_table->buffer.count > 0) && (target_capacity < ref_table->buffer.capacity)) {
		logger_to_console("limiting target resize capacity\n"); DEBUG_BREAK();
		uint32_t largest_ref_id = 0;
		for (uint32_t i = 0; i < ref_table->buffer.count; i++) {
			uint32_t const id = ref_table->dense[i];
			if (largest_ref_id < id) {
				largest_ref_id = id;
			}
		}
		target_capacity = largest_ref_id + 1;
	}

	uint32_t const capacity = ref_table->buffer.capacity;

	array_any_resize(&ref_table->buffer, target_capacity);
	ref_table->dense  = MEMORY_REALLOCATE_ARRAY(ref_table->dense, ref_table->buffer.capacity);
	ref_table->sparse = MEMORY_REALLOCATE_ARRAY(ref_table->sparse, ref_table->buffer.capacity);

	common_memset(ref_table->dense + capacity, 0, sizeof(*ref_table->dense) * (ref_table->buffer.capacity - capacity));
	for (uint32_t i = capacity; i < ref_table->buffer.capacity; i++) {
		ref_table->sparse[i] = (struct Ref){.id = i + 1,};
	}
}

static void ref_table_ensure_capacity(struct Ref_Table * ref_table, uint32_t target_count) {
	if (ref_table->buffer.capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(ref_table->buffer.capacity, target_count - ref_table->buffer.capacity);
		ref_table_resize(ref_table, target_capacity);
	}
}

struct Ref ref_table_aquire(struct Ref_Table * ref_table, void const * value) {
	ref_table_ensure_capacity(ref_table, ref_table->buffer.count + 1);

	uint32_t const ref_id = ref_table->free_sparse_index;

	// update the free list
	struct Ref * entry = ref_table->sparse + ref_id;
	ref_table->free_sparse_index = entry->id;

	// claim the entry
	entry->id = ref_table->buffer.count;
	ref_table->dense[ref_table->buffer.count] = ref_id;
	array_any_push(&ref_table->buffer, value);

	//
	return (struct Ref){
		.id = ref_id + 1,
		.gen = entry->gen,
	};
}

void ref_table_discard(struct Ref_Table * ref_table, struct Ref ref) {
	if (ref.id > ref_table->buffer.capacity) { DEBUG_BREAK(); return; }

	uint32_t const ref_id = ref.id - 1;
	struct Ref * entry = ref_table->sparse + ref_id;
	if (ref_id  != ref_table->dense[entry->id]) { DEBUG_BREAK(); return; }
	if (ref.gen != entry->gen)                  { DEBUG_BREAK(); return; }

	// invalidate the entry
	entry->gen++;

	// keep the `dense` and `values` arrays integrities
	if (entry->id < ref_table->buffer.count - 1) {
		array_any_set_many(&ref_table->buffer, entry->id, 1, array_any_pop(&ref_table->buffer));

		uint32_t const replacement_sparse_index = ref_table->dense[ref_table->buffer.count];

		struct Ref * replacement_entry = ref_table->sparse + replacement_sparse_index;
		if (replacement_entry->id != ref_table->buffer.count) { DEBUG_BREAK(); }

		replacement_entry->id = entry->id;
		ref_table->dense[entry->id] = replacement_sparse_index;
		ref_table->dense[ref_table->buffer.count] = 0;
	}
	else {
		ref_table->buffer.count--;
	}

	// update the free list
	entry->id = ref_table->free_sparse_index;
	ref_table->free_sparse_index = ref_id;
}

void * ref_table_get(struct Ref_Table * ref_table, struct Ref ref) {
	if (ref.id == 0) { return NULL; }
	if (ref.id > ref_table->buffer.capacity) { return NULL; }

	uint32_t const ref_id = ref.id - 1;
	struct Ref const * entry = ref_table->sparse + ref_id;
	if (ref_id  != ref_table->dense[entry->id]) { return NULL; }
	if (ref.gen != entry->gen)                  { return NULL; }

	return array_any_at(&ref_table->buffer, entry->id);
}

void ref_table_set(struct Ref_Table * ref_table, struct Ref ref, void const * value) {
	if (ref.id == 0) { return; }
	if (ref.id > ref_table->buffer.capacity) { DEBUG_BREAK(); return; }

	uint32_t const ref_id = ref.id - 1;
	struct Ref const * entry = ref_table->sparse + ref_id;
	if (ref_id  != ref_table->dense[entry->id]) { DEBUG_BREAK(); return; }
	if (ref.gen != entry->gen)                  { DEBUG_BREAK(); return; }

	array_any_set_many(&ref_table->buffer, entry->id, 1, value);
}

bool ref_table_exists(struct Ref_Table * ref_table, struct Ref ref) {
	if (ref.id == 0) { return false; }
	if (ref.id > ref_table->buffer.capacity) { return false; }

	uint32_t const ref_id = ref.id - 1;
	struct Ref const * entry = ref_table->sparse + ref_id;
	if (ref_id  != ref_table->dense[entry->id]) { return false; }
	if (ref.gen != entry->gen)                  { return false; }

	return true;
}

uint32_t ref_table_get_count(struct Ref_Table * ref_table) {
	return ref_table->buffer.count;
}

struct Ref ref_table_ref_at(struct Ref_Table * ref_table, uint32_t index) {
	if (index >= ref_table->buffer.count) { return c_ref_empty; }
	uint32_t const sparse_index = ref_table->dense[index];
	return (struct Ref){
		.id = sparse_index + 1,
		.gen = ref_table->sparse[sparse_index].gen,
	};
}

void * ref_table_value_at(struct Ref_Table * ref_table, uint32_t index) {
	if (index >= ref_table->buffer.count) { return NULL; }
	uint32_t const sparse_index = ref_table->dense[index];
	return array_any_at(&ref_table->buffer, sparse_index);
}

bool ref_table_iterate(struct Ref_Table const * ref_table, struct Ref_Table_Iterator * iterator) {
	while (iterator->next < ref_table->buffer.count) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		uint32_t const ref_id = ref_table->dense[index];
		iterator->ref = (struct Ref){
			.id = ref_id + 1,
			.gen = ref_table->sparse[ref_id].gen,
		};
		iterator->value = array_any_at(&ref_table->buffer, index);
		return true;
	}
	return false;
}
