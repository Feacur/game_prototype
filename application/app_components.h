#if !defined(APPLICATION_COMPONENTS)
#define APPLICATION_COMPONENTS

#include "framework/maths_types.h"

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

// ----- ----- ----- ----- -----
//     API
// ----- ----- ----- ----- -----

void transform_rect_get_pivot_and_rect(
	struct Transform_Rect const * transform_rect,
	struct uvec2 parent_size,
	struct vec2 * out_pivot, struct rect * out_rect
);

// ----- ----- ----- ----- -----
//     deserialization
// ----- ----- ----- ----- -----

void json_read_transform_3d(struct JSON const * json, struct Transform_3D * transform);
void json_read_transform_2d(struct JSON const * json, struct Transform_2D * transform);
void json_read_transform_rect(struct JSON const * json, struct Transform_Rect * transform);

// ----- ----- ----- ----- -----
//     constants
// ----- ----- ----- ----- -----

extern struct Transform_3D const c_transform_3d_default;
extern struct Transform_2D const c_transform_2d_default;
extern struct Transform_Rect const c_transform_rect_default;

#endif
