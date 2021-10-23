#if !defined(GAME_CONTAINERS_ARRAY_BYTE)
#define GAME_CONTAINERS_ARRAY_BYTE

#include "framework/common.h"

struct Buffer {
	uint64_t capacity, count;
	uint8_t * data;
};

void buffer_init(struct Buffer * buffer);
void buffer_free(struct Buffer * buffer);

void buffer_clear(struct Buffer * buffer);
void buffer_resize(struct Buffer * buffer, uint64_t target_capacity);

void buffer_push(struct Buffer * buffer, uint8_t value);
void buffer_push_many(struct Buffer * buffer, uint64_t count, uint8_t const * value);

void buffer_align(struct Buffer * buffer);

void buffer_set_many(struct Buffer * buffer, uint64_t index, uint64_t count, uint8_t const * value);

uint8_t buffer_pop(struct Buffer * buffer);
uint8_t buffer_peek(struct Buffer const * buffer, uint64_t offset);
uint8_t buffer_at(struct Buffer const * buffer, uint64_t index);

#endif
