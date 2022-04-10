#if !defined(GAME_CONTAINERS_ARRAY_BYTE)
#define GAME_CONTAINERS_ARRAY_BYTE

#include "framework/common.h"

struct Buffer {
	Allocator * allocator;
	size_t capacity, count;
	void * data;
};

struct Buffer buffer_init(Allocator * allocator);
void buffer_free(struct Buffer * buffer);

void buffer_clear(struct Buffer * buffer);
void buffer_resize(struct Buffer * buffer, size_t target_capacity);
void buffer_ensure(struct Buffer * buffer, size_t target_capacity);

void buffer_push_many(struct Buffer * buffer, size_t count, void const * value);

void buffer_align(struct Buffer * buffer);

void buffer_set_many(struct Buffer * buffer, size_t index, size_t count, uint8_t const * value);

void * buffer_pop(struct Buffer * buffer, size_t size);
void * buffer_peek(struct Buffer const * buffer, size_t offset);
void * buffer_at(struct Buffer const * buffer, size_t index);

#endif
