#include <stdlib.h>

//
#include "common.h"

void common_exit_failure(void) {
	exit(EXIT_FAILURE);
}

void common_qsort(void * data, size_t count, size_t value_size, int (* compare)(void const * v1, void const * v2)) {
	qsort(data, count, value_size, compare);
}
