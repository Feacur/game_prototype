#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/parsing.h"

#include "framework/containers/ref.h"
#include "framework/containers/strings.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_command.h"

#include "application/asset_types.h"
#include "application/asset_registry.h"
#include "application/asset_parser.h"
#include "application/components.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "components.h"
#include "object_camera.h"
#include "object_entity.h"

//
#include "game_state.h"

// static uint32_t state_read_json_hex(struct JSON const * json) {
// 	struct CString const value = json_as_string(json, S_NULL);
// 	if (value.length > 2 && value.data[0] == '0' && value.data[1] == 'x')
// 	{
// 		return parse_hex_u32(value.data + 2);
// 	}
// 	return 0;
// }

// ----- ----- ----- ----- -----
//     Transform part
// ----- ----- ----- ----- -----

static void state_read_json_transform_3d(struct JSON const * json, struct Transform_3D * transform) {
	*transform = c_transform_3d_default;
	if (json->type == JSON_OBJECT) {
		struct vec3 euler = {0, 0, 0};
		state_read_json_flt_n(json_get(json, S_("euler")), 3, &euler.x);
		transform->rotation = quat_set_radians(euler);

		state_read_json_flt_n(json_get(json, S_("pos")),   3, &transform->position.x);
		state_read_json_flt_n(json_get(json, S_("quat")),  4, &transform->rotation.x);
		state_read_json_flt_n(json_get(json, S_("scale")), 3, &transform->scale.x);
	}
}

static void state_read_json_transform_rect(struct JSON const * json, struct Transform_Rect * transform) {
	*transform = c_transform_rect_default;
	if (json->type == JSON_OBJECT) {
		state_read_json_flt_n(json_get(json, S_("anchor_min")), 2, &transform->anchor_min.x);
		state_read_json_flt_n(json_get(json, S_("anchor_max")), 2, &transform->anchor_max.x);
		state_read_json_flt_n(json_get(json, S_("offset")),     2, &transform->offset.x);
		state_read_json_flt_n(json_get(json, S_("extents")),    2, &transform->extents.x);
		state_read_json_flt_n(json_get(json, S_("pivot")),      2, &transform->pivot.x);
	}
}

// ----- ----- ----- ----- -----
//     Cameras part
// ----- ----- ----- ----- -----

static enum Camera_Mode state_read_json_camera_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("screen"))) {
			return CAMERA_MODE_SCREEN;
		}
		if (cstring_equals(value, S_("aspect_x"))) {
			return CAMERA_MODE_ASPECT_X;
		}
		if (cstring_equals(value, S_("aspect_y"))) {
			return CAMERA_MODE_ASPECT_Y;
		}
	}
	return CAMERA_MODE_NONE;
}

static void state_read_json_camera(struct JSON const * json, struct Camera * camera) {
	state_read_json_transform_3d(json_get(json, S_("transform")), &camera->transform);

	camera->params = (struct Camera_Params){
		.mode  = state_read_json_camera_mode(json_get(json, S_("mode"))),
		.ncp   = (float)json_get_number(json, S_("ncp")),
		.fcp   = (float)json_get_number(json, S_("fcp")),
		.ortho = (float)json_get_number(json, S_("ortho")),
	};
	camera->clear = (struct Camera_Clear){
		.mask  = state_read_json_texture_type(json_get(json, S_("clear_mask"))),
		// .rgba = state_read_json_hex(json_get(json, S_("clear_rgba"))),
	};
	state_read_json_flt_n(json_get(json, S_("clear_color")), 4, &camera->clear.color.x);

	struct CString const target = json_get_string(json, S_("target"));
	if (target.data != NULL) {
		struct Asset_Target const * asset = asset_system_aquire_instance(&gs_game.assets, target);
		camera->gpu_target_ref = (asset != NULL) ? asset->gpu_ref : c_ref_empty;
	}
	else { camera->gpu_target_ref = c_ref_empty; }
}

static void state_read_json_cameras(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const cameras_count = json_count(json);
	for (uint32_t i = 0; i < cameras_count; i++) {
		struct JSON const * camera_json = json_at(json, i);

		struct Camera camera;
		state_read_json_camera(camera_json, &camera);

		array_any_push_many(&gs_game.cameras, 1, &camera);
	}
}

// ----- ----- ----- ----- -----
//     Entities part
// ----- ----- ----- ----- -----

