#include <stdio.h>
#include <stdlib.h>

//
#include "memory.h"

// void * memory_allocate(size_t size) {
// 	void * result = malloc(size);
// 	if (result == NULL) { debug_log("'malloc' failed\n"); DEBUG_BREAK(); exit(1); }
// 	return result;
// }

void * memory_reallocate(void * pointer, size_t size) {
	if (size == 0) {
		free(pointer);
		return NULL;
	}

	void * result = realloc(pointer, size);
	if (result == NULL) { fprintf(stderr, "'realloc' failed\n"); DEBUG_BREAK(); exit(1); }
	return result;
}

// void memory_free(void * pointer) {
// 	free(pointer);
// }
