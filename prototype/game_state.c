#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/parsing.h"

#include "framework/containers/ref.h"
#include "framework/containers/strings.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_objects.h"

#include "batcher_2d.h"
#include "components.h"
#include "object_camera.h"
#include "object_entity.h"
#include "asset_types.h"

//
#include "game_state.h"

struct Game_State state;

void state_init(void) {

	// init state
	{
		state = (struct Game_State){
			.batcher = batcher_2d_init(),
		};
		asset_system_init(&state.asset_system);
		array_any_init(&state.targets, sizeof(struct Ref));
		array_any_init(&state.materials, sizeof(struct Gfx_Material));
		array_any_init(&state.cameras, sizeof(struct Camera));
		array_any_init(&state.entities, sizeof(struct Entity));
	}

	// init asset system
	{ // state is expected to be inited
		// > map types
		asset_system_map_extension(&state.asset_system, S_("shader"), S_("glsl"));
		asset_system_map_extension(&state.asset_system, S_("model"),  S_("obj"));
		asset_system_map_extension(&state.asset_system, S_("model"),  S_("fbx"));
		asset_system_map_extension(&state.asset_system, S_("image"),  S_("png"));
		asset_system_map_extension(&state.asset_system, S_("font"),   S_("ttf"));
		asset_system_map_extension(&state.asset_system, S_("font"),   S_("otf"));
		asset_system_map_extension(&state.asset_system, S_("bytes"),  S_("txt"));
		asset_system_map_extension(&state.asset_system, S_("json"),   S_("json"));

		// > register types
		asset_system_set_type(&state.asset_system, S_("shader"), (struct Asset_Callbacks){
			.init = asset_shader_init,
			.free = asset_shader_free,
		}, sizeof(struct Asset_Shader));

		asset_system_set_type(&state.asset_system, S_("model"), (struct Asset_Callbacks){
			.init = asset_model_init,
			.free = asset_model_free,
		}, sizeof(struct Asset_Model));

		asset_system_set_type(&state.asset_system, S_("image"), (struct Asset_Callbacks){
			.init = asset_image_init,
			.free = asset_image_free,
		}, sizeof(struct Asset_Image));

		asset_system_set_type(&state.asset_system, S_("font"), (struct Asset_Callbacks){
			.init = asset_font_init,
			.free = asset_font_free,
		}, sizeof(struct Asset_Font));

		asset_system_set_type(&state.asset_system, S_("bytes"), (struct Asset_Callbacks){
			.init = asset_bytes_init,
			.free = asset_bytes_free,
		}, sizeof(struct Asset_Bytes));

		asset_system_set_type(&state.asset_system, S_("json"), (struct Asset_Callbacks){
			.type_init = asset_json_type_init,
			.type_free = asset_json_type_free,
			.init = asset_json_init,
			.free = asset_json_free,
		}, sizeof(struct Asset_JSON));
	}
}

void state_free(void) {
	batcher_2d_free(state.batcher);

	asset_system_free(&state.asset_system);

	for (uint32_t i = 0; i < state.materials.count; i++) {
		gfx_material_free(array_any_at(&state.materials, i));
	}

	array_any_free(&state.targets);
	array_any_free(&state.materials);
	array_any_free(&state.cameras);
	array_any_free(&state.entities);

	common_memset(&state, 0, sizeof(state));
}

static void state_read_json_targets(struct JSON const * json);
static void state_read_json_materials(struct JSON const * json);
static void state_read_json_cameras(struct JSON const * json);
static void state_read_json_entities(struct JSON const * json);

void state_read_json(struct JSON const * json) {
	struct JSON const * targets_json = json_get(json, S_("targets"));
	state_read_json_targets(targets_json);

	struct JSON const * materials_json = json_get(json, S_("materials"));
	state_read_json_materials(materials_json);

	struct JSON const * cameras_json = json_get(json, S_("cameras"));
	state_read_json_cameras(cameras_json);

	struct JSON const * entities_json = json_get(json, S_("entities"));
	state_read_json_entities(entities_json);
}

// ----- ----- ----- ----- -----
//     Targets part
// ----- ----- ----- ----- -----

static void state_read_json_target_buffer(struct JSON const * json, struct Array_Any * parameters) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	uint32_t const type_id = json_get_id(json, S_("type"));
	if (type_id == INDEX_EMPTY) { DEBUG_BREAK(); return; }

	bool const buffer_read = json_get_boolean(json, S_("read"), false);

	if (type_id == json_find_id(json, S_("color_rgba_u8"))) {
		array_any_push(parameters, &(struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_U8,
			.channels = 4,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		});
		return;
	}

	if (type_id == json_find_id(json, S_("color_depth_r32"))) {
		array_any_push(parameters, &(struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_DEPTH,
			.data_type = DATA_TYPE_R32,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		});
		return;
	}
}

