#if !defined(GAME_PROTOTYPE_COMPONENTS)
#define GAME_PROTOTYPE_COMPONENTS

#include "framework/vector_types.h"

struct Transform_3D {
	struct vec3 scale;    // vec3_one
	struct vec4 rotation; // quat_identity
	struct vec3 position;
};

struct Transform_2D {
	struct vec2 scale;    // vec2_one
	struct vec2 rotation; // comp_identity
	struct vec2 position;
};

struct Transform_Rect {
	struct vec2 min_relative, min_absolute;
	struct vec2 max_relative, max_absolute;
	struct vec2 pivot;
};

//

extern struct Transform_3D const transform_3d_default;
extern struct Transform_2D const transform_2d_default;
extern struct Transform_Rect const transform_rect_default;

#endif
