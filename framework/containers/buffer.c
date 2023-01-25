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
	buffer->size = 0;
}

void buffer_resize(struct Buffer * buffer, size_t target_capacity) {
	buffer->capacity = target_capacity;
	if (buffer->size > target_capacity) { buffer->size = target_capacity; }
	buffer->data = default_allocator(buffer->allocator)(buffer->data, target_capacity);
}

void buffer_ensure(struct Buffer * buffer, size_t target_capacity) {
	if (buffer->capacity < target_capacity) {
		buffer_resize(buffer, target_capacity);
	}
}

static void buffer_grow_if_must(struct Buffer * buffer, size_t target_size) {
	if (buffer->capacity < target_size) {
		size_t const target_capacity = grow_capacity_value_u64(buffer->capacity, target_size - buffer->capacity);
		buffer_resize(buffer, target_capacity);
	}
}

void buffer_push_many(struct Buffer * buffer, size_t size, void const * value) {
	buffer_grow_if_must(buffer, buffer->size + size);
	if (value != NULL) {
		common_memcpy(
			(uint8_t *)buffer->data + buffer->size,
			value, size
		);
	}
	buffer->size += size;
}

void buffer_set_many(struct Buffer * buffer, size_t offset, size_t size, void const * value) {
	if (offset + size > buffer->size) { logger_to_console("out of bounds\n"); DEBUG_BREAK(); return; }
	common_memcpy(
		(uint8_t *)buffer->data + offset,
		value, size
	);
}

void * buffer_pop(struct Buffer * buffer, size_t size) {
	if (buffer->size < size) { DEBUG_BREAK(); return NULL; }
	buffer->size -= size;
	return (uint8_t *)buffer->data + buffer->size;
}

void * buffer_peek(struct Buffer const * buffer, size_t offset) {
	if (offset >= buffer->size) { DEBUG_BREAK(); return NULL; }
	return (uint8_t *)buffer->data + (buffer->size - offset - 1);
}

void * buffer_at(struct Buffer const * buffer, size_t offset) {
	if (offset >= buffer->size) { DEBUG_BREAK(); return NULL; }
	return (uint8_t *)buffer->data + offset;
}
