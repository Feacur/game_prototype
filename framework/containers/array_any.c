#include "framework/memory.h"

#include <stdio.h>
#include <string.h>

// @todo: use 2/3 as a growth factor here? no real need to be a power of 2
#define GROWTH_FACTOR 2
// #define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

struct Array_Any {
	uint32_t value_size;
	uint32_t capacity, count;
	uint8_t * data;
};

#if GROWTH_FACTOR == 2
	// #include "framework/maths.h"
	uint32_t round_up_to_PO2_u32(uint32_t value);
#endif

//
#include "array_any.h"

struct Array_Any * array_any_init(uint32_t value_size) {
	if (value_size == 0) {
		value_size = 1;
		fprintf(stderr, "value size should be non-zero\n"); DEBUG_BREAK(); return NULL;
	}

	struct Array_Any * array = MEMORY_ALLOCATE(struct Array_Any);
	*array = (struct Array_Any){
		.value_size = value_size,
	};
	return array;
}

void array_any_free(struct Array_Any * array) {
	MEMORY_FREE(array->data);

	memset(array, 0, sizeof(*array));
	MEMORY_FREE(array);
}

void array_any_clear(struct Array_Any * array) {
	array->count = 0;
}

void array_any_ensure_minimum_capacity(struct Array_Any * array, uint32_t minimum_capacity) {
	if (minimum_capacity < 8) { minimum_capacity = 8; }
#if GROWTH_FACTOR == 2
	if (minimum_capacity > 0x80000000) {
		minimum_capacity = 0x80000000;
		fprintf(stderr, "requested capacity is too large\n"); DEBUG_BREAK();
	}
	minimum_capacity = round_up_to_PO2_u32(minimum_capacity);
#endif

	array->capacity = minimum_capacity;
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, array->capacity * array->value_size);
}

uint32_t array_any_get_count(struct Array_Any * array) {
	return array->count;
}

void array_any_push(struct Array_Any * array, void const * value) {
	if (array->count + 1 > array->capacity) {
		array_any_ensure_minimum_capacity(array, array->capacity + 1);
	}

	memcpy(
		array->data + array->count * array->value_size,
		value,
		array->value_size * sizeof(*array->data)
	);
	array->count++;
}

void * array_any_pop(struct Array_Any * array) {
	if (array->count == 0) { return NULL; }
	array->count--;
	return array->data + array->count * array->value_size;
}

void * array_any_peek(struct Array_Any * array, uint32_t offset) {
	if (offset >= array->count) { return NULL; }
	return array->data + (array->count - offset - 1) * array->value_size;
}

void * array_any_at(struct Array_Any * array, uint32_t index) {
	if (index >= array->count) { return NULL; }
	return array->data + index * array->value_size;
}

#undef GROWTH_FACTOR
// #undef GROW_CAPACITY