#include "framework/assets/json.h"
#include "framework/maths.h"

#include "json_read.h"

//
#include "components.h"

// ----- ----- ----- ----- -----
//     Transform part
// ----- ----- ----- ----- -----

void json_read_transform_3d(struct JSON const * json, struct Transform_3D * transform) {
	*transform = c_transform_3d_default;
	if (json->type == JSON_OBJECT) {
		struct vec3 euler = {0, 0, 0};
		json_read_many_flt(json_get(json, S_("euler")), 3, &euler.x);
		transform->rotation = quat_set_radians(euler);

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
		transform->rotation = comp_set_radians(euler);

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
