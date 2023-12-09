#include "framework/memory.h"
#include "framework/containers/buffer.h"

//
#include "buffer_system.h"

static struct Buffer_System {
	struct Buffer buffer;
	struct Buffer_System_Transient {
		struct Buffer_System_Transient * next;
		uint8_t payload[FLEXIBLE_ARRAY];
	} * transient;
} gs_buffer_system;

static void buffer_system_reset_transient(void) {
	while (gs_buffer_system.transient != NULL) {
		void * to_free = gs_buffer_system.transient;
		gs_buffer_system.transient = gs_buffer_system.transient->next;
		MEMORY_FREE(to_free);
	}
}

void buffer_system_init(void) {
	gs_buffer_system = (struct Buffer_System){
		.buffer = buffer_init(NULL),
	};
}

void buffer_system_free(void) {
	buffer_system_reset_transient();
	buffer_free(&gs_buffer_system.buffer);
	// common_memset(&gs_buffer_system, 0, sizeof(gs_buffer_system));
}

void * buffer_system_get(size_t size) {
	if (size == 0) { return NULL; }

	// @note: accumulate size, even past the capacity
	size_t const offset = gs_buffer_system.buffer.size;
	gs_buffer_system.buffer.size += size;

	// @note: use the buffer until it's memory should be reallocated
	if (gs_buffer_system.buffer.size > gs_buffer_system.buffer.capacity) {
		struct Buffer_System_Transient * transient = MEMORY_ALLOCATE_SIZE(sizeof(struct Buffer_System_Transient) + size);
		transient->next = gs_buffer_system.transient;
		gs_buffer_system.transient = transient;
		return transient->payload;
	}

	return buffer_at(&gs_buffer_system.buffer, offset);
}

void buffer_system_reset(void) {
	buffer_system_reset_transient();
	buffer_ensure(&gs_buffer_system.buffer, gs_buffer_system.buffer.size);
	buffer_clear(&gs_buffer_system.buffer);
}
