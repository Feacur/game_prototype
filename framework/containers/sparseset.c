#include "framework/memory.h"
#include "framework/logger.h"
#include "internal/helpers.h"

// @note: `id` of external `struct Handle` is expected to be `sparse index + 1`

//
#include "sparseset.h"

struct Sparseset sparseset_init(uint32_t value_size) {
	return (struct Sparseset){
		.packed = array_init(value_size),
		.sparse = array_init(sizeof(struct Handle)),
	};
}

void sparseset_free(struct Sparseset * sparseset) {
	array_free(&sparseset->packed);
	array_free(&sparseset->sparse);
	MEMORY_FREE(sparseset->ids);
	common_memset(sparseset, 0, sizeof(*sparseset));
}

void sparseset_clear(struct Sparseset * sparseset) {
	array_clear(&sparseset->packed);
	array_clear(&sparseset->sparse);
	common_memset(sparseset->ids, 0, sizeof(*sparseset->ids) * sparseset->packed.capacity);
	sparseset->free_list = 0;
}

void sparseset_resize(struct Sparseset * sparseset, uint32_t target_capacity) {
	if ((sparseset->packed.count > 0) && (target_capacity < sparseset->packed.count)) {
		logger_to_console("limiting target resize capacity\n"); DEBUG_BREAK();
		uint32_t largest_handle_id = 0;
		for (uint32_t i = 0; i < sparseset->packed.count; i++) {
			uint32_t const id = sparseset->ids[i];
			if (largest_handle_id < id) {
				largest_handle_id = id;
			}
		}
		target_capacity = largest_handle_id + 1;
	}

	array_resize(&sparseset->packed, target_capacity);
	array_resize(&sparseset->sparse, target_capacity);
	sparseset->ids = MEMORY_REALLOCATE_ARRAY(sparseset->ids, sparseset->packed.capacity);
}

static void sparseset_ensure_capacity(struct Sparseset * sparseset, uint32_t target_count) {
	if (sparseset->packed.capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(sparseset->packed.capacity, target_count - sparseset->packed.capacity);
		sparseset_resize(sparseset, target_capacity);
	}
}

struct Handle sparseset_aquire(struct Sparseset * sparseset, void const * value) {
	sparseset_ensure_capacity(sparseset, sparseset->packed.count + 1);

	uint32_t const handle_id = sparseset->free_list;

	// update the free list
	if (handle_id >= sparseset->sparse.count) {
		array_push_many(&sparseset->sparse, 1, &(struct Handle){
			.id = handle_id + 1,
		});
	}
	struct Handle * entry = array_at(&sparseset->sparse, handle_id);
	sparseset->free_list = entry->id;

	// claim the entry
	sparseset->ids[sparseset->packed.count] = handle_id;
	entry->id = sparseset->packed.count;
	array_push_many(&sparseset->packed, 1, value);

	//
	return (struct Handle){
		.id = handle_id + 1,
		.gen = entry->gen,
	};
}

void sparseset_discard(struct Sparseset * sparseset, struct Handle handle) {
	if (handle.id == 0)                      { DEBUG_BREAK(); return; }
	if (handle.id > sparseset->sparse.count) { DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle * entry = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry->gen)                { DEBUG_BREAK(); return; }
	if (handle_id  != sparseset->ids[entry->id]) { DEBUG_BREAK(); return; }

	// swap the removed entry with the last one, maintain packing
	void const * value = array_pop(&sparseset->packed);
	if (entry->id < sparseset->packed.count) {
		uint32_t const swap_id = sparseset->ids[sparseset->packed.count];
		struct Handle * swap = array_at(&sparseset->sparse, swap_id);
		if (swap->id != sparseset->packed.count) { DEBUG_BREAK(); }

		sparseset->ids[entry->id] = swap_id;
		swap->id = entry->id;
		array_set_many(&sparseset->packed, entry->id, 1, value);
	}

	// update the free list
	*entry = (struct Handle){
		.id = sparseset->free_list,
		.gen = (entry->gen + 1) & 0xff,
	};
	sparseset->free_list = handle_id;
}

void * sparseset_get(struct Sparseset * sparseset, struct Handle handle) {
	if (handle.id == 0)                      { return NULL; }
	if (handle.id > sparseset->sparse.count) { return NULL; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry->gen)                { return NULL; }
	if (handle_id  != sparseset->ids[entry->id]) { return NULL; }

	return array_at(&sparseset->packed, entry->id);
}

void sparseset_set(struct Sparseset * sparseset, struct Handle handle, void const * value) {
	if (handle.id == 0)                      { DEBUG_BREAK(); return; }
	if (handle.id > sparseset->sparse.count) { DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry->gen)                { DEBUG_BREAK(); return; }
	if (handle_id  != sparseset->ids[entry->id]) { DEBUG_BREAK(); return; }

	array_set_many(&sparseset->packed, entry->id, 1, value);
}

bool sparseset_iterate(struct Sparseset const * sparseset, struct Sparseset_Iterator * iterator) {
	while (iterator->next < sparseset->packed.count) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		uint32_t const handle_id = sparseset->ids[index];
		struct Handle const * entry = array_at(&sparseset->sparse, handle_id);
		iterator->handle = (struct Handle){
			.id = handle_id + 1,
			.gen = entry->gen,
		};
		iterator->value = array_at(&sparseset->packed, index);
		return true;
	}
	return false;
}
