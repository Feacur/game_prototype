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
} gs_arena_system;

inline static size_t arena_system_checksum(void const * pointer) {
	return (size_t)pointer ^ 0x0123456789abcdef;
}

static void arena_system_fallback_free(void) {
	FOR_HASHSET(&gs_arena_system.fallback, it) {
		void * const * pointer = it.key;
		MEMORY_FREE(*pointer);
	}
}

void arena_system_init(void) {
	gs_arena_system = (struct Arena_System){
		.buffer = buffer_init(NULL),
		.buffered = array_init(sizeof(struct Memory_Header *)),
		.fallback = hashset_init(hash64, sizeof(struct Memory_Header *)),
	};
	// @note: so far the most offending was `stb_truetype`
	// with multiple 25kb per allocation without frees
	uint32_t const preallocate = 64 + 32;
	buffer_resize(&gs_arena_system.buffer, preallocate * (1 << 10));
	array_resize(&gs_arena_system.buffered, preallocate);
}

void arena_system_free(void) {
	arena_system_fallback_free();
	buffer_free(&gs_arena_system.buffer);
	array_free(&gs_arena_system.buffered);
	hashset_free(&gs_arena_system.fallback);
	common_memset(&gs_arena_system, 0, sizeof(gs_arena_system));
}

void arena_system_reset(void) {
	if (gs_arena_system.buffer.capacity < gs_arena_system.peak) {
		WRN(
			"> buffer system\n"
			"  capacity .. %zu\n"
			"  size ...... %zu\n"
			"  peak ...... %zu\n"
			""
			, gs_arena_system.buffer.capacity
			, gs_arena_system.buffer.size
			, gs_arena_system.peak
		);
		buffer_resize(&gs_arena_system.buffer, gs_arena_system.peak);
	}
	//
	arena_system_fallback_free();
	buffer_clear(&gs_arena_system.buffer);
	array_clear(&gs_arena_system.buffered);
	hashset_clear(&gs_arena_system.fallback);
	gs_arena_system.required = 0;
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

static void arena_system_pop(void * pointer) {
	if (pointer == NULL) { return; }

	struct Memory_Header * header = (struct Memory_Header *)pointer - 1;
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
}

static ALLOCATOR(arena_fallback) {
	if (pointer != NULL) {
		hashset_del(&gs_arena_system.fallback, &pointer);
		struct Memory_Header const * header = (struct Memory_Header *)pointer - 1;
		gs_arena_system.required -= sizeof(*header) + header->size;
	}

	void * const new_pointer = memory_reallocate(pointer, size);
	if (new_pointer == NULL) { return NULL; }

	hashset_set(&gs_arena_system.fallback, &new_pointer);

	gs_arena_system.required += sizeof(struct Memory_Header) + size;
	gs_arena_system.peak = max_size(gs_arena_system.peak, gs_arena_system.required);

	return new_pointer;
}

ALLOCATOR(arena_reallocate) {
	if (pointer == NULL) {
		if (size == 0) { return NULL; }

		void * const new_buffered = arena_system_push(size);
		if (new_buffered != NULL) { return new_buffered; }

		return arena_fallback(NULL, size);
	}

	// reallocate `buffered`
	struct Memory_Header const * header = (struct Memory_Header *)pointer - 1;
	if (header->checksum == arena_system_checksum(header)) {
		arena_system_pop(pointer);
		void * const new_buffered = arena_system_push(size);
		if (new_buffered == NULL && size != 0) {
			void * const new_fallback = arena_fallback(NULL, size);
			common_memcpy(new_fallback, pointer, min_size(size, header->size));
			return new_fallback;
		}
		return new_buffered;
	}

	// reallocate `fallback`
	void * const new_buffered = arena_system_push(size);
	if (new_buffered == NULL && size != 0) {
		return arena_fallback(pointer, size);
	}
	common_memcpy(new_buffered, pointer, min_size(size, header->size));
	arena_fallback(pointer, 0);
	return new_buffered;
}
