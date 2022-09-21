#include "framework/logger.h"
#include "framework/platform_debug.h"

#include <stdlib.h>

// @todo: custom specialized allocators
// @todo: put meta right into pointer headers instead of a dictionary? alongside?
//        keeping at least lightweight hashset allows us to verify pointers we own
//        but it's just an extra failsafe then
// @idea: use native OS backend and a custom allocators? if I ever want to learn that area deeper...
// @idea: use OS-native allocators instead of CRT's

//
#include "memory.h"

struct Memory_Header {
	size_t checksum, size;
	struct Memory_Header * prev, * next;
	struct Callstack callstack;
};

static struct Memory_State {
	struct Memory_Header * root;
	uint32_t count;
	size_t bytes;
} gs_memory_state;

void * memory_reallocate(void * pointer, size_t size) {
	struct Callstack const callstack = platform_debug_get_callstack();

	struct Memory_Header * pointer_header = (pointer != NULL)
		? (struct Memory_Header *)pointer - 1
		: NULL;

	if (pointer_header != NULL) {
		if (pointer_header->checksum != (size_t)pointer_header) {
			struct CString const stacktrace = platform_debug_get_stacktrace(callstack, 1);
			logger_to_console("incorrect checksum: \"%.*s\"\n", stacktrace.length, stacktrace.data); DEBUG_BREAK();
			return pointer;
		}

		// untrack memory
		gs_memory_state.count--;
		gs_memory_state.bytes -= pointer_header->size;

		pointer_header->checksum = 0;
		if (gs_memory_state.root == pointer_header) { gs_memory_state.root = pointer_header->next; }
		if (pointer_header->next != NULL) { pointer_header->next->prev = pointer_header->prev; }
		if (pointer_header->prev != NULL) { pointer_header->prev->next = pointer_header->next; }
	}

	// free
	if (size == 0) {
		if (pointer_header == NULL) { return NULL; }
		common_memset(pointer_header, 0, sizeof(*pointer_header) + pointer_header->size);
		free(pointer_header);
		return NULL;
	}

	// allocate or reallocate
	struct Memory_Header * reallocated_header = realloc(pointer_header, sizeof(*pointer_header) + size);
	if (reallocated_header != NULL) {
		gs_memory_state.count++;
		gs_memory_state.bytes += size;
		*reallocated_header = (struct Memory_Header){
			.checksum = (size_t)reallocated_header,
			.size = size,
			.callstack = callstack,
		};

		// track memory
		reallocated_header->next = gs_memory_state.root;
		if (gs_memory_state.root != NULL) { gs_memory_state.root->prev = reallocated_header; }
		gs_memory_state.root = reallocated_header;

		return reallocated_header + 1;
	}

	// failed
	struct CString const stacktrace = platform_debug_get_stacktrace(callstack, 1);
	logger_to_console("'realloc' failed: \"%.*s\"\n", stacktrace.length, stacktrace.data); DEBUG_BREAK();
	return pointer;
}

void * memory_reallocate_without_tracking(void * pointer, size_t size) {
	struct Callstack const callstack = platform_debug_get_callstack();

	// free
	if (size == 0) { free(pointer); return NULL; }

	// allocate or reallocate
	void * reallocated = realloc(pointer, size);
	if (reallocated != NULL) { return reallocated; }

	// failed
	struct CString const stacktrace = platform_debug_get_stacktrace(callstack, 1);
	logger_to_console("'realloc' failed: \"%.*s\"\n", stacktrace.length, stacktrace.data); DEBUG_BREAK();
	return pointer;
}

//
#include "framework/internal/memory_to_system.h"

bool memory_to_system_cleanup(void) {
	if (gs_memory_state.root == NULL) { return false; }

	uint32_t const pointer_digits_count = 12;

	uint32_t bytes_digits_count = 0;
	{
		for (size_t v = gs_memory_state.bytes; v > 0; v = v / 10) {
			bytes_digits_count++;
		}

		uint32_t const header_blank_offset = ((pointer_digits_count >= 8) ? (pointer_digits_count - 8) : 0);
		logger_to_console(
			"> Memory report%*s(bytes: %*.zu | count: %u):\n"
			,
			header_blank_offset, "",
			bytes_digits_count,  gs_memory_state.bytes,
			gs_memory_state.count
		);
	}

	for (struct Memory_Header const * it = gs_memory_state.root; it != NULL; it = it->next) {
		struct CString const stacktrace = platform_debug_get_stacktrace(it->callstack, 1);
		logger_to_console(
			"  [0x%0*.zx] (bytes: %*.zu) stacktrace:\n"
			"%.*s"
			,
			pointer_digits_count, (size_t)(it + 1),
			bytes_digits_count,   it->size,
			stacktrace.length,    stacktrace.data
		);
	}

	while (gs_memory_state.root != NULL) {
		void * pointer = (gs_memory_state.root + 1);
		memory_reallocate(pointer, 0);
	}

	return true;
}
