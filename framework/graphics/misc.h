#if !defined(FRAMEWORK_GRAPHICS_GPU_MISC)
#define FRAMEWORK_GRAPHICS_GPU_MISC

// interface from `graphics.c` to anywhere
// - general purpose

#include "framework/maths_types.h"

void graphics_update(void);

struct mat4 graphics_projection_mat4(
	struct vec2 scale_xy, struct vec2 offset_xy,
	float view_near, float view_far, float ortho
);

#endif
