#include "framework/logger.h"

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

void state_read_json(struct JSON const * json) {
	struct JSON const * targets_json = json_get(json, S_("targets"));
	state_read_json_targets(targets_json);

	struct JSON const * materials_json = json_get(json, S_("materials"));
	state_read_json_materials(materials_json);
}

//

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

static void state_read_json_target(struct JSON const * json, struct Array_Any * parameters) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	parameters->count = 0;
	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct JSON const * buffer_json = json_at(buffers_json, i);
		state_read_json_target_buffer(buffer_json, parameters);
	}

	if (parameters->count == 0) { DEBUG_BREAK(); return; }

	uint32_t const size_x = (uint32_t)json_get_number(json, S_("size_x"), 320);
	uint32_t const size_y = (uint32_t)json_get_number(json, S_("size_y"), 180);

	array_any_push(&state.targets, (struct Ref[]){
		gpu_target_init(size_x, size_y, array_any_at(parameters, 0), parameters->count)
	});
}

//

static struct Ref state_read_json_uniform_image(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return ref_empty; }

	struct CString const path = json_get_string(json, S_("path"), S_NULL);
	if (path.data == NULL) { DEBUG_BREAK(); return ref_empty; }

	struct Asset_Image const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return ref_empty; }

	return asset->gpu_ref;
}

static struct Ref state_read_json_uniform_target(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return ref_empty; }

	uint32_t const id = (uint32_t)json_get_number(json, S_("id"), UINT16_MAX);
	if (id == UINT16_MAX) { DEBUG_BREAK(); return ref_empty; }

	struct Ref const * gpu_target = array_any_at(&state.targets, id);
	if (gpu_target == NULL) { DEBUG_BREAK(); return ref_empty; }

	uint32_t const buffer_type_id = json_get_id(json, S_("buffer_type"));
	if (buffer_type_id == INDEX_EMPTY) { DEBUG_BREAK(); return ref_empty; }

	if (buffer_type_id == json_find_id(json, S_("color"))) {
		return gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_COLOR, 0);
	}

	if (buffer_type_id == json_find_id(json, S_("depth"))) {
		return gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_DEPTH, 0);
	}

	DEBUG_BREAK();
	return ref_empty;
}

static struct Ref state_read_json_uniform_font(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return ref_empty; }

	struct CString const path = json_get_string(json, S_("path"), S_NULL);
	if (path.data == NULL) { DEBUG_BREAK(); return ref_empty; }

	struct Asset_Font const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return ref_empty; }

	return asset->gpu_ref;
}

static struct Ref state_read_json_uniform_texture(struct JSON const * json) {
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

static void state_read_json_material(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct CString const path = json_get_string(json, S_("shader"), S_NULL);
	if (path.data == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Shader const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return; }

	array_any_push(&state.materials, &(struct Gfx_Material){0});
	struct Gfx_Material * material = array_any_at(&state.materials, state.materials.count - 1);

	uint32_t const mode_id = json_get_id(json, S_("mode"));
	if (mode_id == INDEX_EMPTY) { DEBUG_BREAK(); return; }

	struct Blend_Mode blend_mode;

	if (mode_id == json_find_id(json, S_("opaque"))) {
		blend_mode = blend_mode_opaque;
	}
	else if (mode_id == json_find_id(json, S_("transparent"))) {
		blend_mode = blend_mode_transparent;
	}
	else {
		blend_mode = blend_mode_opaque;
	}

	bool const depth_write = json_get_boolean(json, S_("depth"), true);

	struct Depth_Mode depth_mode;
	if (depth_write) {
		depth_mode = (struct Depth_Mode){.enabled = true, .mask = true};
	}
	else {
		depth_mode = (struct Depth_Mode){0};
	}

	gfx_material_init(
		material, asset->gpu_ref,
		&blend_mode, &depth_mode
	);

	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->gpu_program_ref, &uniforms_count, &uniforms);

	struct Array_Byte uniforms_buffer;
	array_byte_init(&uniforms_buffer);
	array_byte_resize(&uniforms_buffer, 4 * sizeof(float));

