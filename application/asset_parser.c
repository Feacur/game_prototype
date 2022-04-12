#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/systems/asset_system.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"

#include "asset_types.h"

//
#include "asset_parser.h"

enum Texture_Type state_read_json_texture_type(struct JSON const * json) {
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

enum Filter_Mode state_read_json_filter_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		uint32_t const id = json_as_id(json);
		if (id == json_find_id(json, S_("point"))) {
			return FILTER_MODE_POINT;
		}
		if (id == json_find_id(json, S_("lerp"))) {
			return FILTER_MODE_LERP;
		}
	}
	return FILTER_MODE_NONE;
}

enum Wrap_Mode state_read_json_wrap_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		uint32_t const id = json_as_id(json);
		if (id == json_find_id(json, S_("edge"))) {
			return WRAP_MODE_EDGE;
		}
		if (id == json_find_id(json, S_("repeat"))) {
			return WRAP_MODE_REPEAT;
		}
		if (id == json_find_id(json, S_("border"))) {
			return WRAP_MODE_BORDER;
		}
		if (id == json_find_id(json, S_("mirror_edge"))) {
			return WRAP_MODE_MIRROR_EDGE;
		}
		if (id == json_find_id(json, S_("mirror_repeat"))) {
			return WRAP_MODE_MIRROR_REPEAT;
		}
	}
	return WRAP_MODE_NONE;
}

struct Texture_Settings state_read_json_texture_settings(struct JSON const * json) {
	return (struct Texture_Settings){
		.mipmap        = state_read_json_filter_mode(json_get(json, S_("mipmap"))),
		.minification  = state_read_json_filter_mode(json_get(json, S_("minification"))),
		.magnification = state_read_json_filter_mode(json_get(json, S_("magnification"))),
		.wrap_x        = state_read_json_wrap_mode(json_get(json, S_("wrap_x"))),
		.wrap_y        = state_read_json_wrap_mode(json_get(json, S_("wrap_y"))),
	};
}

static void state_read_json_uniform_texture(struct Asset_System * system, struct JSON const * json, struct Ref * result);
void state_read_json_unt_n(struct Asset_System * system, struct JSON const * json, uint32_t length, struct Ref * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t element_index = 0; element_index < count; element_index++) {
			state_read_json_uniform_texture(system, json_at(json, element_index), result + element_index);
		}
	}
	else {
		state_read_json_uniform_texture(system, json, result);
	}
}

void state_read_json_flt_n(struct JSON const * json, uint32_t length, float * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = json_at_number(json, i, result[i]);
		}
	}
	else {
		result[0] = json_as_number(json, result[0]);
	}
}

void state_read_json_u32_n(struct JSON const * json, uint32_t length, uint32_t * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (uint32_t)json_at_number(json, i, (float)result[i]);
		}
	}
	else {
		result[0] = (uint32_t)json_as_number(json, (float)result[0]);
	}
}

void state_read_json_s32_n(struct JSON const * json, uint32_t length, int32_t * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (int32_t)json_at_number(json, i, (float)result[i]);
		}
	}
	else {
		result[0] = (int32_t)json_as_number(json, (float)result[0]);
	}
}

struct Blend_Mode state_read_json_blend_mode(struct JSON const * json) {
	uint32_t const mode_id = json_get_id(json, S_("mode"));

	if (mode_id == json_find_id(json, S_("opaque"))) {
		return c_blend_mode_opaque;
	}

	if (mode_id == json_find_id(json, S_("transparent"))) {
		return c_blend_mode_transparent;
	}

	return c_blend_mode_opaque;
}

struct Depth_Mode state_read_json_depth_mode(struct JSON const * json) {
	bool const depth_write = json_get_boolean(json, S_("depth"), false);

	if (depth_write) {
		return (struct Depth_Mode){.enabled = true, .mask = true};
	}

	return (struct Depth_Mode){0};
}

static struct Texture_Parameters state_read_json_texture_parameters(struct JSON const * json);
struct Ref state_read_json_target(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { return c_ref_empty; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { return c_ref_empty; }

	struct Array_Any parameters_buffer = array_any_init(sizeof(struct Texture_Parameters));

	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Texture_Parameters const texture_parameters = state_read_json_texture_parameters(json_at(buffers_json, i));
		array_any_push(&parameters_buffer, &texture_parameters);
	}

	struct Ref result = c_ref_empty;
	if (parameters_buffer.count > 0) {
		uint32_t const size_x = (uint32_t)json_get_number(json, S_("size_x"), 0);
		uint32_t const size_y = (uint32_t)json_get_number(json, S_("size_y"), 0);
		if (size_x > 0 && size_y > 0) {
			result = gpu_target_init(size_x, size_y, parameters_buffer.count, parameters_buffer.data);
		}
	}

	array_any_free(&parameters_buffer);
	return result;
}

