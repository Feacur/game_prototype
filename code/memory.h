#if !defined(GAME_MEMORY)
#define GAME_MEMORY

#include "common.h"

// void * memory_allocate(size_t size);
void * memory_reallocate(void * pointer, size_t size);
// void memory_free(void * pointer);

#define MEMORY_ALLOCATE(type) memory_reallocate(NULL, sizeof(type))
#define MEMORY_FREE(pointer) memory_reallocate(pointer, 0)

#define MEMORY_ALLOCATE_ARRAY(type, count) memory_reallocate(NULL, sizeof(type) * (size_t)(count))
#define MEMORY_REALLOCATE_ARRAY(pointer, count) memory_reallocate(pointer, sizeof(*pointer) * (size_t)(count))

#endif
