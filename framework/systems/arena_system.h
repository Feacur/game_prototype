#if !defined(FRAMEWORK_SYSTEMS_arena_system)
#define FRAMEWORK_SYSTEMS_arena_system

#include "framework/common.h"

void arena_system_init(void);
void arena_system_free(void);

void arena_system_reset(void);

ALLOCATOR(arena_reallocate);

#define ARENA_ALLOCATE(type) arena_reallocate((type *)NULL, sizeof(type))
#define ARENA_ALLOCATE_SIZE(size) arena_reallocate(NULL, (size_t)(size))
#define ARENA_ALLOCATE_ARRAY(type, count) arena_reallocate((type *)NULL, sizeof(type) * (size_t)(count))

#define ARENA_FREE(pointer) arena_reallocate(pointer, 0)
#define ARENA_REALLOCATE_ARRAY(pointer, count) arena_reallocate(pointer, sizeof(*pointer) * (size_t)(count))


#endif
