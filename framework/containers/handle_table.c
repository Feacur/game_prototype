#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

// @note: `id` of external `struct Handle` is expected to be `sparse index + 1`

//
#include "handle_table.h"

struct Handle_Table handle_table_init(uint32_t value_size) {
	return (struct Handle_Table){
		.buffer = array_any_init(value_size),
	};
}

void handle_table_free(struct Handle_Table * handle_table) {
	array_any_free(&handle_table->buffer);
	MEMORY_FREE(handle_table->dense);
	MEMORY_FREE(handle_table->sparse);
	common_memset(handle_table, 0, sizeof(*handle_table));
}

void handle_table_clear(struct Handle_Table * handle_table) {
	array_any_clear(&handle_table->buffer);
	handle_table->free_sparse_index = 0;
	common_memset(handle_table->dense, 0, sizeof(*handle_table->dense) * handle_table->buffer.capacity);
	for (uint32_t i = 0; i < handle_table->buffer.capacity; i++) {
		handle_table->sparse[i] = (struct Handle){.id = i + 1,};
	}
}

void handle_table_resize(struct Handle_Table * handle_table, uint32_t target_capacity) {
	if ((handle_table->buffer.count > 0) && (target_capacity < handle_table->buffer.capacity)) {
		logger_to_console("limiting target resize capacity\n"); DEBUG_BREAK();
		uint32_t largest_handle_id = 0;
		for (uint32_t i = 0; i < handle_table->buffer.count; i++) {
			uint32_t const id = handle_table->dense[i];
			if (largest_handle_id < id) {
				largest_handle_id = id;
			}
		}
		target_capacity = largest_handle_id + 1;
	}

	uint32_t const capacity = handle_table->buffer.capacity;

	array_any_resize(&handle_table->buffer, target_capacity);
	handle_table->dense  = MEMORY_REALLOCATE_ARRAY(handle_table->dense, handle_table->buffer.capacity);
	handle_table->sparse = MEMORY_REALLOCATE_ARRAY(handle_table->sparse, handle_table->buffer.capacity);

	common_memset(handle_table->dense + capacity, 0, sizeof(*handle_table->dense) * (handle_table->buffer.capacity - capacity));
	for (uint32_t i = capacity; i < handle_table->buffer.capacity; i++) {
		handle_table->sparse[i] = (struct Handle){.id = i + 1,};
	}
}

static void handle_table_ensure_capacity(struct Handle_Table * handle_table, uint32_t target_count) {
	if (handle_table->buffer.capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(handle_table->buffer.capacity, target_count - handle_table->buffer.capacity);
		handle_table_resize(handle_table, target_capacity);
	}
}

struct Handle handle_table_aquire(struct Handle_Table * handle_table, void const * value) {
	handle_table_ensure_capacity(handle_table, handle_table->buffer.count + 1);

	uint32_t const handle_id = handle_table->free_sparse_index;

	// update the free list
	struct Handle * entry = handle_table->sparse + handle_id;
	handle_table->free_sparse_index = entry->id;

	// claim the entry
	entry->id = handle_table->buffer.count;
	handle_table->dense[handle_table->buffer.count] = handle_id;
	array_any_push_many(&handle_table->buffer, 1, value);

	//
	return (struct Handle){
		.id = handle_id + 1,
		.gen = entry->gen,
	};
}

void handle_table_discard(struct Handle_Table * handle_table, struct Handle handle) {
	if (handle.id > handle_table->buffer.capacity) { DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle * entry = handle_table->sparse + handle_id;
	if (handle_id  != handle_table->dense[entry->id]) { DEBUG_BREAK(); return; }
	if (handle.gen != entry->gen)                  { DEBUG_BREAK(); return; }

	// invalidate the entry
	entry->gen++;

	// keep the `dense` and `values` arrays integrities
	if (entry->id < handle_table->buffer.count - 1) {
		array_any_set_many(&handle_table->buffer, entry->id, 1, array_any_pop(&handle_table->buffer));

		uint32_t const replacement_sparse_index = handle_table->dense[handle_table->buffer.count];

		struct Handle * replacement_entry = handle_table->sparse + replacement_sparse_index;
		if (replacement_entry->id != handle_table->buffer.count) { DEBUG_BREAK(); }

		replacement_entry->id = entry->id;
		handle_table->dense[entry->id] = replacement_sparse_index;
		handle_table->dense[handle_table->buffer.count] = 0;
	}
	else {
		handle_table->buffer.count--;
	}

	// update the free list
	entry->id = handle_table->free_sparse_index;
	handle_table->free_sparse_index = handle_id;
}

void * handle_table_get(struct Handle_Table * handle_table, struct Handle handle) {
	if (handle.id == 0) { return NULL; }
	if (handle.id > handle_table->buffer.capacity) { return NULL; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = handle_table->sparse + handle_id;
	if (handle_id  != handle_table->dense[entry->id]) { return NULL; }
	if (handle.gen != entry->gen)                  { return NULL; }

	return array_any_at(&handle_table->buffer, entry->id);
}

void handle_table_set(struct Handle_Table * handle_table, struct Handle handle, void const * value) {
	if (handle.id == 0) { return; }
	if (handle.id > handle_table->buffer.capacity) { DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = handle_table->sparse + handle_id;
	if (handle_id  != handle_table->dense[entry->id]) { DEBUG_BREAK(); return; }
	if (handle.gen != entry->gen)                  { DEBUG_BREAK(); return; }

	array_any_set_many(&handle_table->buffer, entry->id, 1, value);
}

bool handle_table_exists(struct Handle_Table * handle_table, struct Handle handle) {
	if (handle.id == 0) { return false; }
	if (handle.id > handle_table->buffer.capacity) { return false; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = handle_table->sparse + handle_id;
	if (handle_id  != handle_table->dense[entry->id]) { return false; }
	if (handle.gen != entry->gen)                  { return false; }

	return true;
}

uint32_t handle_table_get_count(struct Handle_Table * handle_table) {
	return handle_table->buffer.count;
}

struct Handle handle_table_handle_at(struct Handle_Table * handle_table, uint32_t index) {
	if (index >= handle_table->buffer.count) { return (struct Handle){0}; }
	uint32_t const sparse_index = handle_table->dense[index];
	return (struct Handle){
		.id = sparse_index + 1,
		.gen = handle_table->sparse[sparse_index].gen,
	};
}

void * handle_table_value_at(struct Handle_Table * handle_table, uint32_t index) {
	if (index >= handle_table->buffer.count) { return NULL; }
	uint32_t const sparse_index = handle_table->dense[index];
	return array_any_at(&handle_table->buffer, sparse_index);
}

bool handle_table_iterate(struct Handle_Table const * handle_table, struct Handle_Table_Iterator * iterator) {
	while (iterator->next < handle_table->buffer.count) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		uint32_t const handle_id = handle_table->dense[index];
		iterator->handle = (struct Handle){
			.id = handle_id + 1,
			.gen = handle_table->sparse[handle_id].gen,
		};
		iterator->value = array_any_at(&handle_table->buffer, index);
		return true;
	}
	return false;
}
