#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "array_s32.h"

struct Array_S32 array_s32_init(void) {
	return (struct Array_S32){0};
}

void array_s32_free(struct Array_S32 * array) {
	MEMORY_FREE(array->data);
	common_memset(array, 0, sizeof(*array));
}

void array_s32_clear(struct Array_S32 * array) {
	array->count = 0;
}

void array_s32_resize(struct Array_S32 * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array->data, target_capacity);
}

void array_s32_ensure(struct Array_S32 * array, uint32_t target_capacity) {
	if (array->capacity < target_capacity) {
		array_s32_resize(array, target_capacity);
	}
}

static void array_s32_grow_if_must(struct Array_S32 * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_s32_resize(array, target_capacity);
	}
}

void array_s32_push_many(struct Array_S32 * array, uint32_t count, int32_t const * value) {
	array_s32_grow_if_must(array, array->count + count);
	if (value != NULL) {
		common_memcpy(
			array->data + array->count,
			value,
			sizeof(*array->data) * count
		);
	}
	array->count += count;
}

void array_s32_set_many(struct Array_S32 * array, uint32_t index, uint32_t count, int32_t const * value) {
	if (index + count > array->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	common_memcpy(
		array->data + index,
		value,
		sizeof(*array->data) * count
	);
}

int32_t array_s32_pop(struct Array_S32 * array) {
	if (array->count == 0) { DEBUG_BREAK(); return 0; }
	return array->data[--array->count];
}

int32_t array_s32_peek(struct Array_S32 const * array, uint32_t offset) {
	if (offset >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[array->count - offset - 1];
}

int32_t array_s32_at(struct Array_S32 const * array, uint32_t index) {
	if (index >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[index];
}
