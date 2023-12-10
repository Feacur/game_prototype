#if !defined(FRAMEWORK_SYSTEMS_BUFFER_SYSTEM)
#define FRAMEWORK_SYSTEMS_BUFFER_SYSTEM

#include "framework/common.h"

void buffer_system_init(void);
void buffer_system_free(void);

void buffer_system_reset(void);

ALLOCATOR(buffer_system_reallocate);

#define BUFFER_ALLOCATE(type) buffer_system_reallocate((type *)NULL, sizeof(type))
#define BUFFER_ALLOCATE_SIZE(size) buffer_system_reallocate(NULL, (size_t)(size))
#define BUFFER_ALLOCATE_ARRAY(type, count) buffer_system_reallocate((type *)NULL, sizeof(type) * (size_t)(count))

#define BUFFER_FREE(pointer) buffer_system_reallocate(pointer, 0)
#define BUFFER_REALLOCATE_ARRAY(pointer, count) buffer_system_reallocate(pointer, sizeof(*pointer) * (size_t)(count))


#endif
