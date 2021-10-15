#include <stdlib.h>
#include <string.h>

//
#include "common.h"

// ----- ----- ----- ----- -----
//     standard
// ----- ----- ----- ----- -----

void common_exit_failure(void) {
	exit(EXIT_FAILURE);
}

void common_memset(void * target, uint8_t value, size_t size) {
	memset(target, value, size);
}

void common_memcpy(void * target, void const * source, size_t size) {
	memcpy(target, source, size);
}

int common_memcmp(void const * buffer_1, void const * buffer_2, size_t size) {
	return memcmp(buffer_1, buffer_2, size);
}

void common_qsort(void * data, size_t count, size_t value_size, int (* compare)(void const * v1, void const * v2)) {
	qsort(data, count, value_size, compare);
}

char const * common_strstr(char const * buffer, char const * value) {
	return strstr(buffer, value);
}

int32_t common_strncmp(char const * buffer_1, char const * buffer_2, size_t size) {
	return strncmp(buffer_1, buffer_2, size);
}
