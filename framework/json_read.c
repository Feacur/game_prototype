#include "framework/maths.h"
#include "framework/formatter.h"
#include "framework/parsing.h"
#include "framework/json_read.h"

#include "framework/platform/file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"

#include "framework/graphics/gfx_objects.h"
#include "framework/assets/json.h"


//
#include "json_read.h"

void process_json(struct CString path, void * data, JSON_Processor * process) {
	struct Buffer file_buffer = platform_file_read_entire(path);
	if (file_buffer.capacity == 0) { process(&c_json_error, data); return; }

	struct JSON json = json_parse((struct CString){
		.length = (uint32_t)file_buffer.size,
		.data = file_buffer.data,
	});
	buffer_free(&file_buffer);

	process(&json, data);

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

enum Texture_Flag json_read_texture_flags(struct JSON const * json) {
	enum Texture_Flag flags = TEXTURE_FLAG_NONE;
	if (json->type == JSON_ARRAY) {
		uint32_t const count = json_count(json);
		for (uint32_t i = 0; i < count; i++) {
			struct CString const value = json_at_string(json, i);
			if (cstring_equals(value, S_("color"))) {
				flags |= TEXTURE_FLAG_COLOR;
			}
			else if (cstring_equals(value, S_("depth"))) {
				flags |= TEXTURE_FLAG_DEPTH;
			}
			else if (cstring_equals(value, S_("stencil"))) {
				flags |= TEXTURE_FLAG_STENCIL;
			}
		}
	}
	return flags;
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

enum Wrap_Flag json_read_wrap_flags(struct JSON const * json) {
	enum Wrap_Flag flags = WRAP_FLAG_NONE;
	if (json->type == JSON_ARRAY) {
		uint32_t const count = json_count(json);
		for (uint32_t i = 0; i < count; i++) {
			struct CString const value = json_at_string(json, i);
			if (cstring_equals(value, S_("mirror"))) {
				flags |= WRAP_FLAG_MIRROR;
			}
			else if (cstring_equals(value, S_("edge"))) {
				flags |= WRAP_FLAG_EDGE;
			}
			else if (cstring_equals(value, S_("repeat"))) {
				flags |= WRAP_FLAG_REPEAT;
			}
		}
	}
	return flags;
}

struct Sampler_Settings json_read_sampler_settings(struct JSON const * json) {
	struct Sampler_Settings result = {
		.mipmap        = json_read_filter_mode(json_get(json, S_("mipmap"))),
		.minification  = json_read_filter_mode(json_get(json, S_("minification"))),
		.magnification = json_read_filter_mode(json_get(json, S_("magnification"))),
		.wrap_x        = json_read_wrap_flags(json_get(json, S_("wrap_x"))),
		.wrap_y        = json_read_wrap_flags(json_get(json, S_("wrap_y"))),
	};
	json_read_many_flt(json_get(json, S_("border")), 4, &result.border.x);
	return result;
}

struct Texture_Settings json_read_texture_settings(struct JSON const * json) {
	struct Texture_Settings result = {
		.max_lod = (uint32_t)json_get_number(json, S_("max_lod")),
	};
	return result;
}

struct Target_Parameters json_read_target_parameters(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct CString const type = json_get_string(json, S_("type"));
	if (type.length == 0) { goto fail; }

	bool const read = json_get_boolean(json, S_("read"));

	if (cstring_equals(type, S_("color_rgba8_unorm"))) {
		return (struct Target_Parameters) {
			.image = {
				.flags = TEXTURE_FLAG_COLOR,
				.type = DATA_TYPE_RGBA8_UNORM,
			},
			.read = read,
		};
	}

	if (cstring_equals(type, S_("depth_r32_f"))) {
		return (struct Target_Parameters) {
			.image = {
				.flags = TEXTURE_FLAG_DEPTH,
				.type = DATA_TYPE_R32_F,
			},
			.read = read,
		};
	}

	// process errors
	fail: ERR("unknown target parameters");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Target_Parameters){0};
}

// ----- ----- ----- ----- -----
//     graphics objects
// ----- ----- ----- ----- -----

struct Handle json_read_target(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { goto fail; }

	// @todo: arena/stack allocator
	struct Array parameters_buffer = array_init(sizeof(struct Target_Parameters));

	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Target_Parameters const parameters = json_read_target_parameters(json_at(buffers_json, i));
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
