#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

#include <string.h>

//
#include "array_u64.h"

void array_u64_init(struct Array_U64 * array) {
	*array = (struct Array_U64){0};
}

void array_u64_free(struct Array_U64 * array) {
	MEMORY_FREE(array, array->data);
	memset(array, 0, sizeof(*array));
}

void array_u64_clear(struct Array_U64 * array) {
	array->count = 0;
}

void array_u64_resize(struct Array_U64 * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, target_capacity);
}

static void array_u64_ensure_capacity(struct Array_U64 * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_u64_resize(array, target_capacity);
	}
}

void array_u64_push(struct Array_U64 * array, uint64_t value) {
	array_u64_ensure_capacity(array, array->count + 1);
	array->data[array->count++] = value;
}

void array_u64_push_many(struct Array_U64 * array, uint32_t count, uint64_t const * value) {
	array_u64_ensure_capacity(array, array->count + count);
	if (value != NULL) {
		memcpy(
			array->data + array->count,
			value,
			sizeof(*array->data) * count
		);
	}
	array->count += count;
}

void array_u64_set_many(struct Array_U64 * array, uint32_t index, uint32_t count, uint64_t const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	memcpy(
		array->data + index,
		value,
		sizeof(*array->data) * count
	);
}

uint64_t array_u64_pop(struct Array_U64 * array) {
	if (array->count == 0) { DEBUG_BREAK(); return 0; }
	return array->data[--array->count];
}

uint64_t array_u64_peek(struct Array_U64 const * array, uint32_t offset) {
	if (offset >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[array->count - offset - 1];
}

uint64_t array_u64_at(struct Array_U64 const * array, uint32_t index) {
	if (index >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[index];
}
