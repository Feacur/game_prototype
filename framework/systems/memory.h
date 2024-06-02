#if !defined(FRAMEWORK_SYSTEMS_MEMORY)
#define FRAMEWORK_SYSTEMS_MEMORY

#include "framework/common.h"

struct Memory_Header {
	size_t checksum, size;
};

// ----- ----- ----- ----- -----
//     Generic part
// ----- ----- ----- ----- -----

ALLOCATOR(realloc_generic);

#define GENERIC_FREE(pointer) generic_arena(pointer, 0)
#define GENERIC_ALLOCATE(type) generic_arena((type *)NULL, sizeof(type))
#define GENERIC_ALLOCATE_ARRAY(type, count) generic_arena((type *)NULL, sizeof(type) * (size_t)(count))

// ----- ----- ----- ----- -----
//     Debug part
// ----- ----- ----- ----- -----

ALLOCATOR(realloc_debug);

#define DEBUG_FREE(pointer) debug_arena(pointer, 0)
#define DEBUG_ALLOCATE(type) debug_arena((type *)NULL, sizeof(type))
#define DEBUG_ALLOCATE_ARRAY(type, count) debug_arena((type *)NULL, sizeof(type) * (size_t)(count))

void system_memory_debug_init(void);
void system_memory_debug_free(void);

// ----- ----- ----- ----- -----
//     Arena part
// ----- ----- ----- ----- -----

ALLOCATOR(realloc_arena);

#define ARENA_FREE(pointer) realloc_arena(pointer, 0)
#define ARENA_ALLOCATE(type) realloc_arena((type *)NULL, sizeof(type))
#define ARENA_ALLOCATE_ARRAY(type, count) realloc_arena((type *)NULL, sizeof(type) * (size_t)(count))

void system_memory_arena_init(void);
void system_memory_arena_free(void);

void system_memory_arena_clear(void);
void system_memory_arena_ensure(size_t size);

// ----- ----- ----- ----- -----
//     Default part
// ----- ----- ----- ----- -----

#if defined(GAME_TARGET_RELEASE)
	#define DEFAULT_REALLOCATOR realloc_generic
#else
	#define DEFAULT_REALLOCATOR realloc_debug
#endif

#define REALLOCATE(pointer, size) DEFAULT_REALLOCATOR(pointer, size)

#define FREE(pointer) REALLOCATE(pointer, 0)
#define ALLOCATE(type) REALLOCATE((type *)NULL, sizeof(type))
#define ALLOCATE_ARRAY(type, count) REALLOCATE((type *)NULL, sizeof(type) * (size_t)(count))

#endif
