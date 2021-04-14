#include "framework/memory.h"

#include <string.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

//
#include "array_u32.h"

void array_u32_init(struct Array_U32 * array) {
	*array = (struct Array_U32){
		.capacity = 0,
		.count = 0,
		.data = 0,
	};
}

void array_u32_free(struct Array_U32 * array) {
	if (array->capacity == 0) { return; }
	MEMORY_FREE(array, array->data);
	array_u32_init(array);
}

void array_u32_resize(struct Array_U32 * array, uint32_t size) {
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, size);
	array->capacity = size;
	array->count = (size >= array->count) ? array->count : size;
}

void array_u32_write(struct Array_U32 * array, uint32_t value) {
	if (array->count + 1 > array->capacity) {
		// @todo: correctly process capacities past 0x80000000
		array->capacity = GROW_CAPACITY(array->capacity);
		array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, array->capacity);
	}

	array->data[array->count] = value;
	array->count++;
}

void array_u32_write_many(struct Array_U32 * array, uint32_t count, uint32_t const * value) {
	if (array->count + count > array->capacity) {
		// @todo: correctly process capacities past 0x80000000
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, array->capacity);
	}

	memcpy(array->data + array->count, value, sizeof(*value) * count);
	array->count += count;
}

void array_u32_write_many_zeroes(struct Array_U32 * array, uint32_t count) {
	if (array->count + count > array->capacity) {
		// @todo: correctly process capacities past 0x80000000
		while (array->count + count > array->capacity) {
			array->capacity = GROW_CAPACITY(array->capacity);
		}
		array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, array->capacity);
	}

	memset(array->data + array->count, 0, sizeof(*array->data) * count);
	array->count += count;
}

#undef GROWTH_FACTOR
#undef GROW_CAPACITY
