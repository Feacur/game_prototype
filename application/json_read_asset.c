#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/systems/asset_system.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/material.h"

#include "asset_types.h"
#include "json_read.h"
#include "json_read_types.h"

//
#include "json_read_asset.h"

struct Ref json_read_target(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { goto fail; }

	// @todo: arena/stack allocator
	struct Array_Any parameters_buffer = array_any_init(sizeof(struct Texture_Parameters));

	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Texture_Parameters const texture_parameters = json_read_texture_parameters(json_at(buffers_json, i));
		array_any_push_many(&parameters_buffer, 1, &texture_parameters);
	}

	struct Ref result = (struct Ref){0};
	if (parameters_buffer.count > 0) {
		uint32_t const size_x = (uint32_t)json_get_number(json, S_("size_x"));
		uint32_t const size_y = (uint32_t)json_get_number(json, S_("size_y"));
		if (size_x > 0 && size_y > 0) {
			result = gpu_target_init(size_x, size_y, parameters_buffer.count, parameters_buffer.data);
		}
	}

	array_any_free(&parameters_buffer);
	return result;

	fail: logger_to_console("failed to read target\n"); DEBUG_BREAK();
	return (struct Ref){0};
}

struct Ref json_read_texture(struct Asset_System * system, struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct CString const path = json_get_string(json, S_("path"));
	if (path.data == NULL) { goto fail; }

	struct Asset_Ref const asset_ref = asset_system_aquire(system, path);
	if (asset_ref_equals(asset_ref, (struct Asset_Ref){0})) { goto fail; }

	void const * instance = asset_system_find_instance(system, asset_ref);
	if (instance == NULL) { goto fail; }

	if (asset_system_match_type(system, asset_ref, S_("image"))) {
		struct Asset_Image const * asset = instance;
		return asset->gpu_ref;
	}

	if (asset_system_match_type(system, asset_ref, S_("target"))) {
		struct Asset_Target const * asset = instance;
		enum Texture_Type const texture_type = json_read_texture_type(json_get(json, S_("buffer_type")));
		uint32_t const index = (uint32_t)json_get_number(json, S_("index"));
		return gpu_target_get_texture_ref(asset->gpu_ref, texture_type, index);
	}

	if (asset_system_match_type(system, asset_ref, S_("font"))) {
		struct Asset_Font const * asset = instance;
		return asset->gpu_ref;
	}

	fail: logger_to_console("failed to read texture\n"); DEBUG_BREAK();
	return (struct Ref){0};
}

void json_read_many_texture(struct Asset_System * system, struct JSON const * json, uint32_t length, struct Ref * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = json_read_texture(system, json_at(json, i));
		}
	}
	else {
		*result = json_read_texture(system, json);
	}
}

void json_read_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct CString const shader_path = json_get_string(json, S_("shader"));
	if (shader_path.data == NULL) { goto fail; }

	struct Asset_Shader const * shader_asset = asset_system_aquire_instance(system, shader_path);
	if (shader_asset == NULL) { goto fail; }

	enum Blend_Mode const blend_mode = json_read_blend_mode(json_get(json, S_("blend")));
	enum Depth_Mode const depth_mode = json_read_depth_mode(json_get(json, S_("depth")));

	*result = gfx_material_init(
		shader_asset->gpu_ref,
		blend_mode, depth_mode
	);

	struct Hash_Table_U32 const * uniforms = gpu_program_get_uniforms(result->gpu_program_ref);

	// @todo: arena/stack allocator
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

			case DATA_TYPE_UNIT_U:
			case DATA_TYPE_UNIT_S:
			case DATA_TYPE_UNIT_F: {
				json_read_many_texture(system, uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&result->uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(struct Ref) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_U: {
				json_read_many_u32(uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&result->uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(uint32_t) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_S: {
				json_read_many_s32(uniform_json,uniform_count,  uniform_data_buffer.data);
				gfx_uniforms_set(&result->uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(int32_t) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_F: {
				json_read_many_flt(uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&result->uniforms, it.key_hash, (struct Gfx_Uniform_In){
					.size = sizeof(float) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;
		}
	}

	array_any_free(&uniform_data_buffer);
	return;

	fail: logger_to_console("failed to read material\n"); DEBUG_BREAK();
	*result = (struct Gfx_Material){0};
}
