#include "framework/maths.h"
#include "framework/memory.h"
#include "framework/formatter.h"
#include "internal/helpers.h"

//
#include "buffer.h"

struct Buffer buffer_init(void) {
	return (struct Buffer){0};
}

void buffer_free(struct Buffer * buffer) {
	if (buffer->allocate == NULL) { return; }
	buffer->allocate(buffer->data, 0);
	common_memset(buffer, 0, sizeof(*buffer));
}

void buffer_clear(struct Buffer * buffer, bool deallocate) {
	if (buffer->allocate == NULL) { return; }
	if (deallocate) {
		buffer->allocate(buffer->data, 0); buffer->data = NULL;
		buffer->capacity = 0;
	}
	buffer->size = 0;
}

void buffer_resize(struct Buffer * buffer, size_t capacity) {
	if (buffer->capacity == capacity) { return; }
	if (buffer->allocate == NULL) {
		buffer->allocate = memory_reallocate;
	}
	void * data = buffer->allocate(buffer->data, capacity);
	if (data == NULL) { return; }
	buffer->capacity = capacity;
	buffer->size = min_size(buffer->size, capacity);
	buffer->data = data;
}

void buffer_ensure(struct Buffer * buffer, size_t capacity) {
	if (buffer->capacity >= capacity) { return; }
	capacity = growth_adjust_buffer(buffer->capacity, capacity);
	buffer_resize(buffer, capacity);
}

void buffer_push_many(struct Buffer * buffer, size_t size, void const * value) {
	buffer_ensure(buffer, buffer->size + size);
	uint8_t * end = (uint8_t *)buffer->data + buffer->size;
	common_memcpy(end, value, size);
	buffer->size += size;
}

void buffer_set_many(struct Buffer * buffer, size_t offset, size_t size, void const * value) {
	if (offset + size > buffer->size) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	uint8_t * at = (uint8_t *)buffer->data + offset;
	common_memcpy(at, value, size);
}

void buffer_insert_many(struct Buffer * buffer, size_t offset, size_t size, void const * value) {
	if (offset > buffer->size) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	buffer_ensure(buffer, buffer->size + size);
	//
	uint8_t * at  = (uint8_t *)buffer->data + offset;
	uint8_t * end = (uint8_t *)buffer->data + buffer->size;
	for (uint8_t * it = end; it > at; it -= 1) {
		common_memcpy(it, it - size, 1);
	}
	common_memcpy(at, value, size);
	buffer->size += size;
}

void * buffer_pop(struct Buffer * buffer, size_t size) {
	if (buffer->size < size) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	buffer->size -= size;
	size_t const offset = buffer->size;
	return (uint8_t *)buffer->data + offset;
}

void * buffer_peek(struct Buffer const * buffer, size_t depth) {
	if (depth >= buffer->size) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	size_t const offset = buffer->size - depth - 1;
	return (uint8_t *)buffer->data + offset;
}

void * buffer_at(struct Buffer const * buffer, size_t offset) {
	if (offset >= buffer->size) {
		ERR("out of bounds");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return NULL;
	}
	return (uint8_t *)buffer->data + offset;
}
