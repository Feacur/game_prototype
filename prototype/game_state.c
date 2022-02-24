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

#include "batcher_2d.h"
#include "components.h"
#include "object_camera.h"
#include "object_entity.h"

//
#include "game_state.h"

struct Game_State gs_game;

void game_init(void) {
	gs_game = (struct Game_State){
		.batcher = batcher_2d_init(),
	};
	buffer_init(&gs_game.buffer);
	array_any_init(&gs_game.gpu_commands, sizeof(struct GPU_Command));
	array_any_init(&gs_game.cameras, sizeof(struct Camera));
	array_any_init(&gs_game.entities, sizeof(struct Entity));

	asset_system_init(&gs_game.assets);
	asset_types_init(&gs_game.assets);
}

void game_free(void) {
	batcher_2d_free(gs_game.batcher);

	asset_system_free(&gs_game.assets);
	asset_types_free(&gs_game.assets);

	buffer_free(&gs_game.buffer);
	array_any_free(&gs_game.gpu_commands);
	array_any_free(&gs_game.cameras);
	array_any_free(&gs_game.entities);

	common_memset(&gs_game, 0, sizeof(gs_game));
}

static void state_read_json_cameras(struct JSON const * json);
static void state_read_json_entities(struct JSON const * json);

void game_read_json(struct JSON const * json) {
	state_read_json_cameras(json_get(json, S_("cameras")));
	state_read_json_entities(json_get(json, S_("entities")));
}

static uint32_t state_read_json_hex(struct JSON const * json) {
	struct CString const value = json_as_string(json, S_EMPTY);
	if (value.length > 2 && value.data[0] == '0' && value.data[1] == 'x')
	{
		return parse_hex_u32(value.data + 2);
	}
	return 0;
}

// ----- ----- ----- ----- -----
//     Transform part
// ----- ----- ----- ----- -----

static void state_read_json_transform_3d(struct JSON const * json, struct Transform_3D * transform) {
	*transform = transform_3d_default;
	if (json->type == JSON_OBJECT) {
		struct vec3 euler = {0, 0, 0};
		state_read_json_float_n(json_get(json, S_("euler")), 3, &euler.x);
		transform->rotation = quat_set_radians(euler);

		state_read_json_float_n(json_get(json, S_("pos")),   3, &transform->position.x);
		state_read_json_float_n(json_get(json, S_("quat")),  4, &transform->rotation.x);
		state_read_json_float_n(json_get(json, S_("scale")), 3, &transform->scale.x);
	}
}

static void state_read_json_transform_rect(struct JSON const * json, struct Transform_Rect * transform) {
	*transform = transform_rect_default;
	if (json->type == JSON_OBJECT) {
		state_read_json_float_n(json_get(json, S_("min_rel")), 2, &transform->min_relative.x);
		state_read_json_float_n(json_get(json, S_("min_abs")), 2, &transform->min_absolute.x);
		state_read_json_float_n(json_get(json, S_("max_rel")), 2, &transform->max_relative.x);
		state_read_json_float_n(json_get(json, S_("max_abs")), 2, &transform->max_absolute.x);
		state_read_json_float_n(json_get(json, S_("pivot")),   2, &transform->pivot.x);
	}
}

// ----- ----- ----- ----- -----
//     Cameras part
// ----- ----- ----- ----- -----

static enum Camera_Mode state_read_json_camera_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		uint32_t const id = json_as_id(json);
		if (id == json_find_id(json, S_("screen"))) {
			return CAMERA_MODE_SCREEN;
		}
		if (id == json_find_id(json, S_("aspect_x"))) {
			return CAMERA_MODE_ASPECT_X;
		}
		if (id == json_find_id(json, S_("aspect_y"))) {
			return CAMERA_MODE_ASPECT_Y;
		}
	}
	return CAMERA_MODE_NONE;
}

static void state_read_json_camera(struct JSON const * json, struct Camera * camera) {
	state_read_json_transform_3d(json_get(json, S_("transform")), &camera->transform);

	camera->params = (struct Camera_Params){
		.mode  = state_read_json_camera_mode(json_get(json, S_("mode"))),
		.ncp   = json_get_number(json, S_("ncp"),   0),
		.fcp   = json_get_number(json, S_("fcp"),   0),
		.ortho = json_get_number(json, S_("ortho"), 0),
	};
	camera->clear = (struct Camera_Clear){
		.mask = state_read_json_texture_type(json_get(json, S_("clear_mask"))),
		.rgba = state_read_json_hex(json_get(json, S_("clear_rgba"))),
	};

	struct CString const target = json_get_string(json, S_("target"), S_NULL);
	if (target.data != NULL) {
		struct Asset_Target const * asset = asset_system_aquire_instance(&gs_game.assets, target);
		camera->gpu_target_ref = (asset != NULL) ? asset->gpu_ref : ref_empty;
	}
	else { camera->gpu_target_ref = ref_empty; }
}

static void state_read_json_cameras(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const cameras_count = json_count(json);
	for (uint32_t i = 0; i < cameras_count; i++) {
		struct JSON const * camera_json = json_at(json, i);

		struct Camera camera;
		state_read_json_camera(camera_json, &camera);

		array_any_push(&gs_game.cameras, &camera);
	}
}

// ----- ----- ----- ----- -----
//     Entities part
// ----- ----- ----- ----- -----

static enum Entity_Type state_read_json_entity_type(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		uint32_t const id = json_as_id(json);
		if (id == json_find_id(json, S_("mesh"))) {
			return ENTITY_TYPE_MESH;
		}
		if (id == json_find_id(json, S_("quad_2d"))) {
			return ENTITY_TYPE_QUAD_2D;
		}
		if (id == json_find_id(json, S_("text_2d"))) {
			return ENTITY_TYPE_TEXT_2D;
		}
	}
	return ENTITY_TYPE_NONE;
}

static void state_read_json_entity(struct JSON const * json, struct Entity * entity) {
	state_read_json_transform_3d(json_get(json, S_("transform")), &entity->transform);
	state_read_json_transform_rect(json_get(json, S_("rect")), &entity->rect);

	entity->camera   = (uint32_t)json_get_number(json, S_("camera_uid"), 0) - 1;

	struct CString const material_path = json_get_string(json, S_("material"), S_NULL);
	entity->material = asset_system_aquire(&gs_game.assets, material_path);

	entity->type = state_read_json_entity_type(json_get(json, S_("type")));
	switch (entity->type) {
		case ENTITY_TYPE_NONE: break;

		case ENTITY_TYPE_MESH: {
			struct CString const model_path = json_get_string(json, S_("model"), S_NULL);
			entity->as.mesh = (struct Entity_Mesh){
				.mesh = asset_system_aquire(&gs_game.assets, model_path),
			};
		} break;

		case ENTITY_TYPE_QUAD_2D: {
			struct CString const uniform = json_get_string(json, S_("uniform"), S_NULL);
			entity->as.quad = (struct Entity_Quad){
				.texture_uniform = graphics_add_uniform_id(uniform),
				.fit = json_get_boolean(json, S_("fit"), false),
			};
		} break;

		case ENTITY_TYPE_TEXT_2D: {
			struct CString const font_path = json_get_string(json, S_("font"), S_NULL);
			struct CString const message_path = json_get_string(json, S_("message"), S_NULL);
			entity->as.text = (struct Entity_Text){
				.font = asset_system_aquire(&gs_game.assets, font_path),
				.message = asset_system_aquire(&gs_game.assets, message_path),
			};
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

		array_any_push(&gs_game.entities, &entity);
	}
}
