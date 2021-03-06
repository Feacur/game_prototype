#include "memory.h"

#include <string.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR) \

//
#include "array_float.h"

void array_float_init(struct Array_Float * array) {
	*array = (struct Array_Float){
		.capacity = 0,
		.count = 0,
		.data = 0,
	};
}

void array_float_free(struct Array_Float * array) {
	if (array->capacity == 0 && array->data != NULL) { return; }
	MEMORY_FREE(array->data);
	array_float_init(array);
}

void array_float_resize(struct Array_Float * array, uint32_t size) {
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, size);
	array->capacity = size;
	array->count = (size >= array->count) ? array->count : size;
}

void array_float_write(struct Array_Float * array, float value) {
	if (array->count + 1 > array->capacity) {
		array->capacity = GROW_CAPACITY(array->capacity);
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	array->data[array->count] = value;
	array->count++;
}

void array_float_write_many(struct Array_Float * array, uint32_t count, float const * value) {
	if (array->count + count > array->capacity) {
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity);
	}

	memcpy(array->data + array->count, value, count * sizeof(*value));
	array->count += count;
}
