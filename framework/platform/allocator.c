#include "framework/formatter.h"

#include <stdlib.h>


// @idea: use OS-native allocators instead of CRT's

//
#include "allocator.h"

ALLOCATOR(platform_reallocate) {
	if (size == 0) { free(pointer); return NULL; }

	pointer = realloc(pointer, size);
	if (pointer != NULL) { return pointer; }

	ERR("'realloc' failed:");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return NULL;
}
