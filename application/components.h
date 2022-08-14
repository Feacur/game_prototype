#if !defined(GAME_APPLICATION_COMPONENTS)
#define GAME_APPLICATION_COMPONENTS

#include "framework/vector_types.h"

struct Transform_3D {
	struct vec3 position;
	struct vec4 rotation;
	struct vec3 scale;
};

struct Transform_2D {
	struct vec2 position;
	struct vec2 rotation;
	struct vec2 scale;
};

struct Transform_Rect {
	struct vec2 anchor_min, anchor_max;
	struct vec2 offset, extents;
	struct vec2 pivot;
};

//

extern struct Transform_3D const c_transform_3d_default;
extern struct Transform_2D const c_transform_2d_default;
extern struct Transform_Rect const c_transform_rect_default;

#endif
