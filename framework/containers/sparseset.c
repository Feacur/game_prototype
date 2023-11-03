#include "framework/memory.h"
#include "framework/formatter.h"
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

void sparseset_ensure(struct Sparseset * sparseset, uint32_t capacity) {
	if (capacity <= sparseset->packed.capacity) { return; }
	capacity = growth_adjust_array(sparseset->packed.capacity, capacity);

	array_resize(&sparseset->packed, capacity);
	array_resize(&sparseset->sparse, capacity);
	sparseset->ids = MEMORY_REALLOCATE_ARRAY(sparseset->ids, capacity);
}

struct Handle sparseset_aquire(struct Sparseset * sparseset, void const * value) {
	sparseset_ensure(sparseset, sparseset->packed.count + 1);

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
	if (handle.id == 0)                      { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (handle.id > sparseset->sparse.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle * entry = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry->gen)                { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (handle_id  != sparseset->ids[entry->id]) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	// swap the removed entry with the last one, maintain packing
	void const * value = array_pop(&sparseset->packed, 1);
	if (entry->id < sparseset->packed.count) {
		uint32_t const swap_id = sparseset->ids[sparseset->packed.count];
		struct Handle * swap = array_at(&sparseset->sparse, swap_id);
		if (swap->id != sparseset->packed.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); }

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

void * sparseset_get(struct Sparseset const * sparseset, struct Handle handle) {
	if (handle.id == 0)                      { return NULL; }
	if (handle.id > sparseset->sparse.count) { return NULL; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry->gen)                { return NULL; }
	if (handle_id  != sparseset->ids[entry->id]) { return NULL; }

	return array_at(&sparseset->packed, entry->id);
}

void sparseset_set(struct Sparseset * sparseset, struct Handle handle, void const * value) {
	if (handle.id == 0)                      { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (handle.id > sparseset->sparse.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry->gen)                { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (handle_id  != sparseset->ids[entry->id]) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	array_set_many(&sparseset->packed, entry->id, 1, value);
}

uint32_t sparseset_get_count(struct Sparseset const * sparseset) {
	return sparseset->packed.count;
}

void * sparseset_get_at(struct Sparseset const * sparseset, uint32_t index) {
	return array_at(&sparseset->packed, index);
}

static struct Handle sparseset_get_handle_at(struct Sparseset const * sparseset, uint32_t index) {
	uint32_t const handle_id = sparseset->ids[index];
	struct Handle const * entry = array_at_unsafe(&sparseset->sparse, handle_id);
	return (struct Handle){
		.id = handle_id + 1,
		.gen = entry->gen,
	};
}

bool sparseset_iterate(struct Sparseset const * sparseset, struct Sparseset_Iterator * iterator) {
	while (iterator->next < sparseset->packed.count) {
		uint32_t const index = iterator->next++;
		iterator->curr   = index;
		iterator->value  = array_at_unsafe(&sparseset->packed, index);
		iterator->handle = sparseset_get_handle_at(sparseset, index);
		return true;
	}
	return false;
}