static void state_read_json_target(struct JSON const * json, struct Ref * target_ref, struct Array_Any * parameters_buffer) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	parameters_buffer->count = 0;
	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct JSON const * buffer_json = json_at(buffers_json, i);
		state_read_json_target_buffer(buffer_json, parameters_buffer);
	}

	if (parameters_buffer->count == 0) { DEBUG_BREAK(); return; }

	uint32_t const size_x = (uint32_t)json_get_number(json, S_("size_x"), 320);
	uint32_t const size_y = (uint32_t)json_get_number(json, S_("size_y"), 180);

	*target_ref = gpu_target_init(size_x, size_y, array_any_at(parameters_buffer, 0), parameters_buffer->count);
}

static void state_read_json_targets(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	// @todo: arena/stack allocator
	struct Array_Any parameters_buffer;
	array_any_init(&parameters_buffer, sizeof(struct Texture_Parameters));

	uint32_t const targets_count = json_count(json);
	for (uint32_t i = 0; i < targets_count; i++) {
		struct JSON const * target_json = json_at(json, i);

		struct Ref target_ref;
		state_read_json_target(target_json, &target_ref, &parameters_buffer);

		array_any_push(&state.targets, &target_ref);
	}

	array_any_free(&parameters_buffer);
}

// ----- ----- ----- ----- -----
//     Materials part
// ----- ----- ----- ----- -----

static struct Ref state_read_json_uniform_image(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return ref_empty; }

	struct CString const path = json_get_string(json, S_("path"), S_NULL);
	if (path.data == NULL) { DEBUG_BREAK(); return ref_empty; }

	struct Asset_Image const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return ref_empty; }

	return asset->gpu_ref;
}

static struct Ref state_read_json_uniform_target(struct JSON const * json) {
	if (json->type == JSON_NULL) { return ref_empty; }
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return ref_empty; }

	uint32_t const buffer_type_id = json_get_id(json, S_("buffer_type"));
	if (buffer_type_id == INDEX_EMPTY) { DEBUG_BREAK(); return ref_empty; }

	uint32_t const target_uid = (uint32_t)json_get_number(json, S_("uid"), 0);
	if (target_uid == 0) { DEBUG_BREAK(); return ref_empty; }

	struct Ref const * gpu_target = array_any_at(&state.targets, target_uid - 1);
	if (gpu_target == NULL) { DEBUG_BREAK(); return ref_empty; }

	uint32_t const index = (uint32_t)json_get_number(json, S_("index"), 0);

	if (buffer_type_id == json_find_id(json, S_("color"))) {
		return gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_COLOR, index);
	}

	if (buffer_type_id == json_find_id(json, S_("depth"))) {
		return gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_DEPTH, index);
	}

	if (buffer_type_id == json_find_id(json, S_("stencil"))) {
		return gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_STENCIL, index);
	}

	if (buffer_type_id == json_find_id(json, S_("dstencil"))) {
		return gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_DSTENCIL, index);
	}

	DEBUG_BREAK();
	return ref_empty;
}

static struct Ref state_read_json_uniform_font(struct JSON const * json) {
	if (json->type == JSON_NULL) { return ref_empty; }
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return ref_empty; }

	struct CString const path = json_get_string(json, S_("path"), S_NULL);
	if (path.data == NULL) { DEBUG_BREAK(); return ref_empty; }

	struct Asset_Font const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return ref_empty; }

	return asset->gpu_ref;
}

static struct Ref state_read_json_uniform_texture(struct JSON const * json) {
	if (json->type == JSON_NULL) { return ref_empty; }
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return ref_empty; }

	uint32_t const type_id = json_get_id(json, S_("type"));
	if (type_id == INDEX_EMPTY) { DEBUG_BREAK(); return ref_empty; }

	if (type_id == json_find_id(json, S_("image"))) {
		return state_read_json_uniform_image(json);
	}

	if (type_id == json_find_id(json, S_("target"))) {
		return state_read_json_uniform_target(json);
	}

	if (type_id == json_find_id(json, S_("font"))) {
		return state_read_json_uniform_font(json);
	}

	return ref_empty;
}

static void state_read_json_blend_mode(struct JSON const * json, struct Blend_Mode * blend_mode) {
	uint32_t const mode_id = json_get_id(json, S_("mode"));

	if (mode_id == json_find_id(json, S_("opaque"))) {
		*blend_mode = blend_mode_opaque;
		return;
	}

	if (mode_id == json_find_id(json, S_("transparent"))) {
		*blend_mode = blend_mode_transparent;
		return;
	}

	*blend_mode = blend_mode_opaque;
}

