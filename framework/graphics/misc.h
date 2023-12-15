#if !defined(FRAMEWORK_GRAPHICS_GPU_MISC)
#define FRAMEWORK_GRAPHICS_GPU_MISC

// interface from `graphics.c` to anywhere
// - general purpose

#include "framework/maths_types.h"
#include "framework/graphics/gfx_types.h"

struct Buffer;

void graphics_update(void);

struct mat4 graphics_projection_mat4(
	struct vec2 scale_xy, struct vec2 offset_xy,
	float view_near, float view_far, float ortho
);

size_t graphics_offest_align(size_t offset, enum Buffer_Mode mode);
void graphics_buffer_align(struct Buffer * buffer, enum Buffer_Mode mode);

#endif
