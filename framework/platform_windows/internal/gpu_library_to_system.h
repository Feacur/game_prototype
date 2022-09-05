#if !defined(FRAMEWORK_PLATFORM_WINDOWS_GPU_LIBRARY_TO_SYSTEM)
#define FRAMEWORK_PLATFORM_WINDOWS_GPU_LIBRARY_TO_SYSTEM

// @purpose: interface from `gpu_library_*.c` to `system.c`
// - graphics library initialization

#include "framework/common.h"

bool gpu_library_to_system_init(void);
void gpu_library_to_system_free(void);

#endif
