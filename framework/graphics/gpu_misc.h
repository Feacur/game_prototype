#if !defined(GAME_GRAPHICS_GPU_MISC)
#define GAME_GRAPHICS_GPU_MISC

// interface from `gpu_*/graphics.c` to anywhere
// - general purpose

#include "framework/common.h"

uint32_t graphics_add_uniform(char const * name);
uint32_t graphics_find_uniform(char const * name);

#endif
