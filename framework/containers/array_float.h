#if !defined(GAME_ARRAY_FLOAT)
#define GAME_ARRAY_FLOAT

#include "framework/common.h"

struct Array_Float {
	uint32_t capacity, count;
	float * data;
};

void array_float_init(struct Array_Float * array);
void array_float_free(struct Array_Float * array);

void array_float_resize(struct Array_Float * array, uint32_t size);
void array_float_write(struct Array_Float * array, float value);
void array_float_write_many(struct Array_Float * array, uint32_t count, float const * value);

#endif
