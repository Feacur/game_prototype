#if !defined(FRAMEWORK_PLATFORM_MEMORY)
#define FRAMEWORK_PLATFORM_MEMORY

#include "framework/common.h"

struct Memory_Header {
	size_t checksum, size;
};

ALLOCATOR(generic_reallocate);
ALLOCATOR(debug_reallocate);

#if defined(GAME_TARGET_RELEASE)
	#define DEFAULT_REALLOCATOR generic_reallocate
#else
	#define DEFAULT_REALLOCATOR debug_reallocate
#endif

#define REALLOCATE(pointer, size) DEFAULT_REALLOCATOR(pointer, size)

void memory_debug_report(void);
void memory_debug_clear(void);


#define FREE(pointer) REALLOCATE(pointer, 0)
#define ALLOCATE(type) REALLOCATE((type *)NULL, sizeof(type))
#define ALLOCATE_ARRAY(type, count) REALLOCATE((type *)NULL, sizeof(type) * (size_t)(count))

#endif
