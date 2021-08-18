#if !defined(GAME_PROTOTYPE_COMPONENTS)
#define GAME_PROTOTYPE_COMPONENTS

#include "framework/vector_types.h"

struct Transform_3D {
	struct vec3 scale;
	struct vec4 rotation;
	struct vec3 position;
};

struct Transform_2D {
	struct vec2 scale;
	struct vec2 rotation;
	struct vec2 position;
};

struct Transform_Rect {
	struct vec2 min_relative, min_absolute;
	struct vec2 max_relative, max_absolute;
	struct vec2 pivot;
};

void transform_rect_calculate(
	struct Transform_Rect const * transform, uint32_t camera_size_x, uint32_t camera_size_y,
	struct vec2 * min, struct vec2 * max, struct vec2 * pivot
);

#endif
