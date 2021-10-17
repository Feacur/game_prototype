#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/parsing.h"

#include "framework/containers/ref.h"
#include "framework/containers/strings.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_command.h"

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
		array_any_init(&state.gpu_commands, sizeof(struct GPU_Command));
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

	array_any_free(&state.gpu_commands);
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
	state_read_json_targets(json_get(json, S_("targets")));
	state_read_json_materials(json_get(json, S_("materials")));
	state_read_json_cameras(json_get(json, S_("cameras")));
	state_read_json_entities(json_get(json, S_("entities")));
}

static void state_read_json_float_n(struct JSON const * json, uint32_t length, float * data) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			data[i] = json_at_number(json, i, data[i]);
		}
	}
	else {
		data[0] = json_as_number(json, data[0]);
	}
}

static void state_read_json_u32_n(struct JSON const * json, uint32_t length, uint32_t * data) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			data[i] = (uint32_t)json_at_number(json, i, (float)data[i]);
		}
	}
	else {
		data[0] = (uint32_t)json_as_number(json, (float)data[0]);
	}
}

static void state_read_json_s32_n(struct JSON const * json, uint32_t length, int32_t * data) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			data[i] = (int32_t)json_at_number(json, i, (float)data[i]);
		}
	}
	else {
		data[0] = (int32_t)json_as_number(json, (float)data[0]);
	}
}

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

static enum Texture_Type state_read_json_texture_type(struct JSON const * json) {
	enum Texture_Type texture_type = TEXTURE_TYPE_NONE;
	if (json->type == JSON_ARRAY) {
		uint32_t const count = json_count(json);
		for (uint32_t i = 0; i < count; i++) {
			uint32_t const id = json_at_id(json, i);
			if (id == json_find_id(json, S_("color"))) {
				texture_type |= TEXTURE_TYPE_COLOR;
			}
			else if (id == json_find_id(json, S_("depth"))) {
				texture_type |= TEXTURE_TYPE_DEPTH;
			}
			else if (id == json_find_id(json, S_("stencil"))) {
				texture_type |= TEXTURE_TYPE_STENCIL;
			}
		}
	}
	return texture_type;
}

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

static enum Entity_Rect_Behaviour state_read_json_entity_rect_behaviour(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		uint32_t const id = json_as_id(json);
		if (id == json_find_id(json, S_("fit"))) {
			return ENTITY_RECT_BEHAVIOUR_FIT;
		}
	}
	return ENTITY_RECT_BEHAVIOUR_NONE;
}

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

static uint32_t state_read_json_hex(struct JSON const * json) {
	struct CString const value = json_as_string(json, S_EMPTY);
	if (value.length > 2 && value.data[0] == '0' && value.data[1] == 'x')
	{
		return parse_hex_u32(value.data + 2);
	}
	return 0;
}