struct Gfx_Material state_read_json_material(struct Asset_System * system, struct JSON const * json) {
	if (json->type != JSON_OBJECT) { return (struct Gfx_Material){0}; }

	struct CString const shader_path = json_get_string(json, S_("shader"), S_NULL);
	if (shader_path.data == NULL) { return (struct Gfx_Material){0}; }

	struct Asset_Shader const * shader_asset = asset_system_aquire_instance(system, shader_path);
	if (shader_asset == NULL) { return (struct Gfx_Material){0}; }

	struct Blend_Mode blend_mode = state_read_json_blend_mode(json);
	struct Depth_Mode depth_mode = state_read_json_depth_mode(json);

	struct Gfx_Material result = gfx_material_init(
		shader_asset->gpu_ref,
		&blend_mode, &depth_mode
	);

	struct Hash_Table_U32 const * uniforms = gpu_program_get_uniforms(result.gpu_program_ref);

	struct Array_Any uniform_data_buffer = array_any_init(sizeof(uint8_t));

	FOR_HASH_TABLE_U32(uniforms, it) {
		struct Gpu_Program_Field const * uniform = it.value;
		struct CString const uniform_name = graphics_get_uniform_value(it.key_hash);
		struct JSON const * uniform_json = json_get(json, uniform_name);

		if (uniform_json->type == JSON_NULL) { continue; }

		uint32_t const uniform_count = data_type_get_count(uniform->type) * uniform->array_size;
		uint32_t const uniform_bytes = data_type_get_size(uniform->type) * uniform->array_size;

		array_any_ensure(&uniform_data_buffer, uniform_bytes);

		uint32_t const json_elements_count = max_u32(1, json_count(uniform_json));
		if (json_elements_count != uniform_count) {
			logger_to_console(
				"uniform `%.*s` size mismatch: expected %u, found %u\n",
				uniform_name.length, uniform_name.data,
				uniform_count, json_elements_count
			); DEBUG_BREAK();
		}

		switch (data_type_get_element_type(uniform->type)) {
			default: logger_to_console("unknown data type\n"); DEBUG_BREAK(); break;

			case DATA_TYPE_UNIT: {
				state_read_json_unt_n(system, uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&result.uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(struct Ref) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_U: {
				state_read_json_u32_n(uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&result.uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(uint32_t) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_S: {
				state_read_json_s32_n(uniform_json,uniform_count,  uniform_data_buffer.data);
				gfx_uniforms_set(&result.uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(int32_t) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_F: {
				state_read_json_flt_n(uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&result.uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(float) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;
		}
	}

	array_any_free(&uniform_data_buffer);
	return result;
}

//

static struct Texture_Parameters state_read_json_texture_parameters(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { return (struct Texture_Parameters){0}; }

	uint32_t const type_id = json_get_id(json, S_("type"));
	if (type_id == INDEX_EMPTY) { return (struct Texture_Parameters){0}; }

	bool const buffer_read = json_get_boolean(json, S_("read"), false);

	if (type_id == json_find_id(json, S_("color_rgba8_unorm"))) {
		return (struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_RGBA8_UNORM,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		};
	}

	if (type_id == json_find_id(json, S_("depth_r32_f"))) {
		return (struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_DEPTH,
			.data_type = DATA_TYPE_R32_F,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		};
	}

	return (struct Texture_Parameters){0};
}

static void state_read_json_uniform_texture(struct Asset_System * system, struct JSON const * json, struct Ref * result) {
	if (json->type != JSON_OBJECT) { return; }

	struct CString const path = json_get_string(json, S_("path"), S_NULL);
	if (path.data == NULL) { return; }

	struct Asset_Ref const asset_ref = asset_system_aquire(system, path);
	if (asset_ref.name_id == c_asset_ref_empty.name_id) { return; }

	void const * instance = asset_system_find_instance(system, asset_ref);
	if (instance == NULL) { return; }

	if (asset_system_match_type(system, asset_ref, S_("image"))) {
		struct Asset_Image const * asset = instance;
		*result = asset->gpu_ref;
		return;
	}

	if (asset_system_match_type(system, asset_ref, S_("target"))) {
		struct Asset_Target const * asset = instance;
		enum Texture_Type const texture_type = state_read_json_texture_type(json_get(json, S_("buffer_type")));
		uint32_t const index = (uint32_t)json_get_number(json, S_("index"), 0);
		*result = gpu_target_get_texture_ref(asset->gpu_ref, texture_type, index);
		return;
	}

	if (asset_system_match_type(system, asset_ref, S_("font"))) {
		struct Asset_Font const * asset = instance;
		*result = asset->gpu_ref;
		return;
	}
}
