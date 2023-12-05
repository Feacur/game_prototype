#include "framework/maths.h"
#include "framework/json_read.h"
#include "framework/assets/json.h"

#include "json_load.h"

//
#include "app_components.h"

// ----- ----- ----- ----- -----
//     API
// ----- ----- ----- ----- -----

void transform_rect_get_pivot_and_rect(
	struct Transform_Rect const * transform_rect,
	struct uvec2 parent_size,
	struct vec2 * out_pivot, struct rect * out_rect
) {
	struct vec2 const offset_min = {
		.x = transform_rect->offset.x - transform_rect->extents.x * transform_rect->pivot.x,
		.y = transform_rect->offset.y - transform_rect->extents.y * transform_rect->pivot.y,
	};
	struct vec2 const offset_max = {
		.x = transform_rect->offset.x + transform_rect->extents.x * (1 - transform_rect->pivot.x),
		.y = transform_rect->offset.y + transform_rect->extents.y * (1 - transform_rect->pivot.y),
	};

	struct vec2 const min = {
		transform_rect->anchor_min.x * (float)parent_size.x + offset_min.x,
		transform_rect->anchor_min.y * (float)parent_size.y + offset_min.y,
	};
	struct vec2 const max = {
		transform_rect->anchor_max.x * (float)parent_size.x + offset_max.x,
		transform_rect->anchor_max.y * (float)parent_size.y + offset_max.y,
	};

	*out_pivot = (struct vec2){
		.x = lerp(min.x, max.x, transform_rect->pivot.x),
		.y = lerp(min.y, max.y, transform_rect->pivot.y),
	};
	*out_rect = (struct rect){
		.min = {min.x - out_pivot->x, min.y - out_pivot->y},
		.max = {max.x - out_pivot->x, max.y - out_pivot->y},
	};
}

// ----- ----- ----- ----- -----
//     deserialization
// ----- ----- ----- ----- -----

void json_read_transform_3d(struct JSON const * json, struct Transform_3D * transform) {
	*transform = c_transform_3d_default;
	if (json->type == JSON_OBJECT) {
		struct vec3 euler = {0, 0, 0};
		json_read_many_flt(json_get(json, S_("euler")), 3, &euler.x);
		transform->rotation = quat_radians(euler);

		json_read_many_flt(json_get(json, S_("pos")),   3, &transform->position.x);
		json_read_many_flt(json_get(json, S_("quat")),  4, &transform->rotation.x);
		json_read_many_flt(json_get(json, S_("scale")), 3, &transform->scale.x);
	}
}

void json_read_transform_2d(struct JSON const * json, struct Transform_2D * transform) {
	*transform = c_transform_2d_default;
	if (json->type == JSON_OBJECT) {
		float euler = 0;
		json_read_many_flt(json_get(json, S_("euler")), 1, &euler);
		transform->rotation = comp_radians(euler);

		json_read_many_flt(json_get(json, S_("pos")),   2, &transform->position.x);
		json_read_many_flt(json_get(json, S_("comp")), 2, &transform->rotation.x);
		json_read_many_flt(json_get(json, S_("scale")), 2, &transform->scale.x);
	}
}

void json_read_transform_rect(struct JSON const * json, struct Transform_Rect * transform) {
	*transform = c_transform_rect_default;
	if (json->type == JSON_OBJECT) {
		json_read_many_flt(json_get(json, S_("anchor_min")), 2, &transform->anchor_min.x);
		json_read_many_flt(json_get(json, S_("anchor_max")), 2, &transform->anchor_max.x);
		json_read_many_flt(json_get(json, S_("offset")),     2, &transform->offset.x);
		json_read_many_flt(json_get(json, S_("extents")),    2, &transform->extents.x);
		json_read_many_flt(json_get(json, S_("pivot")),      2, &transform->pivot.x);
	}
}

// ----- ----- ----- ----- -----
//     constants
// ----- ----- ----- ----- -----

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified

struct Transform_3D const c_transform_3d_default = {
	.rotation = {0, 0, 0, 1},
	.scale = {1, 1, 1},
};

struct Transform_2D const c_transform_2d_default = {
	.rotation = {1, 0},
	.scale = {1, 1},
};

struct Transform_Rect const c_transform_rect_default = {0};
