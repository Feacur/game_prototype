//
#include "components.h"

struct Transform_3D const transform_3d_default = {
	.scale = (struct vec3){1, 1, 1},
	.rotation = (struct vec4){0, 0, 0, 1},
};

struct Transform_2D const transform_2d_default = {
	.scale = (struct vec2){1, 1},
	.rotation = (struct vec2){1, 0},
};
struct Transform_Rect const transform_rect_default = {
	.max_relative = (struct vec2){1, 1},
};
