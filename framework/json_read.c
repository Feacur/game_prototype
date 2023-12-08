#include "framework/maths.h"
#include "framework/formatter.h"
#include "framework/parsing.h"
#include "framework/json_read.h"
#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/assets/json.h"
#include "framework/graphics/gfx_objects.h"

#include "framework/platform/file.h"

#include "json_read.h"

//

void process_json(struct CString path, void * data, void (* action)(struct JSON const * json, void * output)) {
	struct Buffer file_buffer = platform_file_read_entire(path);
	if (file_buffer.capacity == 0) { action(&c_json_error, data); return; }

	struct JSON json = json_parse((char const *)file_buffer.data);
	buffer_free(&file_buffer);

	action(&json, data);

	json_free(&json);
}

// ----- ----- ----- ----- -----
//     common
// ----- ----- ----- ----- -----

uint32_t json_read_hex(struct JSON const * json) {
	struct CString const value = json_as_string(json);
	if (value.length > 2 && value.data[0] == '0' && value.data[1] == 'x')
	{
		return parse_hex_u32(value.data + 2);
	}
	return parse_hex_u32(value.data);
}

void json_read_many_flt(struct JSON const * json, uint32_t length, float * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (float)json_at_number(json, i);
		}
	}
	else if (json->type == JSON_NUMBER) {
		*result = (float)json_as_number(json);
	}
}

void json_read_many_u32(struct JSON const * json, uint32_t length, uint32_t * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (uint32_t)json_at_number(json, i);
		}
	}
	else if (json->type == JSON_NUMBER) {
		*result = (uint32_t)json_as_number(json);
	}
}

void json_read_many_s32(struct JSON const * json, uint32_t length, int32_t * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (int32_t)json_at_number(json, i);
		}
	}
	else if (json->type == JSON_NUMBER) {
		*result = (int32_t)json_as_number(json);
	}
}

// ----- ----- ----- ----- -----
//     graphics types
// ----- ----- ----- ----- -----

enum Texture_Type json_read_texture_type(struct JSON const * json) {
	enum Texture_Type texture_type = TEXTURE_TYPE_NONE;
	if (json->type == JSON_ARRAY) {
		uint32_t const count = json_count(json);
		for (uint32_t i = 0; i < count; i++) {
			struct CString const value = json_at_string(json, i);
			if (cstring_equals(value, S_("color"))) {
				texture_type |= TEXTURE_TYPE_COLOR;
			}
			else if (cstring_equals(value, S_("depth"))) {
				texture_type |= TEXTURE_TYPE_DEPTH;
			}
			else if (cstring_equals(value, S_("stencil"))) {
				texture_type |= TEXTURE_TYPE_STENCIL;
			}
		}
	}
	return texture_type;
}

enum Filter_Mode json_read_filter_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("point"))) {
			return FILTER_MODE_POINT;
		}
		if (cstring_equals(value, S_("lerp"))) {
			return FILTER_MODE_LERP;
		}
	}
	return FILTER_MODE_NONE;
}

enum Wrap_Mode json_read_wrap_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("edge"))) {
			return WRAP_MODE_EDGE;
		}
		if (cstring_equals(value, S_("repeat"))) {
			return WRAP_MODE_REPEAT;
		}
		if (cstring_equals(value, S_("border"))) {
			return WRAP_MODE_BORDER;
		}
		if (cstring_equals(value, S_("mirror_edge"))) {
			return WRAP_MODE_MIRROR_EDGE;
		}
		if (cstring_equals(value, S_("mirror_repeat"))) {
			return WRAP_MODE_MIRROR_REPEAT;
		}
	}
	return WRAP_MODE_NONE;
}

enum Blend_Mode json_read_blend_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("mix"))) {
			return BLEND_MODE_MIX;
		}
		if (cstring_equals(value, S_("pma"))) {
			return BLEND_MODE_PMA;
		}
		if (cstring_equals(value, S_("add"))) {
			return BLEND_MODE_ADD;
		}
		if (cstring_equals(value, S_("sub"))) {
			return BLEND_MODE_SUB;
		}
		if (cstring_equals(value, S_("mul"))) {
			return BLEND_MODE_MUL;
		}
		if (cstring_equals(value, S_("scr"))) {
			return BLEND_MODE_SCR;
		}
	}
	return BLEND_MODE_NONE;
}

enum Depth_Mode json_read_depth_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("transparent"))) {
			return DEPTH_MODE_TRANSPARENT;
		}
		if (cstring_equals(value, S_("opaque"))) {
			return DEPTH_MODE_OPAQUE;
		}
	}
	return DEPTH_MODE_NONE;
}

struct Texture_Parameters json_read_texture_parameters(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct CString const type = json_get_string(json, S_("type"));
	if (type.length == 0) { goto fail; }

	bool const buffer_read = json_get_boolean(json, S_("read"));

	if (cstring_equals(type, S_("color_rgba8_unorm"))) {
		return (struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_RGBA8_UNORM,
			.flags = buffer_read ? TEXTURE_FLAG_NONE : TEXTURE_FLAG_OPAQUE,
		};
	}

	if (cstring_equals(type, S_("depth_r32_f"))) {
		return (struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_DEPTH,
			.data_type = DATA_TYPE_R32_F,
			.flags = buffer_read ? TEXTURE_FLAG_NONE : TEXTURE_FLAG_OPAQUE,
		};
	}

	// process errors
	fail: ERR("unknown texture parameters");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Texture_Parameters){0};
}

struct Texture_Settings json_read_texture_settings(struct JSON const * json) {
	struct Texture_Settings result = {0};
	json_read_many_u32(json_get(json, S_("max_lod")), 1, &result.max_lod);
	return result;
}

struct Sampler_Settings json_read_sampler_settings(struct JSON const * json) {
	struct Sampler_Settings result = {
		.mipmap        = json_read_filter_mode(json_get(json, S_("mipmap"))),
		.minification  = json_read_filter_mode(json_get(json, S_("minification"))),
		.magnification = json_read_filter_mode(json_get(json, S_("magnification"))),
		.wrap_x        = json_read_wrap_mode(json_get(json, S_("wrap_x"))),
		.wrap_y        = json_read_wrap_mode(json_get(json, S_("wrap_y"))),
	};
	json_read_many_flt(json_get(json, S_("border")), 4, &result.border.x);
	return result;
}

// ----- ----- ----- ----- -----
//     graphics objects
// ----- ----- ----- ----- -----

struct Handle json_read_target(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { goto fail; }

	// @todo: arena/stack allocator
	struct Array parameters_buffer = array_init(sizeof(struct Texture_Parameters));

	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Texture_Parameters const parameters = json_read_texture_parameters(json_at(buffers_json, i));
		array_push_many(&parameters_buffer, 1, &parameters);
	}

	struct Handle result = (struct Handle){0};
	if (parameters_buffer.count > 0) {
		struct uvec2 size = {0};
		json_read_many_u32(json_get(json, S_("size")), 2, &size.x);
		if (size.x > 0 && size.y > 0) {
			result = gpu_target_init((struct GPU_Target_Asset){
				.size = size,
				.count = parameters_buffer.count,
				.parameters = parameters_buffer.data,
			});
		}
	}

	array_free(&parameters_buffer);
	return result;

	// process errors
	fail: ERR("failed to read target asset");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Handle){0};
}
