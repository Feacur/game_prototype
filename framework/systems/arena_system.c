#include "framework/formatter.h"
#include "framework/maths.h"
#include "framework/platform/allocator.h"
#include "framework/containers/array.h"
#include "framework/containers/buffer.h"
#include "framework/containers/hashmap.h"
#include "framework/systems/memory_system.h"


//
#include "arena_system.h"

static struct Arena_System {
	struct Buffer buffer;
	struct Array buffered;   // `struct Memory_Header *`
	struct Hashmap fallback; // `struct Memory_Header *`
	size_t required, peak;
} gs_arena_system = {
	.buffer = {
		.allocate = platform_reallocate,
	},
	.buffered = {
		.allocate = platform_reallocate,
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
	if (deallocate) {
		gs_arena_system.peak = 0;
	}
	// growth
	if (gs_arena_system.buffer.capacity < gs_arena_system.peak) {
		WRN(
			"> arena system\n"
			"  capacity .. %zu\n"
			"  peak ...... %zu\n"
			""
			, gs_arena_system.buffer.capacity
			, gs_arena_system.peak
		);
		buffer_resize(&gs_arena_system.buffer, gs_arena_system.peak);
	}
	// dependencies
	FOR_HASHMAP(&gs_arena_system.fallback, it) {
		void * const * pointer = it.key;
		gs_arena_system.fallback.allocate(*pointer, 0);
	}
	// personal
	buffer_clear(&gs_arena_system.buffer, deallocate);
	array_clear(&gs_arena_system.buffered, deallocate);
	hashmap_clear(&gs_arena_system.fallback, deallocate);
	gs_arena_system.required = 0;
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
	gs_arena_system.buffer.size += buffered_size;

	struct Memory_Header * header = buffer_at(&gs_arena_system.buffer, offset);
	array_push_many(&gs_arena_system.buffered, 1, &header);

	*header = (struct Memory_Header){
		.checksum = arena_system_checksum(header),
		.size = size,
	};

	return header + 1;
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
		hashmap_del(&gs_arena_system.fallback, &pointer);
		struct Memory_Header const * header = (struct Memory_Header *)pointer - 1;
		gs_arena_system.required -= sizeof(*header) + header->size;
	}

	void * pointer1 = gs_arena_system.fallback.allocate(pointer, size);
	if (pointer1 == NULL) { return NULL; }

	if (!hashmap_set(&gs_arena_system.fallback, &pointer1, NULL)) {
		DEBUG_BREAK();
	}
	pointer = pointer1;

	gs_arena_system.required += sizeof(struct Memory_Header) + size;
	gs_arena_system.peak = max_size(gs_arena_system.peak, gs_arena_system.required);

	return pointer;
}

ALLOCATOR(arena_reallocate) {
	struct Memory_Header const * header = arena_system_pop(pointer);
	void * const buffered = arena_system_push(size);

	if (header != NULL) {
		// buffered -> buffered
		if (buffered != NULL) { return buffered; }
		if (size == 0) { return buffered; }

		// buffered -> fallback
		void * const fallback = arena_fallback(NULL, size);
		common_memcpy(fallback, pointer, min_size(size, header->size));
		return fallback;
	}

	// fallback -> fallback
	if (buffered == NULL && size != 0) {
		return arena_fallback(pointer, size);
	}

	// fallback -> buffered
	common_memcpy(buffered, pointer, size);
	arena_fallback(pointer, 0);
	return buffered;
}
