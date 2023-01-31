#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/json_read.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/systems/asset_system.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/material.h"

#include "asset_types.h"

//
#include "json_load.h"

static void json_fill_uniforms(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * material);
void json_load_gfx_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result) {
	if (json->type != JSON_OBJECT) { goto fail; }

	*result = gfx_material_init();
	result->blend_mode = json_read_blend_mode(json_get(json, S_("blend")));
	result->depth_mode = json_read_depth_mode(json_get(json, S_("depth")));

	struct CString const shader_path = json_get_string(json, S_("shader"));
	if (shader_path.data == NULL) { goto fail; }

	struct Asset_Shader const * shader_asset = asset_system_aquire_instance(system, shader_path);
	if (shader_asset == NULL) { goto fail; }

	gfx_material_set_shader(result, shader_asset->gpu_handle);
	json_fill_uniforms(system, json, result);

	return;

	// process errors
	fail: logger_to_console("failed to load gfx material asset\n");
}

//

static struct Handle json_load_texture(struct Asset_System * system, struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct CString const path = json_get_string(json, S_("path"));
	if (path.data == NULL) { goto fail; }

	struct Asset_Handle const asset_handle = asset_system_aquire(system, path);
	if (asset_handle_is_null(asset_handle)) { goto fail; }

	void const * instance = asset_system_find_instance(system, asset_handle);
	if (instance == NULL) { goto fail; }

	if (asset_system_match_type(system, asset_handle, S_("image"))) {
		struct Asset_Image const * asset = instance;
		return asset->gpu_handle;
	}

	if (asset_system_match_type(system, asset_handle, S_("target"))) {
		struct Asset_Target const * asset = instance;
		enum Texture_Type const texture_type = json_read_texture_type(json_get(json, S_("buffer_type")));
		uint32_t const index = (uint32_t)json_get_number(json, S_("index"));
		return gpu_target_get_texture_handle(asset->gpu_handle, texture_type, index);
	}

	if (asset_system_match_type(system, asset_handle, S_("fonts"))) {
		struct Asset_Fonts const * asset = instance;
		return asset->gpu_handle;
	}

	logger_to_console("unknown texture asset type\n"); DEBUG_BREAK();
	return (struct Handle){0};

	// process errors
	fail: logger_to_console("failed to load texture asset\n");
	return (struct Handle){0};
}

static void json_load_many_texture(struct Asset_System * system, struct JSON const * json, uint32_t length, struct Handle * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = json_load_texture(system, json_at(json, i));
		}
	}
	else {
		*result = json_load_texture(system, json);
	}
}

static void json_fill_uniforms(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * material) {
	struct Hash_Table_U32 const * gpu_program_uniforms = gpu_program_get_uniforms(material->gpu_program_handle);
	if (gpu_program_uniforms == NULL) { goto fail; }

	// @todo: arena/stack allocator
	struct Array_Any uniform_data_buffer = array_any_init(sizeof(uint8_t));

	FOR_HASH_TABLE_U32(gpu_program_uniforms, it) {
		struct Gpu_Uniform const * gpu_uniform = it.value;
		struct CString const uniform_name = graphics_get_uniform_value(it.key_hash);
		struct JSON const * uniform_json = json_get(json, uniform_name);

		if (uniform_json->type == JSON_NULL) { continue; }

		uint32_t const uniform_count = data_type_get_count(gpu_uniform->type) * gpu_uniform->array_size;
		uint32_t const uniform_bytes = data_type_get_size(gpu_uniform->type) * gpu_uniform->array_size;

		array_any_ensure(&uniform_data_buffer, uniform_bytes);

		uint32_t const json_elements_count = max_u32(1, json_count(uniform_json));
		if (json_elements_count != uniform_count) {
			logger_to_console(
				"uniform `%.*s` size mismatch: expected %u, found %u\n",
				uniform_name.length, uniform_name.data,
				uniform_count, json_elements_count
			); DEBUG_BREAK();
		}

		switch (data_type_get_element_type(gpu_uniform->type)) {
			default: logger_to_console("unknown data type\n"); DEBUG_BREAK(); break;

			case DATA_TYPE_UNIT_U:
			case DATA_TYPE_UNIT_S:
			case DATA_TYPE_UNIT_F: {
				json_load_many_texture(system, uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&material->uniforms, it.key_hash, (struct CArray){
					.size = sizeof(struct Handle) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_U: {
				json_read_many_u32(uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&material->uniforms, it.key_hash, (struct CArray){
					.size = sizeof(uint32_t) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_S: {
				json_read_many_s32(uniform_json,uniform_count,  uniform_data_buffer.data);
				gfx_uniforms_set(&material->uniforms, it.key_hash, (struct CArray){
					.size = sizeof(int32_t) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;

			case DATA_TYPE_R32_F: {
				json_read_many_flt(uniform_json, uniform_count, uniform_data_buffer.data);
				gfx_uniforms_set(&material->uniforms, it.key_hash, (struct CArray){
					.size = sizeof(float) * uniform_count,
					.data = uniform_data_buffer.data,
				});
			} break;
		}
	}

	array_any_free(&uniform_data_buffer);
	return;

	// process errors
	fail: logger_to_console("failed to fill uniforms\n");
}