static struct Ref state_read_json_get_target_ref(struct JSON const * json) {
	uint32_t const target_uid = (uint32_t)json_as_number(json, 0);
	if (target_uid != 0) {
		struct Ref const * gpu_target = array_any_at(&state.targets, target_uid - 1);
		if (gpu_target != NULL) { return *gpu_target; }
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
	if (json->type == JSON_OBJECT) {
		struct CString     const   path  = json_get_string(json, S_("path"), S_NULL);
		struct Asset_Image const * asset = asset_system_find_instance(&state.asset_system, path);
		if (asset != NULL) { return asset->gpu_ref; }
	}
	return ref_empty;
}

static struct Ref state_read_json_uniform_target(struct JSON const * json) {
	if (json->type == JSON_OBJECT) {
		struct Ref        const gpu_target   = state_read_json_get_target_ref(json_get(json, S_("uid")));
		enum Texture_Type const texture_type = state_read_json_texture_type(json_get(json, S_("buffer_type")));
		uint32_t          const index        = (uint32_t)json_get_number(json, S_("index"), 0);
		return gpu_target_get_texture_ref(gpu_target, texture_type, index);
	}
	return ref_empty;
}

static struct Ref state_read_json_uniform_font(struct JSON const * json) {
	if (json->type == JSON_OBJECT) {
		struct CString    const   path  = json_get_string(json, S_("path"), S_NULL);
		struct Asset_Font const * asset = asset_system_find_instance(&state.asset_system, path);
		if (asset != NULL) { return asset->gpu_ref; }
	}
	return ref_empty;
}

static struct Ref state_read_json_uniform_texture(struct JSON const * json) {
	if (json->type == JSON_OBJECT) {
		uint32_t const type_id = json_get_id(json, S_("type"));
		if (type_id == json_find_id(json, S_("image"))) {
			return state_read_json_uniform_image(json);
		}
		if (type_id == json_find_id(json, S_("target"))) {
			return state_read_json_uniform_target(json);
		}
		if (type_id == json_find_id(json, S_("font"))) {
			return state_read_json_uniform_font(json);
		}
	}
	return ref_empty;
}

static void state_read_json_material(struct JSON const * json, struct Gfx_Material * material, struct Array_Any * uniform_data_buffer) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct CString      const   path  = json_get_string(json, S_("shader"), S_NULL);
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
			array_any_resize(uniform_data_buffer, uniform_bytes);
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
					uint32_t const count = min_u32(uniform_elements_count, json_count(uniform_json));
					for (uint32_t element_index = 0; element_index < count; element_index++) {
						struct Ref * data = uniform_data_buffer->data;
						data[element_index] = state_read_json_uniform_texture(json_at(uniform_json, element_index));
					}
				}
				else {
					struct Ref * data = uniform_data_buffer->data;
					data[0] = state_read_json_uniform_texture(uniform_json);
				}
				gfx_material_set_texture(material, uniforms[i].id, uniform_elements_count, uniform_data_buffer->data);
			} break;

			case DATA_TYPE_U32: {
				state_read_json_u32_n(uniform_json, uniform_elements_count, uniform_data_buffer->data);
				gfx_material_set_u32(material, uniforms[i].id, uniform_elements_count, uniform_data_buffer->data);
			} break;

			case DATA_TYPE_S32: {
				state_read_json_s32_n(uniform_json,uniform_elements_count,  uniform_data_buffer->data);
				gfx_material_set_s32(material, uniforms[i].id, uniform_elements_count, uniform_data_buffer->data);
			} break;

			case DATA_TYPE_R32: {
				state_read_json_float_n(uniform_json, uniform_elements_count, uniform_data_buffer->data);
				gfx_material_set_float(material, uniforms[i].id, uniform_elements_count, uniform_data_buffer->data);
			} break;
		}
	}
}

static void state_read_json_materials(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	// @todo: arena/stack allocator
	struct Array_Any uniform_data_buffer;
	array_any_init(&uniform_data_buffer, sizeof(uint8_t));

	uint32_t const materials_count = json_count(json);
	for (uint32_t i = 0; i < materials_count; i++) {
		struct JSON const * material_json = json_at(json, i);

		struct Gfx_Material material;
		state_read_json_material(material_json, &material, &uniform_data_buffer);

		array_any_push(&state.materials, &material);
	}

	array_any_free(&uniform_data_buffer);
}

// ----- ----- ----- ----- -----
//     Cameras part
// ----- ----- ----- ----- -----

static void state_read_json_camera(struct JSON const * json, struct Camera * camera) {
	state_read_json_transform_3d(json_get(json, S_("transform")), &camera->transform);

	camera->mode  = state_read_json_camera_mode(json_get(json, S_("mode")));

	camera->ncp   = json_get_number(json, S_("ncp"),   0);
	camera->fcp   = json_get_number(json, S_("fcp"),   0);
	camera->ortho = json_get_number(json, S_("ortho"), 0);

	camera->gpu_target_ref = state_read_json_get_target_ref(json_get(json, S_("target_uid")));
	camera->clear_mask = state_read_json_texture_type(json_get(json, S_("clear_mask")));
	camera->clear_rgba = state_read_json_hex(json_get(json, S_("clear_rgba")));
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
	state_read_json_transform_3d(json_get(json, S_("transform")), &entity->transform);
	state_read_json_transform_rect(json_get(json, S_("rect")), &entity->rect);

	entity->rect_behaviour = state_read_json_entity_rect_behaviour(json_get(json, S_("rect_auto")));

	entity->material = (uint32_t)json_get_number(json, S_("material_uid"), 0) - 1;
	entity->camera   = (uint32_t)json_get_number(json, S_("camera_uid"), 0) - 1;

	entity->type = state_read_json_entity_type(json_get(json, S_("type")));
	switch (entity->type) {
		case ENTITY_TYPE_NONE: break;

		case ENTITY_TYPE_MESH: {
			struct CString const model_path = json_get_string(json, S_("model"), S_NULL);
			struct Asset_Model const * model = asset_system_find_instance(&state.asset_system, model_path);
			entity->as.mesh = (struct Entity_Mesh){
				.gpu_mesh_ref = (model != NULL) ? model->gpu_ref : ref_empty,
			};
		} break;

		case ENTITY_TYPE_QUAD_2D:
			struct CString const uniform = json_get_string(json, S_("uniform"), S_NULL);
			entity->as.quad.texture_uniform = graphics_add_uniform_id(uniform);
			break;

		case ENTITY_TYPE_TEXT_2D:
			break;
	}
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
