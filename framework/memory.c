#include "containers/hash_table_u64.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// @idea: use native OS backend and a custom allocators? if I ever want to learn that area deeper...

struct Pointer_Data {
	size_t size;
	void const * owner;
	char const * source;
};

static struct Memory_State {
	uint64_t total_bytes;
	struct Hash_Table_U64 * pointers;
	uint32_t pointers_count;
} memory_state;

//
#include "memory.h"

// void * memory_allocate(void const * owner, char const * source, size_t size) {
// 	bool const track_allocation = (memory_state.pointers != NULL && memory_state.pointers != owner);
// 
// 	void * result = malloc(size);
// 	if (result == NULL) { debug_log("'malloc' failed\n"); DEBUG_BREAK(); exit(EXIT_FAILURE); }
// 
// 	if (track_allocation) {
// 		memory_state.pointers_count++;
// 		memory_state.total_bytes += size;
// 		hash_table_u64_set(memory_state.pointers, (uint64_t)result, &(struct Pointer_Data){
// 			.size   = size,
// 			.owner  = owner,
// 			.source = source,
// 		});
// 	}
// 
// 	return result;
// }

void * memory_reallocate(void const * owner, char const * source, void * pointer, size_t size) {
	bool const track_allocation = (memory_state.pointers != NULL && memory_state.pointers != owner);
	struct Pointer_Data const * pointer_data = NULL;
	if (pointer != NULL && track_allocation) {
		pointer_data = hash_table_u64_get(memory_state.pointers, (uint64_t)pointer);
	}

	if (size == 0) {
		free(pointer);
		if (pointer_data != NULL) {
			memory_state.pointers_count--;
			memory_state.total_bytes -= pointer_data->size;
			hash_table_u64_del(memory_state.pointers, (uint64_t)pointer);
		}
		return NULL;
	}

	void * result = realloc(pointer, size);
	if (result == NULL) { fprintf(stderr, "'realloc' failed\n"); DEBUG_BREAK(); exit(EXIT_FAILURE); }

	if (pointer_data != NULL) {
		memory_state.pointers_count--;
		memory_state.total_bytes -= pointer_data->size;
		hash_table_u64_del(memory_state.pointers, (uint64_t)pointer);
	}

	if (track_allocation) {
		memory_state.pointers_count++;
		memory_state.total_bytes += size;
		hash_table_u64_set(memory_state.pointers, (uint64_t)result, &(struct Pointer_Data){
			.size   = size,
			.owner  = owner,
			.source = source,
		});
	}

	return result;
}

// void memory_free(void const * owner, char const * source, void * pointer) {
// 	bool const track_allocation = (memory_state.pointers != NULL && memory_state.pointers != owner);
// 	struct Pointer_Data const * pointer_data = NULL;
// 	if (pointer != NULL && track_allocation) {
// 		pointer_data = hash_table_u64_get(memory_state.pointers, (uint64_t)pointer);
// 	}
// 
// 	free(pointer);
// 
// 	if (pointer_data != NULL) {
// 		memory_state.pointers_count--;
// 		memory_state.total_bytes -= pointer_data->size;
// 		hash_table_u64_del(memory_state.pointers, (uint64_t)pointer);
// 	}
// }

//
#include "framework/internal/memory_to_system.h"

void memory_to_system_init(void) {
	memory_state.pointers = hash_table_u64_init(sizeof(struct Pointer_Data));
}

void memory_to_system_free(void) {
	if (memory_state.total_bytes > 0 || memory_state.pointers_count > 0) {
		fprintf(stderr, "leaked %llu bytes with %u pointers(s):\n", memory_state.total_bytes, memory_state.pointers_count);

		uint32_t const iteration_capacity = hash_table_u64_get_iteration_capacity(memory_state.pointers);
		for (uint32_t i = 0; i < iteration_capacity; i++) {
			struct Pointer_Data const * value = hash_table_u64_iterate(memory_state.pointers, i);
			if (value == NULL) { continue; }
			fprintf(stderr, "-- %llu bytes from '%s'\n", value->size, value->source);
		}

		DEBUG_BREAK();
	}
	hash_table_u64_free(memory_state.pointers);
	memset(&memory_state, 0, sizeof(memory_state));
}
