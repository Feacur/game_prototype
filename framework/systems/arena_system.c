#include "framework/formatter.h"
#include "framework/memory.h"
#include "framework/maths.h"
#include "framework/containers/array.h"
#include "framework/containers/buffer.h"
#include "framework/containers/hashset.h"

//
#include "arena_system.h"

static struct Arena_System {
	struct Buffer buffer;
	struct Array buffered;   // `struct Memory_Header *`
	struct Hashset fallback; // `struct Memory_Header *`
	size_t required, peak;
} gs_arena_system = {
	.buffer = {
		.allocate = generic_reallocate,
	},
	.buffered = {
		.allocate = generic_reallocate,
		.value_size = sizeof(struct Memory_Header *),
	},
	.fallback = {
		.allocate = generic_reallocate,
		.get_hash = hash64,
		.key_size = sizeof(struct Memory_Header *),
	},
};

inline static size_t arena_system_checksum(void const * pointer) {
	return (size_t)pointer ^ 0x0123456789abcdef;
}

void arena_system_clear(bool deallocate) {
	// dependencies
	FOR_HASHSET(&gs_arena_system.fallback, it) {
		void * const * pointer = it.key;
		gs_arena_system.fallback.allocate(*pointer, 0);
	}
	// personal
	buffer_clear(&gs_arena_system.buffer, deallocate);
	array_clear(&gs_arena_system.buffered, deallocate);
	hashset_clear(&gs_arena_system.fallback, deallocate);
	gs_arena_system.required = 0;
	//
	if (deallocate) {
		gs_arena_system.peak = 0;
	}
	//
	if (gs_arena_system.buffer.capacity < gs_arena_system.peak) {
		WRN(
			"> buffer system\n"
			"  capacity .. %zu\n"
			"  peak ...... %zu\n"
			""
			, gs_arena_system.buffer.capacity
			, gs_arena_system.peak
		);
		buffer_resize(&gs_arena_system.buffer, gs_arena_system.peak);
	}
}

void arena_system_ensure(size_t size) {
	buffer_resize(&gs_arena_system.buffer, size);
	array_resize(&gs_arena_system.buffered, (uint32_t)(size / (1 << 10)));
}

static void * arena_system_push(size_t size) {
	if (size == 0) { return NULL; }

	size_t const buffered_size = sizeof(struct Memory_Header) + size;
	if (gs_arena_system.buffer.size + buffered_size > gs_arena_system.buffer.capacity) {
		return NULL;
	}

	gs_arena_system.required += buffered_size;
	gs_arena_system.peak = max_size(gs_arena_system.peak, gs_arena_system.required);

	size_t const offset = gs_arena_system.buffer.size;
	gs_arena_system.buffer.size = gs_arena_system.required;

	struct Memory_Header * new_header = buffer_at(&gs_arena_system.buffer, offset);
	array_push_many(&gs_arena_system.buffered, 1, &new_header);

	*new_header = (struct Memory_Header){
		.checksum = arena_system_checksum(new_header),
		.size = size,
	};

	return new_header + 1;
}

static struct Memory_Header * arena_system_pop(void * pointer) {
	if (pointer == NULL) { return NULL; }

	struct Memory_Header * header = (struct Memory_Header *)pointer - 1;
	if (header->checksum != arena_system_checksum(header)) { return NULL; }
	header->checksum = 0;

	for (uint32_t i = 0; i < gs_arena_system.buffered.count; i++) {
		void * const * it = array_peek(&gs_arena_system.buffered, i);
		struct Memory_Header const * buffered = *it;
		if (buffered->checksum != 0) { break; }

		size_t const buffered_size = sizeof(*buffered) + buffered->size;
		gs_arena_system.required -= buffered_size;

		gs_arena_system.buffer.size -= buffered_size;
		gs_arena_system.buffered.count -= 1;
	}

	return header;
}

static ALLOCATOR(arena_fallback) {
	if (pointer != NULL) {
		hashset_del(&gs_arena_system.fallback, &pointer);
		struct Memory_Header const * header = (struct Memory_Header *)pointer - 1;
		gs_arena_system.required -= sizeof(*header) + header->size;
	}

	void * const new_pointer = gs_arena_system.fallback.allocate(pointer, size);
	if (new_pointer == NULL) { return NULL; }

	hashset_set(&gs_arena_system.fallback, &new_pointer);

	gs_arena_system.required += sizeof(struct Memory_Header) + size;
	gs_arena_system.peak = max_size(gs_arena_system.peak, gs_arena_system.required);

	return new_pointer;
}

ALLOCATOR(arena_reallocate) {
	struct Memory_Header const * header = arena_system_pop(pointer);
	void * const new_buffered = arena_system_push(size);

	if (header != NULL) {
		// buffered -> buffered
		if (new_buffered != NULL) { return new_buffered; }
		if (size == 0) { return new_buffered; }

		// buffered -> fallback
		void * const new_fallback = arena_fallback(NULL, size);
		common_memcpy(new_fallback, pointer, min_size(size, header->size));
		return new_fallback;
	}

	// fallback -> fallback
	if (new_buffered == NULL && size != 0) {
		return arena_fallback(pointer, size);
	}

	// fallback -> buffered
	common_memcpy(new_buffered, pointer, size);
	FREE(pointer);
	return new_buffered;
}
