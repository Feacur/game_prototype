#if !defined(FRAMEWORK_SYSTEMS_ARENA_SYSTEM)
#define FRAMEWORK_SYSTEMS_ARENA_SYSTEM

#include "framework/common.h"

void arena_system_clear(bool deallocate);
void arena_system_ensure(size_t size);

ALLOCATOR(arena_reallocate);


#define ARENA_FREE(pointer) arena_reallocate(pointer, 0)
#define ARENA_ALLOCATE(type) arena_reallocate((type *)NULL, sizeof(type))
#define ARENA_ALLOCATE_ARRAY(type, count) arena_reallocate((type *)NULL, sizeof(type) * (size_t)(count))

#endif
