#include "framework/formatter.h"
#include "framework/memory.h"
#include "framework/containers/array.h"
#include "framework/containers/buffer.h"

//
#include "buffer_system.h"

static struct Buffer_System {
	struct Buffer buffer;
	struct Array buffered;  // `struct Memory_Header *`
	struct Array transient; // `void *`
} gs_buffer_system;

static void buffer_system_reset_transient(void) {
	FOR_ARRAY(&gs_buffer_system.transient, it) {
		void * const * transient = it.value;
		MEMORY_FREE(*transient);
	}
	array_clear(&gs_buffer_system.transient);
}

void buffer_system_init(void) {
	gs_buffer_system = (struct Buffer_System){
		.buffer = buffer_init(NULL),
		.buffered = array_init(sizeof(struct Memory_Header *)),
		.transient = array_init(sizeof(void *)),
	};
	buffer_resize(&gs_buffer_system.buffer, (1 << 16) * 2);
}

void buffer_system_free(void) {
	buffer_system_reset_transient();
	buffer_free(&gs_buffer_system.buffer);
	array_free(&gs_buffer_system.buffered);
	array_free(&gs_buffer_system.transient);
	// common_memset(&gs_buffer_system, 0, sizeof(gs_buffer_system));
}

void * buffer_system_push(size_t size) {
	if (size == 0) { return NULL; }

	// @note: accumulate size, even past the capacity
	size_t const offset = gs_buffer_system.buffer.size;
	gs_buffer_system.buffer.size += sizeof(struct Memory_Header) + size;

	// @note: use the buffer until it's memory should be reallocated
	if (gs_buffer_system.buffer.size > gs_buffer_system.buffer.capacity) {
		WRN(
			"> buffer system\n"
			"  limit ... %zu\n"
			"  offset .. %zu\n"
			"  pushed .. %zu\n"
			""
			, gs_buffer_system.buffer.capacity
			, offset
			, size
		);
		void * transient = MEMORY_ALLOCATE_SIZE(size);
		array_push_many(&gs_buffer_system.transient, 1, &transient);
		return transient;
	}

	struct Memory_Header * buffered = buffer_at(&gs_buffer_system.buffer, offset);
	*buffered = (struct Memory_Header){
		.checksum = ~(size_t)buffered,
		.size = size,
	};
	array_push_many(&gs_buffer_system.buffered, 1, &buffered);
	return buffered + 1;
}

void buffer_system_pop(void * pointer) {
	if (pointer == NULL) { return; }
	if (gs_buffer_system.buffered.count == 0) { return; }
	if (gs_buffer_system.transient.count > 0) { return; }

	struct Memory_Header * pointer_header = (struct Memory_Header *)pointer - 1;
	if (pointer_header->checksum != ~(size_t)pointer_header) { return; }

	pointer_header->checksum = 0;
	for (uint32_t i = 0; i < gs_buffer_system.buffered.count; i++) {
		void * const * entry = array_peek(&gs_buffer_system.buffered, i);
		struct Memory_Header * buffered = *entry;
		if (buffered->checksum != 0) { break; }

		gs_buffer_system.buffer.size -= sizeof(*buffered);
		gs_buffer_system.buffer.size -= buffered->size;
		array_pop(&gs_buffer_system.buffered, 1);
	}
}

void buffer_system_reset(void) {
	buffer_ensure(&gs_buffer_system.buffer, gs_buffer_system.buffer.size);
	buffer_clear(&gs_buffer_system.buffer);
	array_clear(&gs_buffer_system.buffered);
	buffer_system_reset_transient();
}
