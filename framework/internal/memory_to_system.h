#if !defined(FRAMEWORK_INTERNAL_MEMORY_TO_SYSTEM)
#define FRAMEWORK_INTERNAL_MEMORY_TO_SYSTEM

// interface from `memory.c` to `system.c`
// - framework initialization

#include "framework/common.h"

uint32_t memory_to_system_report(void);

#endif
