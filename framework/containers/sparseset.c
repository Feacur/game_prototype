#include "framework/formatter.h"
#include "framework/systems/memory_system.h"

#include "internal/helpers.h"


// @note: `id` of external `struct Handle` is expected to be `sparse index + 1`

//
#include "sparseset.h"

struct Sparseset sparseset_init(uint32_t value_size) {
	return (struct Sparseset){
		.payload = array_init(value_size),
		.sparse  = array_init(sizeof(struct Handle)),
		.packed  = array_init(sizeof(uint32_t)),
	};
}

void sparseset_free(struct Sparseset * sparseset) {
	array_free(&sparseset->payload);
	array_free(&sparseset->sparse);
	array_free(&sparseset->packed);
	common_memset(sparseset, 0, sizeof(*sparseset));
}

void sparseset_clear(struct Sparseset * sparseset, bool deallocate) {
	array_clear(&sparseset->payload, deallocate);
	array_clear(&sparseset->sparse, deallocate);
	array_clear(&sparseset->packed, deallocate); sparseset->packed.count = sparseset->packed.capacity;
	sparseset->h_free.id = 0;
	sparseset->h_free.gen++;
}

void sparseset_ensure(struct Sparseset * sparseset, uint32_t capacity) {
	if (capacity <= sparseset->payload.capacity) { return; }
	capacity = growth_adjust_array(sparseset->payload.capacity, capacity);

	array_resize(&sparseset->payload, capacity);
	array_resize(&sparseset->sparse,  capacity);
	array_resize(&sparseset->packed,  capacity); sparseset->packed.count = capacity;
}

struct Handle sparseset_aquire(struct Sparseset * sparseset, void const * value) {
	sparseset_ensure(sparseset, sparseset->payload.count + 1);

	uint32_t const handle_id = sparseset->h_free.id;

	// update the free list
	if (handle_id >= sparseset->sparse.count) {
		array_push_many(&sparseset->sparse, 1, &(struct Handle){
			.id = handle_id + 1,
		});
	}
	struct Handle * entry_handle = array_at(&sparseset->sparse, handle_id);
	sparseset->h_free.id = entry_handle->id;

	// claim the entry
	array_set_many(&sparseset->packed, sparseset->payload.count, 1, &handle_id);
	entry_handle->id = sparseset->payload.count;
	array_push_many(&sparseset->payload, 1, value);

	//
	return (struct Handle){
		.id = handle_id + 1,
		.gen = entry_handle->gen,
	};
}

void sparseset_discard(struct Sparseset * sparseset, struct Handle handle) {
	if (handle_is_null(handle))              { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (handle.id > sparseset->sparse.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle * entry_handle = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry_handle->gen) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	uint32_t const * entry_id = array_at(&sparseset->packed, entry_handle->id);
	if (handle_id  != *entry_id) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	// swap the removed entry with the last one, maintain packing
	void const * swap_pack = array_pop(&sparseset->payload, 1);
	if (entry_handle->id < sparseset->payload.count) {
		uint32_t const * swap_id = array_at(&sparseset->packed, sparseset->payload.count);
		struct Handle * swap_handle = array_at(&sparseset->sparse, *swap_id);
		if (swap_handle->id != sparseset->payload.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); }

		array_set_many(&sparseset->packed,    entry_handle->id, 1, swap_id);
		array_set_many(&sparseset->payload, entry_handle->id, 1, swap_pack);

		swap_handle->id = entry_handle->id;
	}

	// update the free list
	*entry_handle = (struct Handle){
		.id = sparseset->h_free.id,
		.gen = entry_handle->gen + 1,
	};
	sparseset->h_free.id = handle_id;
}

void * sparseset_get(struct Sparseset const * sparseset, struct Handle handle) {
	if (handle_is_null(handle))              { return NULL; }
	if (handle.id > sparseset->sparse.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry_handle = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry_handle->gen) { return NULL; }
	uint32_t const * entry_id = array_at(&sparseset->packed, entry_handle->id);
	if (handle_id  != *entry_id) { return NULL; }

	return array_at(&sparseset->payload, entry_handle->id);
}

void sparseset_set(struct Sparseset * sparseset, struct Handle handle, void const * value) {
	if (handle_is_null(handle))              { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (handle.id > sparseset->sparse.count) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	uint32_t const handle_id = handle.id - 1;
	struct Handle const * entry_handle = array_at(&sparseset->sparse, handle_id);
	if (handle.gen != entry_handle->gen) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	uint32_t const * entry_id = array_at(&sparseset->packed, entry_handle->id);
	if (handle_id  != *entry_id) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	array_set_many(&sparseset->payload, entry_handle->id, 1, value);
}

uint32_t sparseset_get_count(struct Sparseset const * sparseset) {
	return sparseset->payload.count;
}

void * sparseset_get_at(struct Sparseset const * sparseset, uint32_t index) {
	return array_at(&sparseset->payload, index);
}

void * sparseset_get_at_unsafe(struct Sparseset const * sparseset, uint32_t index) {
	return array_at_unsafe(&sparseset->payload, index);
}

static struct Handle sparseset_get_handle_at_unsafe(struct Sparseset const * sparseset, uint32_t index) {
	uint32_t const * id = array_at_unsafe(&sparseset->packed, index);
	struct Handle const * entry_handle = array_at_unsafe(&sparseset->sparse, *id);
	return (struct Handle){
		.id = *id + 1,
		.gen = entry_handle->gen,
	};
}

bool sparseset_iterate(struct Sparseset const * sparseset, struct Sparseset_Iterator * iterator) {
	while (iterator->next < sparseset->payload.count) {
		uint32_t const index = iterator->next++;
		iterator->curr   = index;
		iterator->value  = array_at_unsafe(&sparseset->payload, index);
		iterator->handle = sparseset_get_handle_at_unsafe(sparseset, index);
		return true;
	}
	return false;
}