static enum Entity_Type state_read_json_entity_type(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("mesh"))) {
			return ENTITY_TYPE_MESH;
		}
		if (cstring_equals(value, S_("quad_2d"))) {
			return ENTITY_TYPE_QUAD_2D;
		}
		if (cstring_equals(value, S_("text_2d"))) {
			return ENTITY_TYPE_TEXT_2D;
		}
	}
	return ENTITY_TYPE_NONE;
}

static enum Entity_Quad_Mode state_read_json_entity_quad_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("fit"))) {
			return ENTITY_QUAD_MODE_FIT;
		}
		if (cstring_equals(value, S_("size"))) {
			return ENTITY_QUAD_MODE_SIZE;
		}
	}
	return ENTITY_QUAD_MODE_NONE;
}

static enum Entity_Rotation_Mode state_read_json_entity_rotation_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("x"))) {
			return ENTITY_ROTATION_MODE_X;
		}
		if (cstring_equals(value, S_("y"))) {
			return ENTITY_ROTATION_MODE_Y;
		}
		if (cstring_equals(value, S_("z"))) {
			return ENTITY_ROTATION_MODE_Z;
		}
	}
	return ENTITY_ROTATION_MODE_NONE;
}

static void state_read_json_entity(struct JSON const * json, struct Entity * entity) {
	state_read_json_transform_3d(json_get(json, S_("transform")), &entity->transform);
	state_read_json_transform_rect(json_get(json, S_("rect")), &entity->rect);
	entity->rotation_mode = state_read_json_entity_rotation_mode(json_get(json, S_("rotation_mode")));

	entity->camera   = (uint32_t)json_get_number(json, S_("camera_uid")) - 1;

	struct CString const material_path = json_get_string(json, S_("material"));
	entity->material = asset_system_aquire(&gs_game.assets, material_path);

	entity->type = state_read_json_entity_type(json_get(json, S_("type")));
	switch (entity->type) {
		case ENTITY_TYPE_NONE: break;

		case ENTITY_TYPE_MESH: {
			struct CString const model_path = json_get_string(json, S_("model"));
			entity->as.mesh = (struct Entity_Mesh){
				.mesh = asset_system_aquire(&gs_game.assets, model_path),
			};
		} break;

		case ENTITY_TYPE_QUAD_2D: {
			struct CString const uniform = json_get_string(json, S_("uniform"));
			entity->as.quad = (struct Entity_Quad){
				.texture_uniform = graphics_add_uniform_id(uniform),
				.mode = state_read_json_entity_quad_mode(json_get(json, S_("mode"))),
			};
		} break;

		case ENTITY_TYPE_TEXT_2D: {
			struct CString const font_path = json_get_string(json, S_("font"));
			struct CString const message_path = json_get_string(json, S_("message"));
			entity->as.text = (struct Entity_Text){
				.font = asset_system_aquire(&gs_game.assets, font_path),
				.message = asset_system_aquire(&gs_game.assets, message_path),
				.size = (float)json_get_number(json, S_("size")),
			};
			state_read_json_flt_n(json_get(json, S_("alignment")), 2, &entity->as.text.alignment.x);
		} break;
	}
}

static void state_read_json_entities(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const entities_count = json_count(json);
	for (uint32_t i = 0; i < entities_count; i++) {
		struct JSON const * entity_json = json_at(json, i);

		struct Entity entity;
		state_read_json_entity(entity_json, &entity);

		array_any_push_many(&gs_game.entities, 1, &entity);
	}
}

//

struct Game_State gs_game;

void game_init(void) {
	gs_game = (struct Game_State){
		.cameras = array_any_init(sizeof(struct Camera)),
		.entities = array_any_init(sizeof(struct Entity)),
		.assets = asset_system_init(),
	};
	asset_types_init(&gs_game.assets);
}

void game_free(void) {
	asset_types_free(&gs_game.assets);
	asset_system_free(&gs_game.assets);

	array_any_free(&gs_game.cameras);
	array_any_free(&gs_game.entities);

	common_memset(&gs_game, 0, sizeof(gs_game));
}

void game_fill_scene(struct JSON const * json, void * data) {
	if (json->type == JSON_ERROR) { DEBUG_BREAK(); return; }
	if (data != &gs_game) { DEBUG_BREAK(); return; }
	state_read_json_cameras(json_get(json, S_("cameras")));
	state_read_json_entities(json_get(json, S_("entities")));
}
