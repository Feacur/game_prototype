#if !defined(GAME_CONTAINERS_ARRAY_BYTE)
#define GAME_CONTAINERS_ARRAY_BYTE

#include "framework/common.h"

struct Array_Byte {
	uint64_t capacity, count;
	uint8_t * data;
};

void array_byte_init(struct Array_Byte * array);
void array_byte_free(struct Array_Byte * array);

void array_byte_clear(struct Array_Byte * array);
void array_byte_resize(struct Array_Byte * array, uint64_t target_capacity);

void array_byte_push(struct Array_Byte * array, uint8_t value);
void array_byte_push_many(struct Array_Byte * array, uint64_t count, uint8_t const * value);

uint8_t array_byte_pop(struct Array_Byte * array);
uint8_t array_byte_peek(struct Array_Byte * array, uint64_t offset);
uint8_t array_byte_at(struct Array_Byte * array, uint64_t index);

#endif