static void state_read_json_depth_mode(struct JSON const * json, struct Depth_Mode * depth_mode) {
	bool const depth_write = json_get_boolean(json, S_("depth"), false);

	if (depth_write) {
		*depth_mode = (struct Depth_Mode){.enabled = true, .mask = true};
		return;
	}

	*depth_mode = (struct Depth_Mode){0};
}

static void state_read_json_material(struct JSON const * json, struct Gfx_Material * material, struct Array_Byte * uniform_data_buffer) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct CString const path = json_get_string(json, S_("shader"), S_NULL);
	if (path.data == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Shader const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return; }

	struct Blend_Mode blend_mode;
	state_read_json_blend_mode(json, &blend_mode);

	struct Depth_Mode depth_mode;
	state_read_json_depth_mode(json, &depth_mode);

	gfx_material_init(
		material, asset->gpu_ref,
		&blend_mode, &depth_mode
	);

	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->gpu_program_ref, &uniforms_count, &uniforms);

	uniform_data_buffer->count = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		struct CString const uniform_name = graphics_get_uniform_value(uniforms[i].id);
		struct JSON const * uniform_json = json_get(json, uniform_name);

		if (uniform_json->type == JSON_NULL) { continue; }

		uint32_t const uniform_elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;

		uint32_t const uniform_bytes = data_type_get_size(uniforms[i].type) * uniforms[i].array_size;
		if (uniform_data_buffer->capacity < uniform_bytes) {
			array_byte_resize(uniform_data_buffer, uniform_bytes);
		}

		uint32_t const json_elements_count = max_u32(1, json_count(uniform_json));
		if (json_elements_count != uniform_elements_count) {
			logger_to_console(
				"uniform `%.*s` size mismatch: expected %u, found %u\n",
				uniform_name.length, uniform_name.data,
				uniform_elements_count, json_elements_count
			); DEBUG_BREAK();
		}

		switch (data_type_get_element_type(uniforms[i].type)) {
			default: logger_to_console("unknown data type\n"); DEBUG_BREAK(); break;

			case DATA_TYPE_UNIT: {
				if (uniform_json->type == JSON_ARRAY) {
					for (uint32_t element_index = 0; element_index < uniform_elements_count; element_index++) {
						struct Ref * data = (void *)uniform_data_buffer->data;
						data[element_index] = state_read_json_uniform_texture(json_at(uniform_json, element_index));
					}
				}
				else {
					struct Ref * data = (void *)uniform_data_buffer->data;
					data[0] = state_read_json_uniform_texture(uniform_json);
				}
				gfx_material_set_texture(material, uniforms[i].id, uniform_elements_count, (void *)uniform_data_buffer->data);
			} break;

			case DATA_TYPE_U32: {
				if (uniform_json->type == JSON_ARRAY) {
					for (uint32_t element_index = 0; element_index < uniform_elements_count; element_index++) {
						uint32_t * data = (void *)uniform_data_buffer->data;
						data[element_index] = (uint32_t)json_at_number(uniform_json, element_index, 0);
					}
				}
				else {
					uint32_t * data = (void *)uniform_data_buffer->data;
					data[0] = (uint32_t)json_as_number(uniform_json, 0);
				}
				gfx_material_set_u32(material, uniforms[i].id, uniform_elements_count, (void *)uniform_data_buffer->data);
			} break;

			case DATA_TYPE_S32: {
				if (uniform_json->type == JSON_ARRAY) {
					for (uint32_t element_index = 0; element_index < uniform_elements_count; element_index++) {
						int32_t * data = (void *)uniform_data_buffer->data;
						data[element_index] = (int32_t)json_at_number(uniform_json, element_index, 0);
					}
				}
				else {
					int32_t * data = (void *)uniform_data_buffer->data;
					data[0] = (int32_t)json_as_number(uniform_json, 0);
				}
				gfx_material_set_s32(material, uniforms[i].id, uniform_elements_count, (void *)uniform_data_buffer->data);
			} break;

			case DATA_TYPE_R32: {
				if (uniform_json->type == JSON_ARRAY) {
					for (uint32_t element_index = 0; element_index < uniform_elements_count; element_index++) {
						float * data = (void *)uniform_data_buffer->data;
						data[element_index] = json_at_number(uniform_json, element_index, 0);
					}
				}
				else {
					float * data = (void *)uniform_data_buffer->data;
					data[0] = json_as_number(uniform_json, 0);
				}
				gfx_material_set_float(material, uniforms[i].id, uniform_elements_count, (void *)uniform_data_buffer->data);
			} break;
		}
	}
}

