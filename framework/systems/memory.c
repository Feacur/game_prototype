#include "framework/formatter.h"
#include "framework/maths.h"

#include "framework/platform/allocator.h"
#include "framework/platform/debug.h"

#include "framework/containers/array.h"
#include "framework/containers/buffer.h"
#include "framework/containers/hashmap.h"


//
#include "memory.h"

// ----- ----- ----- ----- -----
//     Generic part
// ----- ----- ----- ----- -----

inline static size_t system_memory_generic_checksum(void const * pointer) {
	return (size_t)pointer;
}

ALLOCATOR(realloc_generic) {
	struct Memory_Header * header = (pointer != NULL)
		? (struct Memory_Header *)pointer - 1
		: NULL;

	if (header != NULL) {
		if (header->checksum != system_memory_generic_checksum(header)) {
			ERR("is not generic memory:");
			REPORT_CALLSTACK(); DEBUG_BREAK();
			return NULL;
		}
		header->checksum = 0;
	}

	header = platform_reallocate(header, (size != 0)
		? sizeof(*header) + size
		: 0
	);

	if (header != NULL) {
		*header = (struct Memory_Header){
			.checksum = system_memory_generic_checksum(header),
			.size = size,
		};

		return header + 1;
	}

	return NULL;
}

// ----- ----- ----- ----- -----
//     Debug party
// ----- ----- ----- ----- -----

static struct Memory_Header_Debug {
	struct Memory_Header_Debug * prev;
	struct Memory_Header_Debug * next;
	struct Callstack callstack;
	struct Memory_Header base;
} * gs_memory;

inline static size_t system_memory_debug_checksum(void const * pointer) {
	return ~(size_t)pointer;
}

ALLOCATOR(realloc_debug) {
	struct Memory_Header_Debug * header = (pointer != NULL)
		? (struct Memory_Header_Debug *)pointer - 1
		: NULL;

	if (header != NULL) {
		if (header->base.checksum != system_memory_debug_checksum(header)) {
			ERR("> debug memory:");
			PRINT_CALLSTACK(2, header->callstack);
			REPORT_CALLSTACK(); DEBUG_BREAK();
			return NULL;
		}
		header->base.checksum = 0;

		if (gs_memory == header) { gs_memory = header->next; }
		if (header->next != NULL) { header->next->prev = header->prev; }
		if (header->prev != NULL) { header->prev->next = header->next; }
	}

	header = platform_reallocate(header, (size != 0)
		? sizeof(*header) + size
		: 0
	);

	if (header != NULL) {
		*header = (struct Memory_Header_Debug){
			.base = {
				.checksum = system_memory_debug_checksum(header),
				.size = size,
			},
			.callstack = platform_debug_get_callstack(1),
		};

		header->next = gs_memory;
		if (gs_memory != NULL) { gs_memory->prev = header; }
		gs_memory = header;

		return header + 1;
	}

	return NULL;
}

void system_memory_debug_report(void) {
	if (gs_memory == NULL) { return; }

	uint32_t const pointer_digits_count = sizeof(size_t) * 2;

	uint32_t total_count = 0;
	uint64_t total_bytes = 0;
	for (struct Memory_Header_Debug * it = gs_memory; it; it = it->next) {
		total_count++;
		total_bytes += it->base.size;
	}

	uint32_t bytes_digits_count = 0;
	for (size_t v = total_bytes; v > 0; v = v / 10) {
		bytes_digits_count++;
	}

	{
		struct CString const header = S_("memory report");
		WRN(
			"> %-*.*s (bytes: %*zu | count: %u):"
			""
			, pointer_digits_count + 4 // compensate for [0x]
			, header.length, header.data
			, bytes_digits_count,  total_bytes
			, total_count
		);
	}

	for (struct Memory_Header_Debug const * it = gs_memory; it != NULL; it = it->next) {
		WRN(
			"  [%#.*zx] (bytes: %*zu) stacktrace:"
			""
			, pointer_digits_count, (size_t)(it + 1)
			, bytes_digits_count,   it->base.size
		);
		PRINT_CALLSTACK(2, it->callstack);
	}

	DEBUG_BREAK();
}

void system_memory_debug_clear(void) {
	while (gs_memory != NULL) {
		void * const header = gs_memory;
		gs_memory = gs_memory->next;
		platform_reallocate(header, 0);
	}
}

// ----- ----- ----- ----- -----
//     Arena part
// ----- ----- ----- ----- -----

