#include "containers/hash_table_u64.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// @idea: use native OS backend and a custom allocators? if I ever want to learn that area deeper...

struct Pointer_Data {
	size_t size;
	void * owner;
};

static struct Memory_State {
	uint64_t total;
	struct Hash_Table_U64 * pointers;
} memory_state;

//
#include "memory.h"

// void * memory_allocate(void * owner, size_t size) {
// 	void * result = malloc(size);
// 	if (result == NULL) { debug_log("'malloc' failed\n"); DEBUG_BREAK(); exit(EXIT_FAILURE); }
// 	return result;
// }

void * memory_reallocate(void * owner, void * pointer, size_t size) {
	struct Pointer_Data const * pointer_data = NULL;

	if (memory_state.pointers != NULL && memory_state.pointers != owner) {
		pointer_data = (struct Pointer_Data *)hash_table_u64_get(memory_state.pointers, (uint64_t)pointer);
		hash_table_u64_del(memory_state.pointers, (uint64_t)pointer);
	}

	if (size == 0) {
		if (pointer_data != NULL) { memory_state.total -= pointer_data->size; }
		free(pointer);
		return NULL;
	}

	void * result = realloc(pointer, size);
	if (result == NULL) { fprintf(stderr, "'realloc' failed\n"); DEBUG_BREAK(); exit(EXIT_FAILURE); }

	if (memory_state.pointers != NULL && memory_state.pointers != owner) {
		memory_state.total += size;
		struct Pointer_Data const value = {
			.size = size,
			.owner = owner,
		};
		hash_table_u64_set(memory_state.pointers, (uint64_t)result, &value);
	}

	return result;
}

// void memory_free(void * owner, void * pointer) {
// 	free(pointer);
// }

//
#include "framework/internal/memory_to_system.h"

void memory_to_system_init(void) {
	memory_state.pointers = hash_table_u64_init(sizeof(struct Pointer_Data));
}

void memory_to_system_free(void) {
	hash_table_u64_free(memory_state.pointers);
	memset(&memory_state, 0, sizeof(memory_state));
}
