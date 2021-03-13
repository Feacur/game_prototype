#if !defined(GAME_ARRAY_BYTE)
#define GAME_ARRAY_BYTE

#include "framework/common.h"

struct Array_Byte {
	uint32_t capacity, count;
	uint8_t * data;
};

void array_byte_init(struct Array_Byte * array);
void array_byte_free(struct Array_Byte * array);

void array_byte_resize(struct Array_Byte * array, uint32_t size);
void array_byte_write(struct Array_Byte * array, uint8_t value);
void array_byte_write_many(struct Array_Byte * array, uint32_t count, uint8_t const * value);
void array_byte_write_many_zeroes(struct Array_Byte * array, uint32_t count);

#endif
