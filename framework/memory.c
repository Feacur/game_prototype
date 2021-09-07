#include "framework/logger.h"

#include <string.h>
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
	void const * owner;
	struct CString source;
};

static struct Memory_State { // static ZII
	struct Memory_Header * root;
	uint32_t count;
	size_t bytes;
} memory_state; // @note: global state

void * memory_reallocate(void const * owner, struct CString source, void * pointer, size_t size) {
	struct Memory_Header * pointer_header = (pointer != NULL)
		? (struct Memory_Header *)pointer - 1
		: NULL;

	if (pointer_header != NULL) {
		if (pointer_header->checksum != ~(size_t)pointer_header) {
			logger_to_console("incorrect checksum\n"); DEBUG_BREAK();
			return NULL;
		}

		if (memory_state.root == pointer_header) { memory_state.root = pointer_header->next; }
		if (pointer_header->next != NULL) { pointer_header->next->prev = pointer_header->prev; }
		if (pointer_header->prev != NULL) { pointer_header->prev->next = pointer_header->next; }
	}

	if (size == 0) {
		if (pointer_header == NULL) { return NULL; }
		memory_state.count--;
		memory_state.bytes -= pointer_header->size;
		memset(pointer_header, 0, sizeof(*pointer_header));
		free(pointer_header);
		return NULL;
	}

	struct Memory_Header * reallocated_header = realloc(pointer_header, sizeof(*pointer_header) + size);
	if (reallocated_header == NULL) {
		logger_to_console("'realloc' failed: \"%.*s\"\n", source.length, source.data); DEBUG_BREAK();
		exit(EXIT_FAILURE);
	}

	memory_state.count++;
	memory_state.bytes += size;
	*reallocated_header = (struct Memory_Header){
		.checksum = ~(size_t)reallocated_header,
		.size = size,
		.next = memory_state.root,
		.owner = owner,
		.source = source,
	};

	if (memory_state.root != NULL) { memory_state.root->prev = reallocated_header; }
	memory_state.root = reallocated_header;

	return reallocated_header + 1;
}

//
#include "framework/internal/memory_to_system.h"

uint32_t memory_to_system_report(void) {
	uint32_t const pointer_digits_count = 12;

	uint32_t bytes_digits_count = 0;
	for (size_t v = memory_state.bytes; v > 0; v = v / 10) {
		bytes_digits_count++;
	}

	logger_to_console(
		"\n"
		"> memory report%*s(bytes: %-*zu | count: %u):\n"
		"",
		((pointer_digits_count >= 8) ? (pointer_digits_count - 8) : 0), "",
		bytes_digits_count,
		memory_state.bytes,
		memory_state.count
	);
	uint32_t count = 0;
	if (memory_state.root != NULL) {
		for (struct Memory_Header * it = memory_state.root; it != NULL; it = it->next) {
			logger_to_console(
				"  [0x%0*zx] (bytes: %-*.zu | owner: 0x%0*zx) at '%.*s'\n",
				pointer_digits_count,
				(size_t)(it + 1),
				bytes_digits_count,
				it->size,
				pointer_digits_count,
				(size_t)it->owner,
				it->source.length,
				it->source.data
			);
			count++;
		}
	}
	return count;
}
