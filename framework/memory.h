#if !defined(FRAMEWORK_MEMORY)
#define FRAMEWORK_MEMORY

#include "common.h"

struct Memory_Header {
	size_t checksum, size;
};

ALLOCATOR(memory_reallocate);
ALLOCATOR(memory_reallocate_without_tracking);

inline static Allocator * default_allocator(Allocator * allocator) {
	return (allocator != NULL) ? allocator : memory_reallocate;
}

#define MEMORY_ALLOCATE(type) memory_reallocate((type *)NULL, sizeof(type))
#define MEMORY_ALLOCATE_SIZE(size) memory_reallocate(NULL, (size_t)(size))
#define MEMORY_ALLOCATE_ARRAY(type, count) memory_reallocate((type *)NULL, sizeof(type) * (size_t)(count))

#define MEMORY_FREE(pointer) memory_reallocate(pointer, 0)
#define MEMORY_REALLOCATE_SIZE(pointer, size) memory_reallocate(pointer, (size_t)(size))
#define MEMORY_REALLOCATE_ARRAY(pointer, count) memory_reallocate(pointer, sizeof(*pointer) * (size_t)(count))

#endif