static void state_read_json_materials(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	// @todo: arena/stack allocator
	struct Array_Byte uniform_data_buffer;
	array_byte_init(&uniform_data_buffer);

	uint32_t const materials_count = json_count(json);
	for (uint32_t i = 0; i < materials_count; i++) {
		struct JSON const * material_json = json_at(json, i);

		struct Gfx_Material material;
		state_read_json_material(material_json, &material, &uniform_data_buffer);

		array_any_push(&state.materials, &material);
	}

	array_byte_free(&uniform_data_buffer);
}

// ----- ----- ----- ----- -----
//     Cameras part
// ----- ----- ----- ----- -----

static void state_read_json_camera(struct JSON const * json, struct Camera * camera) {
	(void)json; (void)camera;

	struct JSON const * scale_json    = json_get(json, S_("scale"));
	struct JSON const * rotation_json = json_get(json, S_("rotation"));
	struct JSON const * position_json = json_get(json, S_("position"));

	float scale[3] = {1, 1, 1};
	if (scale_json->type == JSON_ARRAY) {
		for (uint32_t i = 0; i < 3; i++) {
			scale[i] = json_at_number(scale_json, i, 1);
		}
	}
	camera->transform.scale = (struct vec3){scale[0], scale[1], scale[2]};

	float rotation[3] = {0, 0, 0};
	if (rotation_json->type == JSON_ARRAY) {
		for (uint32_t i = 0; i < 3; i++) {
			rotation[i] = json_at_number(rotation_json, i, 0);
		}
	}
	camera->transform.rotation = quat_set_radians((struct vec3){rotation[0], rotation[1], rotation[2]});

	float position[3] = {0, 0, 0};
	if (position_json->type == JSON_ARRAY) {
		for (uint32_t i = 0; i < 3; i++) {
			position[i] = json_at_number(position_json, i, 0);
		}
	}
	camera->transform.position = (struct vec3){position[0], position[1], position[2]};

	camera->mode = CAMERA_MODE_NONE;
	uint32_t const mode_id = json_get_id(json, S_("mode"));
	if (mode_id == json_find_id(json, S_("screen"))) {
		camera->mode = CAMERA_MODE_SCREEN;
	}
	else if (mode_id == json_find_id(json, S_("aspect_x"))) {
		camera->mode = CAMERA_MODE_ASPECT_X;
	}
	else if (mode_id == json_find_id(json, S_("aspect_y"))) {
		camera->mode = CAMERA_MODE_ASPECT_Y;
	}

	camera->ncp   = json_get_number(json, S_("ncp"),   0);
	camera->fcp   = json_get_number(json, S_("fcp"),   0);
	camera->ortho = json_get_number(json, S_("ortho"), 0);

	camera->gpu_target_ref = (struct Ref){0};
	uint32_t const target_uid = (uint32_t)json_get_number(json, S_("target"), 0);
	if (target_uid != 0) {
		struct Ref const * gpu_target = array_any_at(&state.targets, target_uid - 1);
		if (gpu_target != NULL) { camera->gpu_target_ref = *gpu_target; }
	}

	camera->clear_mask = TEXTURE_TYPE_NONE;
	struct JSON const * clear_masks = json_get(json, S_("clear_mask"));
	if (clear_masks->type == JSON_ARRAY) {
		uint32_t const clear_masks_count = json_count(clear_masks);
		for (uint32_t i = 0; i < clear_masks_count; i++) {
			uint32_t const clear_mask_id = json_at_id(clear_masks, i);
			if (clear_mask_id == json_find_id(json, S_("color"))) {
				camera->clear_mask |= TEXTURE_TYPE_COLOR;
			}
			else if (clear_mask_id == json_find_id(json, S_("depth"))) {
				camera->clear_mask |= TEXTURE_TYPE_DEPTH;
			}
			else if (clear_mask_id == json_find_id(json, S_("stencil"))) {
				camera->clear_mask |= TEXTURE_TYPE_STENCIL;
			}
		}
	}

	struct CString const clear_rgba = json_get_string(json, S_("clear_rgba"), S_(""));
	camera->clear_rgba = parse_hex_u32(clear_rgba.data);
}

static void state_read_json_cameras(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const cameras_count = json_count(json);
	for (uint32_t i = 0; i < cameras_count; i++) {
		struct JSON const * camera_json = json_at(json, i);

		struct Camera camera;
		state_read_json_camera(camera_json, &camera);

		array_any_push(&state.cameras, &camera);
	}
}

// ----- ----- ----- ----- -----
//     Entities part
// ----- ----- ----- ----- -----

static void state_read_json_entity(struct JSON const * json, struct Entity * entity) {
	(void)json; (void)entity;
}

static void state_read_json_entities(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const entities_count = json_count(json);
	for (uint32_t i = 0; i < entities_count; i++) {
		struct JSON const * entity_json = json_at(json, i);

		struct Entity entity;
		state_read_json_entity(entity_json, &entity);

		array_any_push(&state.entities, &entity);
	}
}
