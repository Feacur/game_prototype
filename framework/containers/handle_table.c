#include "framework/memory.h"
#include "framework/logger.h"
#include "internal/helpers.h"

// @note: `id` of external `struct Handle` is expected to be `sparse index + 1`

//
#include "handle_table.h"

struct Handle_Table handle_table_init(uint32_t value_size) {
	return (struct Handle_Table){
		.packed = array_init(value_size),
		.sparse = array_init(sizeof(struct Handle)),
	};
}

void handle_table_free(struct Handle_Table * handle_table) {
	array_free(&handle_table->packed);
	array_free(&handle_table->sparse);
	MEMORY_FREE(handle_table->ids);
	common_memset(handle_table, 0, sizeof(*handle_table));
}

void handle_table_clear(struct Handle_Table * handle_table) {
	array_clear(&handle_table->packed);
	array_clear(&handle_table->sparse);
	common_memset(handle_table->ids, 0, sizeof(*handle_table->ids) * handle_table->packed.capacity);
	handle_table->free_list = 0;
}

void handle_table_resize(struct Handle_Table * handle_table, uint32_t target_capacity) {
	if ((handle_table->packed.count > 0) && (target_capacity < handle_table->packed.count)) {
		logger_to_console("limiting target resize capacity\n"); DEBUG_BREAK();
		uint32_t largest_handle_id = 0;
		for (uint32_t i = 0; i < handle_table->packed.count; i++) {
			uint32_t const id = handle_table->ids[i];
			if (largest_handle_id < id) {
				largest_handle_id = id;
			}
		}
		target_capacity = largest_handle_id + 1;
	}

	array_resize(&handle_table->packed, target_capacity);
	array_resize(&handle_table->sparse, target_capacity);
	handle_table->ids = MEMORY_REALLOCATE_ARRAY(handle_table->ids, handle_table->packed.capacity);
}

static void handle_table_ensure_capacity(struct Handle_Table * handle_table, uint32_t target_count) {
	if (handle_table->packed.capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(handle_table->packed.capacity, target_count - handle_table->packed.capacity);
		handle_table_resize(handle_table, target_capacity);
	}
}

struct Handle handle_table_aquire(struct Handle_Table * handle_table, void const * value) {
	handle_table_ensure_capacity(handle_table, handle_table->packed.count + 1);

	uint32_t const handle_id = handle_table->free_list;

	// update the free list
	if (handle_id >= handle_table->sparse.count) {
		array_push_many(&handle_table->sparse, 1, &(struct Handle){
			.id = handle_id + 1,
		});
	}
	struct Handle * entry = array_at(&handle_table->sparse, handle_id);
	handle_table->free_list = entry->id;

	// claim the entry
	handle_table->ids[handle_table->packed.count] = handle_id;
	entry->id = handle_table->packed.count;
	array_push_many(&handle_table->packed, 1, value);

	//
	return (struct Handle){
		.id = handle_id + 1,
		.gen = entry->gen,
	};
}

void handle_table_discard(struct Handle_Table * handle_table, struct Handle handle) {
	if (handle.id == 0)                         { DEBUG_BREAK(); return; }
	if (handle.id > handle_table->sparse.count) { DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle * entry = array_at(&handle_table->sparse, handle_id);
	if (handle.gen != entry->gen)                   { DEBUG_BREAK(); return; }
	if (handle_id  != handle_table->ids[entry->id]) { DEBUG_BREAK(); return; }

	// swap the removed entry with the last one, maintain packing
	void const * value = array_pop(&handle_table->packed);
	if (entry->id < handle_table->packed.count) {
		uint32_t const swap_id = handle_table->ids[handle_table->packed.count];
		struct Handle * swap = array_at(&handle_table->sparse, swap_id);
		if (swap->id != handle_table->packed.count) { DEBUG_BREAK(); }

		handle_table->ids[entry->id] = swap_id;
		swap->id = entry->id;
		array_set_many(&handle_table->packed, entry->id, 1, value);
	}

	// update the free list
	*entry = (struct Handle){
		.id = handle_table->free_list,
		.gen = (entry->gen + 1) & 0xff,
	};
	handle_table->free_list = handle_id;
}

void * handle_table_get(struct Handle_Table * handle_table, struct Handle handle) {
	if (handle.id == 0)                         { return NULL; }
	if (handle.id > handle_table->sparse.count) { return NULL; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = array_at(&handle_table->sparse, handle_id);
	if (handle.gen != entry->gen)                   { return NULL; }
	if (handle_id  != handle_table->ids[entry->id]) { return NULL; }

	return array_at(&handle_table->packed, entry->id);
}

void handle_table_set(struct Handle_Table * handle_table, struct Handle handle, void const * value) {
	if (handle.id == 0)                         { DEBUG_BREAK(); return; }
	if (handle.id > handle_table->sparse.count) { DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = array_at(&handle_table->sparse, handle_id);
	if (handle.gen != entry->gen)                   { DEBUG_BREAK(); return; }
	if (handle_id  != handle_table->ids[entry->id]) { DEBUG_BREAK(); return; }

	array_set_many(&handle_table->packed, entry->id, 1, value);
}

bool handle_table_iterate(struct Handle_Table const * handle_table, struct Handle_Table_Iterator * iterator) {
	while (iterator->next < handle_table->packed.count) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		uint32_t const handle_id = handle_table->ids[index];
		struct Handle const * entry = array_at(&handle_table->sparse, handle_id);
		iterator->handle = (struct Handle){
			.id = handle_id + 1,
			.gen = entry->gen,
		};
		iterator->value = array_at(&handle_table->packed, index);
		return true;
	}
	return false;
}
