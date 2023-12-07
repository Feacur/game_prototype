#include "framework/memory.h"
#include "framework/formatter.h"
#include "framework/maths.h"
#include "framework/json_read.h"

#include "framework/containers/buffer.h"
#include "framework/containers/hashmap.h"

#include "framework/graphics/objects.h"
#include "framework/graphics/gfx_material.h"

#include "framework/systems/string_system.h"
#include "framework/systems/asset_system.h"
#include "framework/systems/material_system.h"

#include "framework/assets/font.h"

#include "asset_types.h"

//
#include "json_load.h"

static void json_fill_uniforms(struct JSON const * json, struct Gfx_Material * material);
struct Handle json_load_gfx_material(struct JSON const * json) {
	struct Handle ms_handle = material_system_aquire();
	if (json->type != JSON_OBJECT) { goto fail; }

	struct Gfx_Material * material = material_system_take(ms_handle);
	material->blend_mode = json_read_blend_mode(json_get(json, S_("blend")));
	material->depth_mode = json_read_depth_mode(json_get(json, S_("depth")));

	struct CString const shader_path = json_get_string(json, S_("shader"));
	if (shader_path.data == NULL) { goto fail; }

	struct Handle const ah_shader = asset_system_load(shader_path);
	struct Asset_Shader const * shader = asset_system_get(ah_shader);
	if (shader == NULL) { goto fail; }

	gfx_material_set_shader(material, shader->gh_program);
	json_fill_uniforms(json_get(json, S_("uniforms")), material);

	return ms_handle;

	// process errors
	fail: ERR("failed to load gfx material asset");
	return ms_handle;
}

struct Handle json_load_font_range(struct JSON const * json, uint32_t * from, uint32_t * to) {
	if (json->type != JSON_OBJECT) { return (struct Handle){0}; }

	struct CString const path = json_get_string(json, S_("path"));
	*from = (uint32_t)json_get_number(json, S_("from"));
	*to   = (uint32_t)json_get_number(json, S_("to"));

	return asset_system_load(path);
}

//

static struct Handle json_load_texture(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct CString const path = json_get_string(json, S_("path"));
	if (path.data == NULL) { goto fail; }

	struct Handle const ah_texture = asset_system_load(path);
	if (handle_is_null(ah_texture)) { goto fail; }

	void const * asset_ptr = asset_system_get(ah_texture);
	if (asset_ptr == NULL) { goto fail; }

	struct CString const type = asset_system_get_type(ah_texture);

	if (cstring_equals(type, S_("image"))) {
		struct Asset_Image const * asset = asset_ptr;
		return asset->gh_texture;
	}

	if (cstring_equals(type, S_("target"))) {
		struct Asset_Target const * asset = asset_ptr;
		enum Texture_Type const texture_type = json_read_texture_type(json_get(json, S_("buffer_type")));
		uint32_t const index = (uint32_t)json_get_number(json, S_("index"));
		return gpu_target_get_texture_handle(asset->gh_target, texture_type, index);
	}

	if (cstring_equals(type, S_("font"))) {
		struct Asset_Font const * asset = asset_ptr;
		return asset->gh_texture;
	}

	// process errors
	fail: ERR("failed to load texture asset");
	return (struct Handle){0};
}

static void json_load_many_texture(struct JSON const * json, uint32_t length, struct Handle * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = json_load_texture(json_at(json, i));
		}
	}
	else {
		*result = json_load_texture(json);
	}
}

static void json_fill_uniforms(struct JSON const * json, struct Gfx_Material * material) {
	struct Hashmap const * gpu_program_uniforms = gpu_program_get_uniforms(material->gh_program);
	if (gpu_program_uniforms == NULL) { goto fail_uniforms; }

	// @todo: arena/stack allocator
	struct Buffer uniform_data_buffer = buffer_init(NULL);

	FOR_GFX_UNIFORMS(&material->uniforms, it) {
		struct Gpu_Uniform const * gpu_uniform = hashmap_get(gpu_program_uniforms, &it.id);
		struct CString const uniform_name = string_system_get(it.id);

		uint32_t const uniform_bytes = data_type_get_size(gpu_uniform->type) * gpu_uniform->array_size;
		if (it.size != uniform_bytes) {
			ERR("uniform `%.*s` size mismatch: expected %u, material %u"
				, uniform_name.length, uniform_name.data
				, uniform_bytes, it.size
			); goto fail_field;
		}

		struct JSON const * uniform_json = json_get(json, uniform_name);
		if (uniform_json->type == JSON_NULL) { continue; }

		uint32_t const uniform_count = data_type_get_count(gpu_uniform->type) * gpu_uniform->array_size;
		uint32_t const json_elements_count = max_u32(1, json_count(uniform_json));
		if (json_elements_count != uniform_count) {
			ERR("uniform `%.*s` length mismatch: expected %u, json %u"
				, uniform_name.length, uniform_name.data
				, uniform_count, json_elements_count
			); goto fail_field;
		}

		buffer_ensure(&uniform_data_buffer, it.size);
		switch (data_type_get_element_type(gpu_uniform->type)) {
			default: ERR("unknown data type");
				goto fail_field;

			case DATA_TYPE_UNIT_U:
			case DATA_TYPE_UNIT_S:
			case DATA_TYPE_UNIT_F: {
				json_load_many_texture(uniform_json, uniform_count, uniform_data_buffer.data);
			} break;

			case DATA_TYPE_R32_U: {
				json_read_many_u32(uniform_json, uniform_count, uniform_data_buffer.data);
			} break;

			case DATA_TYPE_R32_S: {
				json_read_many_s32(uniform_json, uniform_count,  uniform_data_buffer.data);
			} break;

			case DATA_TYPE_R32_F: {
				json_read_many_flt(uniform_json, uniform_count, uniform_data_buffer.data);
			} break;
		}

		common_memcpy(it.value, uniform_data_buffer.data, it.size);
		continue;

		// process errors
		fail_field:
		common_memset(uniform_data_buffer.data, 0, it.size);
		REPORT_CALLSTACK(); DEBUG_BREAK();

	}

	buffer_free(&uniform_data_buffer);
	return;

	// process errors
	fail_uniforms: WRN("missing shader uniforms");
	REPORT_CALLSTACK(); DEBUG_BREAK();
}
