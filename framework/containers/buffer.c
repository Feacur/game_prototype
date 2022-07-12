#include "framework/memory.h"
#include "framework/logger.h"
#include "internal.h"

//
#include "buffer.h"

struct Buffer buffer_init(Allocator * allocator) {
	return (struct Buffer){
		.allocator = allocator,
	};
}

void buffer_free(struct Buffer * buffer) {
	default_allocator(buffer->allocator)(buffer->data, 0);
	common_memset(buffer, 0, sizeof(*buffer));
}

void buffer_clear(struct Buffer * buffer) {
	buffer->count = 0;
}

void buffer_resize(struct Buffer * buffer, size_t target_capacity) {
	buffer->capacity = target_capacity;
	if (buffer->count > target_capacity) { buffer->count = target_capacity; }
	buffer->data = default_allocator(buffer->allocator)(buffer->data, target_capacity);
}

void buffer_ensure(struct Buffer * buffer, size_t target_capacity) {
	if (buffer->capacity < target_capacity) {
		buffer_resize(buffer, target_capacity);
	}
}

static void buffer_grow_if_must(struct Buffer * buffer, size_t target_count) {
	if (buffer->capacity < target_count) {
		size_t const target_capacity = grow_capacity_value_u64(buffer->capacity, target_count - buffer->capacity);
		buffer_resize(buffer, target_capacity);
	}
}

void buffer_push_many(struct Buffer * buffer, size_t count, void const * value) {
	buffer_grow_if_must(buffer, buffer->count + count);
	if (value != NULL) {
		common_memcpy(
			(uint8_t *)buffer->data + buffer->count,
			value, count
		);
	}
	buffer->count += count;
}

void buffer_align(struct Buffer * buffer) {
	size_t const target_count = align_u64(buffer->count);
	buffer_grow_if_must(buffer, target_count);
	buffer->count = target_count;
}

void buffer_set_many(struct Buffer * buffer, size_t index, size_t count, uint8_t const * value) {
	if (index + count > buffer->count) { logger_to_console("out of bounds"); DEBUG_BREAK(); return; }
	common_memcpy(
		(uint8_t *)buffer->data + index,
		value, count
	);
}

void * buffer_pop(struct Buffer * buffer, size_t size) {
	if (buffer->count < size) { DEBUG_BREAK(); return NULL; }
	buffer->count -= size;
	return (uint8_t *)buffer->data + buffer->count;
}

void * buffer_peek(struct Buffer const * buffer, size_t offset) {
	if (offset >= buffer->count) { DEBUG_BREAK(); return NULL; }
	return (uint8_t *)buffer->data + (buffer->count - offset - 1);
}

void * buffer_at(struct Buffer const * buffer, size_t index) {
	if (index >= buffer->count) { DEBUG_BREAK(); return NULL; }
	return (uint8_t *)buffer->data + index;
}
