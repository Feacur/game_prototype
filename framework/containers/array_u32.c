#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "array_u32.h"

struct Array_U32 array_u32_init(void) {
	return (struct Array_U32){0};
}

void array_u32_free(struct Array_U32 * array) {
	MEMORY_FREE(array, array->data);
	common_memset(array, 0, sizeof(*array));
}

void array_u32_clear(struct Array_U32 * array) {
	array->count = 0;
}

void array_u32_resize(struct Array_U32 * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, target_capacity);
}

static void array_u32_ensure_capacity(struct Array_U32 * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_u32_resize(array, target_capacity);
	}
}

void array_u32_push(struct Array_U32 * array, uint32_t value) {
	array_u32_ensure_capacity(array, array->count + 1);
	array->data[array->count++] = value;
}

void array_u32_push_many(struct Array_U32 * array, uint32_t count, uint32_t const * value) {
	array_u32_ensure_capacity(array, array->count + count);
	if (value != NULL) {
		common_memcpy(
			array->data + array->count,
			value,
			sizeof(*array->data) * count
		);
	}
	array->count += count;
}

void array_u32_set_many(struct Array_U32 * array, uint32_t index, uint32_t count, uint32_t const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	common_memcpy(
		array->data + index,
		value,
		sizeof(*array->data) * count
	);
}

uint32_t array_u32_pop(struct Array_U32 * array) {
	if (array->count == 0) { DEBUG_BREAK(); return 0; }
	return array->data[--array->count];
}

uint32_t array_u32_peek(struct Array_U32 const * array, uint32_t offset) {
	if (offset >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[array->count - offset - 1];
}

uint32_t array_u32_at(struct Array_U32 const * array, uint32_t index) {
	if (index >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[index];
}
