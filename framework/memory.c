#include <stdio.h>
#include <stdlib.h>

// @idea: use native OS backend and a custom allocators? if I ever want to learn that area deeper...

//
#include "memory.h"

// void * memory_allocate(size_t size) {
// 	void * result = malloc(size);
// 	if (result == NULL) { debug_log("'malloc' failed\n"); DEBUG_BREAK(); exit(EXIT_FAILURE); }
// 	return result;
// }

void * memory_reallocate(void * pointer, size_t size) {
	if (size == 0) {
		free(pointer);
		return NULL;
	}

	void * result = realloc(pointer, size);
	if (result == NULL) { fprintf(stderr, "'realloc' failed\n"); DEBUG_BREAK(); exit(EXIT_FAILURE); }
	return result;
}

// void memory_free(void * pointer) {
// 	free(pointer);
// }