	for (uint32_t i = 0; i < uniforms_count; i++) {
		struct CString const uniform_name = graphics_get_uniform_value(uniforms[i].id);
		struct JSON const * uniform_json = json_get(json, uniform_name);

		if (uniform_json->type == JSON_NULL) { continue; }

		if (uniform_json->type == JSON_ERROR) {
			logger_to_console("uniform '%.*s' is error\n", uniform_name.length, uniform_name.data); DEBUG_BREAK();
			continue;
		}

		uint32_t const elements_bytes = data_type_get_size(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms_buffer.capacity < elements_bytes) {
			array_byte_resize(&uniforms_buffer, elements_bytes);
		}

		if (uniform_json->type == JSON_ARRAY) {
			uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
			switch (data_type_get_element_type(uniforms[i].type)) {
				default: logger_to_console("unknown data type\n"); DEBUG_BREAK(); break;

				case DATA_TYPE_UNIT: {
					for (uint32_t element_index = 0; element_index < elements_count; element_index++) {
						struct Ref * data = (void *)uniforms_buffer.data;
						data[element_index] = state_read_json_uniform_texture(json_at(uniform_json, element_index));
					}
					gfx_material_set_texture(material, uniforms[i].id, elements_count, (void *)uniforms_buffer.data);
				} break;

				case DATA_TYPE_U32: {
					for (uint32_t element_index = 0; element_index < elements_count; element_index++) {
						uint32_t * data = (void *)uniforms_buffer.data;
						data[element_index] = (uint32_t)json_at_number(uniform_json, element_index, 0);
					}
					gfx_material_set_u32(material, uniforms[i].id, elements_count, (void *)uniforms_buffer.data);
				} break;

				case DATA_TYPE_S32: {
					for (uint32_t element_index = 0; element_index < elements_count; element_index++) {
						int32_t * data = (void *)uniforms_buffer.data;
						data[element_index] = (int32_t)json_at_number(uniform_json, element_index, 0);
					}
					gfx_material_set_s32(material, uniforms[i].id, elements_count, (void *)uniforms_buffer.data);
				} break;

				case DATA_TYPE_R32: {
					for (uint32_t element_index = 0; element_index < elements_count; element_index++) {
						float * data = (void *)uniforms_buffer.data;
						data[element_index] = json_at_number(uniform_json, element_index, 0);
					}
					gfx_material_set_float(material, uniforms[i].id, elements_count, (void *)uniforms_buffer.data);
				} break;
			}
		}
		else {
			switch (data_type_get_element_type(uniforms[i].type)) {
				default: logger_to_console("unknown data type\n"); DEBUG_BREAK(); break;

				case DATA_TYPE_UNIT: {
					struct Ref const data = state_read_json_uniform_texture(uniform_json);
					gfx_material_set_texture(material, uniforms[i].id, 1, &data);
				} break;

				case DATA_TYPE_U32: {
						uint32_t const data = (uint32_t)json_as_number(uniform_json, 0);
					gfx_material_set_u32(material, uniforms[i].id, 1, &data);
				} break;

				case DATA_TYPE_S32: {
					int32_t const data = (int32_t)json_as_number(uniform_json, 0);
					gfx_material_set_s32(material, uniforms[i].id, 1, &data);
				} break;

				case DATA_TYPE_R32: {
					float const data = json_as_number(uniform_json, 0);
					gfx_material_set_float(material, uniforms[i].id, 1, &data);
				} break;
			}
		}
	}

	array_byte_free(&uniforms_buffer);
}

//

static void state_read_json_targets(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	struct Array_Any parameters;
	array_any_init(&parameters, sizeof(struct Texture_Parameters));

	uint32_t const targets_count = json_count(json);
	for (uint32_t i = 0; i < targets_count; i++) {
		struct JSON const * target_json = json_at(json, i);
		state_read_json_target(target_json, &parameters);
	}

	array_any_free(&parameters);
}

static void state_read_json_materials(struct JSON const * json) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const materials_count = json_count(json);
	for (uint32_t i = 0; i < materials_count; i++) {
		struct JSON const * material_json = json_at(json, i);
		state_read_json_material(material_json);
	}
}
