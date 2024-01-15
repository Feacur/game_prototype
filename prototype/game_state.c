#include "framework/formatter.h"
#include "framework/json_read.h"

#include "framework/systems/strings.h"
#include "framework/systems/assets.h"

#include "framework/assets/json.h"

#include "application/json_load.h"
#include "application/asset_types.h"

#include "proto_components.h"
#include "object_camera.h"
#include "object_entity.h"


//
#include "game_state.h"

// ----- ----- ----- ----- -----
//     Cameras part
// ----- ----- ----- ----- -----

static enum Camera_Mode json_read_camera_mode(struct JSON const * json) {
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

static void json_read_camera(struct JSON const * json, struct Camera * camera) {
	json_read_transform_3d(json_get(json, S_("transform")), &camera->transform);

	camera->params = (struct Camera_Params){
		.mode  = json_read_camera_mode(json_get(json, S_("mode"))),
		.ncp   = (float)json_get_number(json, S_("ncp")),
		.fcp   = (float)json_get_number(json, S_("fcp")),
		.ortho = (float)json_get_number(json, S_("ortho")),
	};
	camera->clear = (struct Camera_Clear){
		.flags  = json_read_texture_flags(json_get(json, S_("clear_mask"))),
	};
	json_read_many_flt(json_get(json, S_("clear_color")), 4, &camera->clear.color.x);

	struct CString const target = json_get_string(json, S_("target"));
	if (target.data != NULL) {
		camera->ah_target = system_assets_load(target);
	}
	else { camera->ah_target = (struct Handle){0}; }
}

static void json_read_cameras(struct JSON const * json) {
	if (json->type != JSON_ARRAY) {
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	uint32_t const cameras_count = json_count(json);
	for (uint32_t i = 0; i < cameras_count; i++) {
		struct JSON const * camera_json = json_at(json, i);

		struct Camera camera = camera_init();
		json_read_camera(camera_json, &camera);

		array_push_many(&gs_game.cameras, 1, &camera);
	}
}

// ----- ----- ----- ----- -----
//     Entities part
// ----- ----- ----- ----- -----

static enum Entity_Type json_read_entity_type(struct JSON const * json) {
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
	ERR("unknown entity type");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return ENTITY_TYPE_NONE;
}

static enum Entity_Quad_Mode json_read_entity_quad_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("fit"))) {
			return ENTITY_QUAD_MODE_FIT;
		}
		if (cstring_equals(value, S_("size"))) {
			return ENTITY_QUAD_MODE_SIZE;
		}
	}
	ERR("unknown quad mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return ENTITY_QUAD_MODE_NONE;
}

static enum Entity_Rotation_Mode json_read_entity_rotation_mode(struct JSON const * json) {
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

static void json_read_entity(struct JSON const * json, struct Entity * entity) {
	json_read_transform_3d(json_get(json, S_("transform")), &entity->transform);
	json_read_transform_rect(json_get(json, S_("rect")), &entity->rect);
	entity->rotation_mode = json_read_entity_rotation_mode(json_get(json, S_("rotation_mode")));

	entity->camera = (uint32_t)json_get_number(json, S_("camera_uid")) - 1;

	struct CString const material_path = json_get_string(json, S_("material"));
	entity->ah_material = system_assets_load(material_path);

	entity->type = json_read_entity_type(json_get(json, S_("type")));
	switch (entity->type) {
		case ENTITY_TYPE_NONE: break;

		case ENTITY_TYPE_MESH: {
			struct CString const model_path = json_get_string(json, S_("model"));
			entity->as.mesh = (struct Entity_Mesh){
				.ah_mesh = system_assets_load(model_path),
			};
		} break;

		case ENTITY_TYPE_QUAD_2D: {
			struct CString const uniform = json_get_string(json, S_("uniform"));
			entity->as.quad = (struct Entity_Quad){
				.sh_uniform = system_strings_add(uniform),
				.mode = json_read_entity_quad_mode(json_get(json, S_("mode"))),
			};
			json_read_many_flt(json_get(json, S_("view")), 4, &entity->as.quad.view.min.x);
		} break;

		case ENTITY_TYPE_TEXT_2D: {
			struct CString const font_path = json_get_string(json, S_("font"));
			struct CString const text_path = json_get_string(json, S_("text"));
			entity->as.text = (struct Entity_Text){
				.ah_font = system_assets_load(font_path),
				.ah_text = system_assets_load(text_path),
				.size = (float)json_get_number(json, S_("size")),
			};
			json_read_many_flt(json_get(json, S_("alignment")), 2, &entity->as.text.alignment.x);
		} break;
	}
}

static void json_read_entities(struct JSON const * json) {
	if (json->type != JSON_ARRAY) {
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	uint32_t const entities_count = json_count(json);
	for (uint32_t i = 0; i < entities_count; i++) {
		struct JSON const * entity_json = json_at(json, i);

		struct Entity entity = entity_init();
		json_read_entity(entity_json, &entity);

		array_push_many(&gs_game.entities, 1, &entity);
	}
}

//

struct Game_State gs_game;

void game_init(void) {
	gs_game = (struct Game_State){
		.cameras = array_init(sizeof(struct Camera)),
		.entities = array_init(sizeof(struct Entity)),
	};
}

void game_free(void) {
	FOR_ARRAY(&gs_game.cameras, it) {
		camera_free(it.value);
	}

	FOR_ARRAY(&gs_game.entities, it) {
		entity_free(it.value);
	}

	array_free(&gs_game.cameras);
	array_free(&gs_game.entities);

	zero_out(CBM_(gs_game));
}

JSON_PROCESSOR(game_fill_scene) {
	array_clear(&gs_game.cameras,  false);
	array_clear(&gs_game.entities, false);

	if (json->type == JSON_ERROR) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (data != &gs_game)         { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	json_read_cameras(json_get(json, S_("cameras")));
	json_read_entities(json_get(json, S_("entities")));
}
