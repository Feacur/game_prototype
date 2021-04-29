#if !defined(GAME_CONTAINERS_ARRAY_FLOAT)
#define GAME_CONTAINERS_ARRAY_FLOAT

#include "framework/common.h"

struct Array_Float {
	uint32_t capacity, count;
	float * data;
};

void array_float_init(struct Array_Float * array);
void array_float_free(struct Array_Float * array);

void array_float_clear(struct Array_Float * array);
void array_float_resize(struct Array_Float * array, uint32_t target_capacity);

void array_float_push(struct Array_Float * array, float value);
void array_float_push_many(struct Array_Float * array, uint32_t count, float const * value);

float array_float_pop(struct Array_Float * array);
float array_float_peek(struct Array_Float * array, uint32_t offset);
float array_float_at(struct Array_Float * array, uint32_t index);

#endif
