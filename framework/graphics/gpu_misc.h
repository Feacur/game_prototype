#if !defined(FRAMEWORK_GRAPHICS_GPU_MISC)
#define FRAMEWORK_GRAPHICS_GPU_MISC

// interface from `gpu_*/graphics.c` to anywhere
// - general purpose

#include "framework/common.h"

void graphics_update(void);

uint32_t graphics_add_uniform_id(struct CString name);
uint32_t graphics_find_uniform_id(struct CString name);
struct CString graphics_get_uniform_value(uint32_t value);

#endif
