#if !defined(GAME_ARRAY_POINTER)
#define GAME_ARRAY_POINTER

#include "framework/common.h"

struct Array_Pointer {
	uint32_t capacity, count;
	void ** data;
};

void array_pointer_init(struct Array_Pointer * array);
void array_pointer_free(struct Array_Pointer * array);

void array_pointer_resize(struct Array_Pointer * array, uint32_t size);
void array_pointer_write(struct Array_Pointer * array, void * value);
void array_pointer_write_many(struct Array_Pointer * array, uint32_t count, void ** value);
void array_pointer_write_many_zeroes(struct Array_Pointer * array, uint32_t count);

#endif