static struct system_memory_Arena {
	struct Buffer buffer;
	struct Array buffered;   // `struct Memory_Header *`
	struct Hashmap fallback; // `struct Memory_Header *` : NULL
	size_t required, peak;
} gs_system_memory_arena = {
	.buffer = {
		.allocate = platform_reallocate,
	},
	.buffered = {
		.allocate = platform_reallocate,
		.value_size = sizeof(struct Memory_Header *),
	},
	.fallback = {
		.allocate = realloc_generic,
		.get_hash = hash64,
		.key_size = sizeof(struct Memory_Header *),
	},
};

inline static size_t system_memory_arena_checksum(void const * pointer) {
	return (size_t)pointer ^ 0x0123456789abcdef;
}

void system_memory_arena_clear(bool deallocate) {
	if (deallocate) {
		gs_system_memory_arena.peak = 0;
	}
	// growth
	if (gs_system_memory_arena.buffer.capacity < gs_system_memory_arena.peak) {
		WRN(
			"> arena system\n"
			"  capacity .. %zu\n"
			"  peak ...... %zu\n"
			""
			, gs_system_memory_arena.buffer.capacity
			, gs_system_memory_arena.peak
		);
		buffer_resize(&gs_system_memory_arena.buffer, gs_system_memory_arena.peak);
	}
	// dependencies
	FOR_HASHMAP(&gs_system_memory_arena.fallback, it) {
		void * const * pointer = it.key;
		gs_system_memory_arena.fallback.allocate(*pointer, 0);
	}
	// personal
	buffer_clear(&gs_system_memory_arena.buffer, deallocate);
	array_clear(&gs_system_memory_arena.buffered, deallocate);
	hashmap_clear(&gs_system_memory_arena.fallback, deallocate);
	gs_system_memory_arena.required = 0;
}

void system_memory_arena_ensure(size_t size) {
	buffer_resize(&gs_system_memory_arena.buffer, size);
	array_resize(&gs_system_memory_arena.buffered, (uint32_t)(size / (1 << 10)));
}

static void * system_memory_arena_push(size_t size) {
	if (size == 0) { return NULL; }

	size_t const buffered_size = sizeof(struct Memory_Header) + size;
	if (gs_system_memory_arena.buffer.size + buffered_size > gs_system_memory_arena.buffer.capacity) {
		return NULL;
	}

	gs_system_memory_arena.required += buffered_size;
	gs_system_memory_arena.peak = max_size(gs_system_memory_arena.peak, gs_system_memory_arena.required);

	size_t const offset = gs_system_memory_arena.buffer.size;
	gs_system_memory_arena.buffer.size += buffered_size;

	struct Memory_Header * header = buffer_at(&gs_system_memory_arena.buffer, offset);
	array_push_many(&gs_system_memory_arena.buffered, 1, &header);

	*header = (struct Memory_Header){
		.checksum = system_memory_arena_checksum(header),
		.size = size,
	};

	return header + 1;
}

static struct Memory_Header * system_memory_arena_pop(void * pointer) {
	if (pointer == NULL) { return NULL; }

	struct Memory_Header * header = (struct Memory_Header *)pointer - 1;
	if (header->checksum != system_memory_arena_checksum(header)) { return NULL; }
	header->checksum = 0;

	for (uint32_t i = gs_system_memory_arena.buffered.count; i > 0; i--) {
		void * const * it = array_at(&gs_system_memory_arena.buffered, i - 1);
		struct Memory_Header const * buffered = *it;
		if (buffered->checksum != 0) { break; }

		size_t const buffered_size = sizeof(*buffered) + buffered->size;
		gs_system_memory_arena.required -= buffered_size;

		gs_system_memory_arena.buffer.size -= buffered_size;
		gs_system_memory_arena.buffered.count -= 1;
	}

	return header;
}

static ALLOCATOR(arena_fallback) {
	if (pointer != NULL) {
		if (!hashmap_del(&gs_system_memory_arena.fallback, &pointer)) {
			DEBUG_BREAK();
		}
		struct Memory_Header const * header = (struct Memory_Header *)pointer - 1;
		gs_system_memory_arena.required -= sizeof(*header) + header->size;
	}

	void * pointer1 = gs_system_memory_arena.fallback.allocate(pointer, size);
	if (pointer1 == NULL) { return NULL; }

	if (!hashmap_set(&gs_system_memory_arena.fallback, &pointer1, NULL)) {
		DEBUG_BREAK();
	}
	pointer = pointer1;

	gs_system_memory_arena.required += sizeof(struct Memory_Header) + size;
	gs_system_memory_arena.peak = max_size(gs_system_memory_arena.peak, gs_system_memory_arena.required);

	return pointer;
}

ALLOCATOR(realloc_arena) {
	struct Memory_Header const * header = system_memory_arena_pop(pointer);
	void * const buffered = system_memory_arena_push(size);

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
