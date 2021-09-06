#if !defined(GAME_FRAMEWORK_MEMORY)
#define GAME_FRAMEWORK_MEMORY

#include "common.h"

void * memory_reallocate(void const * owner, struct CString source, void * pointer, size_t size);

#define MEMORY_ALLOCATE(owner, type) memory_reallocate(owner, S_FILE_AND_LINE, (type *)NULL, sizeof(type))
#define MEMORY_FREE(owner, pointer) memory_reallocate(owner, S_FILE_AND_LINE, pointer, 0)

#define MEMORY_ALLOCATE_ARRAY(owner, type, count) memory_reallocate(owner, S_FILE_AND_LINE, (type *)NULL, sizeof(type) * (size_t)(count))
#define MEMORY_REALLOCATE_ARRAY(owner, pointer, count) memory_reallocate(owner, S_FILE_AND_LINE, pointer, sizeof(*pointer) * (size_t)(count))

#define MEMORY_ALLOCATE_SIZE(owner, size) memory_reallocate(owner, S_FILE_AND_LINE, NULL, (size_t)(size))
#define MEMORY_REALLOCATE_SIZE(owner, pointer, size) memory_reallocate(owner, S_FILE_AND_LINE, pointer, (size_t)(size))

#endif
