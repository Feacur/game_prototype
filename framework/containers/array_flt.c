#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "array_flt.h"

struct Array_Flt array_flt_init(void) {
	return (struct Array_Flt){0};
}

void array_flt_free(struct Array_Flt * array) {
	MEMORY_FREE(array->data);
	common_memset(array, 0, sizeof(*array));
}

void array_flt_clear(struct Array_Flt * array) {
	array->count = 0;
}

void array_flt_resize(struct Array_Flt * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, target_capacity);
}

void array_flt_ensure(struct Array_Flt * array, uint32_t target_capacity) {
	if (array->capacity < target_capacity) {
		array_flt_resize(array, target_capacity);
	}
}

static void array_flt_grow_if_must(struct Array_Flt * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_flt_resize(array, target_capacity);
	}
}

void array_flt_push(struct Array_Flt * array, float value) {
	array_flt_grow_if_must(array, array->count + 1);
	array->data[array->count++] = value;
}

void array_flt_push_many(struct Array_Flt * array, uint32_t count, float const * value) {
	array_flt_grow_if_must(array, array->count + count);
	if (value != NULL) {
		common_memcpy(
			array->data + array->count,
			value,
			sizeof(*array->data) * count
		);
	}
	array->count += count;
}

void array_flt_set_many(struct Array_Flt * array, uint32_t index, uint32_t count, float const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	common_memcpy(
		array->data + index,
		value,
		sizeof(*array->data) * count
	);
}

float array_flt_pop(struct Array_Flt * array) {
	if (array->count == 0) { DEBUG_BREAK(); return 0; }
	return array->data[--array->count];
}

float array_flt_peek(struct Array_Flt const * array, uint32_t offset) {
	if (offset >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[array->count - offset - 1];
}

float array_flt_at(struct Array_Flt const * array, uint32_t index) {
	if (index >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[index];
}