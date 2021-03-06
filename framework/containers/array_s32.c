#include "framework/memory.h"
#include "internal.h"

#include <string.h>

//
#include "array_s32.h"

void array_s32_init(struct Array_S32 * array) {
	*array = (struct Array_S32){0};
}

void array_s32_free(struct Array_S32 * array) {
	MEMORY_FREE(array, array->data);
	memset(array, 0, sizeof(*array));
}

void array_s32_clear(struct Array_S32 * array) {
	array->count = 0;
}

void array_s32_resize(struct Array_S32 * array, uint32_t target_capacity) {
	array->capacity = target_capacity;
	if (array->count > target_capacity) { array->count = target_capacity; }
	array->data = MEMORY_REALLOCATE_ARRAY(array, array->data, target_capacity);
}

static void array_s32_ensure_capacity(struct Array_S32 * array, uint32_t target_count) {
	if (array->capacity < target_count) {
		uint32_t const target_capacity = grow_capacity_value_u32(array->capacity, target_count - array->capacity);
		array_s32_resize(array, target_capacity);
	}
}

void array_s32_push(struct Array_S32 * array, int32_t value) {
	array_s32_ensure_capacity(array, array->count + 1);
	array->data[array->count++] = value;
}

void array_s32_push_many(struct Array_S32 * array, uint32_t count, int32_t const * value) {
	array_s32_ensure_capacity(array, array->count + count);
	memcpy(
		array->data + array->count,
		value,
		sizeof(*array->data) * count
	);
	array->count += count;
}

int32_t array_s32_pop(struct Array_S32 * array) {
	if (array->count == 0) { DEBUG_BREAK(); return 0; }
	return array->data[--array->count];
}

int32_t array_s32_peek(struct Array_S32 * array, uint32_t offset) {
	if (offset >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[array->count - offset - 1];
}

int32_t array_s32_at(struct Array_S32 * array, uint32_t index) {
	if (index >= array->count) { DEBUG_BREAK(); return 0; }
	return array->data[index];
}
