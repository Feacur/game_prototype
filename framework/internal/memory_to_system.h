#if !defined(FRAMEWORK_INTERNAL_MEMORY_TO_SYSTEM)
#define FRAMEWORK_INTERNAL_MEMORY_TO_SYSTEM

// @purpose: interface from `memory.c` to `system.c`
// - framework initialization

#include "framework/common.h"

bool memory_to_system_init(void);
void memory_to_system_free(void);

#endif
