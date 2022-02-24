#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/platform_file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/systems/asset_system.h"
#include "framework/graphics/gpu_objects.h"

#include "framework/assets/font.h"
#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/json.h"

//
#include "asset_types.h"

// ----- ----- ----- ----- -----
//     Asset shader part
// ----- ----- ----- ----- -----

void asset_shader_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Shader * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); return; }
	// @todo: return error shader?

	struct Ref const gpu_ref = gpu_program_init(&buffer);
	buffer_free(&buffer);

	asset->gpu_ref = gpu_ref;
}

void asset_shader_free(struct Asset_System * system, void * instance) {
	struct Asset_Shader * asset = instance;
	(void)system;
	gpu_program_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset model part
// ----- ----- ----- ----- -----

void asset_model_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Model * asset = instance;
	(void)system;

	struct Mesh mesh;
	mesh_init(&mesh, name);

	struct Ref const gpu_ref = gpu_mesh_init(&mesh);
	mesh_free(&mesh);

	asset->gpu_ref = gpu_ref;
}

void asset_model_free(struct Asset_System * system, void * instance) {
	struct Asset_Model * asset = instance;
	(void)system;
	gpu_mesh_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----

void asset_image_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Image * asset = instance;
	(void)system;

	struct Image image;
	image_init(&image, name);

	struct Ref const gpu_ref = gpu_texture_init(&image);
	image_free(&image);

	asset->gpu_ref = gpu_ref;
}

void asset_image_free(struct Asset_System * system, void * instance) {
	struct Asset_Image * asset = instance;
	(void)system;
	gpu_texture_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset font part
// ----- ----- ----- ----- -----

void asset_font_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Font * asset = instance;
	(void)system;

	struct Font * font = font_init(name);
	struct Font_Image * buffer = font_image_init(font, 32); // @todo: remove hardcode

	asset->font = font;
	asset->buffer = buffer;
	asset->gpu_ref = gpu_texture_init(font_image_get_asset(buffer));
}

void asset_font_free(struct Asset_System * system, void * instance) {
	struct Asset_Font * asset = instance;
	(void)system;
	font_free(asset->font);
	font_image_free(asset->buffer);
	gpu_texture_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset bytes part
// ----- ----- ----- ----- -----

void asset_bytes_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Bytes * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success) { DEBUG_BREAK(); }

	// @note: memory ownership transfer
	asset->data = buffer.data;
	asset->length = (uint32_t)buffer.count;
}

void asset_bytes_free(struct Asset_System * system, void * instance) {
	struct Asset_Bytes * asset = instance;
	(void)system;
	MEMORY_FREE(instance, asset->data);
}

// ----- ----- ----- ----- -----
//     Asset json part
// ----- ----- ----- ----- -----

static struct Strings gs_asset_json_strings;

void asset_json_type_init(void) {
	strings_init(&gs_asset_json_strings);
}

void asset_json_type_free(void) {
	strings_free(&gs_asset_json_strings);
}

void asset_json_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_JSON * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	json_init(&asset->value, &gs_asset_json_strings, (char const *)buffer.data);

	buffer_free(&buffer);
}

void asset_json_free(struct Asset_System * system, void * instance) {
	struct Asset_JSON * asset = instance;
	(void)system;
	json_free(&asset->value);
}

// ----- ----- ----- ----- -----
// -- Asset target part
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

static void state_read_json_target(struct JSON const * json, struct Ref * target_ref) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	struct Array_Any parameters_buffer;
	array_any_init(&parameters_buffer, sizeof(struct Texture_Parameters));

	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct JSON const * buffer_json = json_at(buffers_json, i);
		state_read_json_target_buffer(buffer_json, &parameters_buffer);
	}

	if (parameters_buffer.count > 0) {
		uint32_t const size_x = (uint32_t)json_get_number(json, S_("size_x"), 0);
		uint32_t const size_y = (uint32_t)json_get_number(json, S_("size_y"), 0);
		if (size_x > 0 && size_y > 0) {
			*target_ref = gpu_target_init(size_x, size_y, parameters_buffer.data, parameters_buffer.count);
		}
	}

	if (target_ref->id == ref_empty.id) { DEBUG_BREAK(); }
	array_any_free(&parameters_buffer);
}

void asset_target_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Target * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	struct Strings strings;
	strings_init(&strings);

	struct JSON json;
	json_init(&json, &strings, (char const *)buffer.data);
	buffer_free(&buffer);

	state_read_json_target(&json, &asset->gpu_ref);

	json_free(&json);
	strings_free(&strings);
}

void asset_target_free(struct Asset_System * system, void * instance) {
	struct Asset_Target * asset = instance;
	(void)system;
	gpu_target_free(asset->gpu_ref);
}

/*
// ----- ----- ----- ----- -----
// -- Asset material part
// ----- ----- ----- ----- -----

void asset_material_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Material * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	struct Strings strings;
	strings_init(&strings);

	struct JSON json;
	json_init(&json, &strings, (char const *)buffer.data);
	buffer_free(&buffer);

	(void)asset;
}

void asset_material_free(struct Asset_System * system, void * instance) {
	struct Asset_Material * asset = instance;
	(void)system;
	(void)asset;
}
*/
