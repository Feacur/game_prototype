#if !defined(FRAMEWORK_CONTAINERS_BUFFER)
#define FRAMEWORK_CONTAINERS_BUFFER

#include "framework/common.h"

struct Buffer {
	Allocator * allocate;
	size_t capacity, size;
	void * data;
};

struct Buffer buffer_init(void);
void buffer_free(struct Buffer * buffer);

void buffer_clear(struct Buffer * buffer);
void buffer_resize(struct Buffer * buffer, size_t capacity);
void buffer_ensure(struct Buffer * buffer, size_t capacity);

void buffer_push_many(struct Buffer * buffer, size_t size, void const * value);
void buffer_set_many(struct Buffer * buffer, size_t offset, size_t size, void const * value);
void buffer_insert_many(struct Buffer * buffer, size_t offset, size_t size, void const * value);

void * buffer_pop(struct Buffer * buffer, size_t size);
void * buffer_peek(struct Buffer const * buffer, size_t offset);
void * buffer_at(struct Buffer const * buffer, size_t offset);

void * buffer_at_unsafe(struct Buffer const * buffer, size_t offset);

#endif
