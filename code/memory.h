#if !defined(GAME_MEMORY)
#define GAME_MEMORY

#include "common.h"

// void * memory_allocate(size_t size);
void * memory_reallocate(void * pointer, size_t size);
// void memory_free(void * pointer);

#define MEMORY_ALLOCATE(type) memory_reallocate(NULL, sizeof(type))
#define MEMORY_REALLOCATE(pointer) memory_reallocate(pointer, sizeof(*pointer))
#define MEMORY_FREE(pointer) memory_reallocate(pointer, 0)

#define MEMORY_ALLOCATE_ARRAY(type, size) memory_reallocate(NULL, sizeof(type) * (size_t)(size))

#endif
