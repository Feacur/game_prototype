#include "framework/maths.h"
#include "framework/formatter.h"
#include "framework/parsing.h"
#include "framework/json_read.h"

#include "framework/platform/file.h"
#include "framework/containers/buffer.h"
#include "framework/systems/memory.h"

#include "framework/graphics/gfx_objects.h"
#include "framework/assets/json.h"


//
#include "json_read.h"

void process_json(struct CString path, void * data, JSON_Processor * process) {
	struct Buffer file_buffer = platform_file_read_entire(path);
	if (file_buffer.capacity == 0) { process(&c_json_null, data); return; }

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

enum Gfx_Type json_read_gfx_type(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);

		// samplers
		if (cstring_equals(value, S_("unit_u"))) { return GFX_TYPE_UNIT_U; }
		if (cstring_equals(value, S_("unit_s"))) { return GFX_TYPE_UNIT_S; }
		if (cstring_equals(value, S_("unit_f"))) { return GFX_TYPE_UNIT_F; }

		// u8
		if (cstring_equals(value, S_("r8_u")))    { return GFX_TYPE_R8_U; }
		if (cstring_equals(value, S_("rg8_u")))   { return GFX_TYPE_RG8_U; }
		if (cstring_equals(value, S_("rgb8_u")))  { return GFX_TYPE_RGB8_U; }
		if (cstring_equals(value, S_("rgba8_u"))) { return GFX_TYPE_RGBA8_U; }

		if (cstring_equals(value, S_("r8_unorm")))    { return GFX_TYPE_R8_UNORM; }
		if (cstring_equals(value, S_("rg8_unorm")))   { return GFX_TYPE_RG8_UNORM; }
		if (cstring_equals(value, S_("rgb8_unorm")))  { return GFX_TYPE_RGB8_UNORM; }
		if (cstring_equals(value, S_("rgba8_unorm"))) { return GFX_TYPE_RGBA8_UNORM; }

		// s8
		if (cstring_equals(value, S_("r8_s")))    { return GFX_TYPE_R8_S; }
		if (cstring_equals(value, S_("rg8_s")))   { return GFX_TYPE_RG8_S; }
		if (cstring_equals(value, S_("rgb8_s")))  { return GFX_TYPE_RGB8_S; }
		if (cstring_equals(value, S_("rgba8_s"))) { return GFX_TYPE_RGBA8_S; }

		if (cstring_equals(value, S_("r8_snorm")))    { return GFX_TYPE_R8_SNORM; }
		if (cstring_equals(value, S_("rg8_snorm")))   { return GFX_TYPE_RG8_SNORM; }
		if (cstring_equals(value, S_("rgb8_snorm")))  { return GFX_TYPE_RGB8_SNORM; }
		if (cstring_equals(value, S_("rgba8_snorm"))) { return GFX_TYPE_RGBA8_SNORM; }

		// u16
		if (cstring_equals(value, S_("r16_u")))    { return GFX_TYPE_R16_U; }
		if (cstring_equals(value, S_("rg16_u")))   { return GFX_TYPE_RG16_U; }
		if (cstring_equals(value, S_("rgb16_u")))  { return GFX_TYPE_RGB16_U; }
		if (cstring_equals(value, S_("rgba16_u"))) { return GFX_TYPE_RGBA16_U; }

		if (cstring_equals(value, S_("r16_unorm")))    { return GFX_TYPE_R16_UNORM; }
		if (cstring_equals(value, S_("rg16_unorm")))   { return GFX_TYPE_RG16_UNORM; }
		if (cstring_equals(value, S_("rgb16_unorm")))  { return GFX_TYPE_RGB16_UNORM; }
		if (cstring_equals(value, S_("rgba16_unorm"))) { return GFX_TYPE_RGBA16_UNORM; }

		// s16
		if (cstring_equals(value, S_("r16_s")))    { return GFX_TYPE_R16_S; }
		if (cstring_equals(value, S_("rg16_s")))   { return GFX_TYPE_RG16_S; }
		if (cstring_equals(value, S_("rgb16_s")))  { return GFX_TYPE_RGB16_S; }
		if (cstring_equals(value, S_("rgba16_s"))) { return GFX_TYPE_RGBA16_S; }

		if (cstring_equals(value, S_("r16_snorm")))    { return GFX_TYPE_R16_SNORM; }
		if (cstring_equals(value, S_("rg16_snorm")))   { return GFX_TYPE_RG16_SNORM; }
		if (cstring_equals(value, S_("rgb16_snorm")))  { return GFX_TYPE_RGB16_SNORM; }
		if (cstring_equals(value, S_("rgba16_snorm"))) { return GFX_TYPE_RGBA16_SNORM; }

		// u32
		if (cstring_equals(value, S_("r32_u")))    { return GFX_TYPE_R32_U; }
		if (cstring_equals(value, S_("rg32_u")))   { return GFX_TYPE_RG32_U; }
		if (cstring_equals(value, S_("rgb32_u")))  { return GFX_TYPE_RGB32_U; }
		if (cstring_equals(value, S_("rgba32_u"))) { return GFX_TYPE_RGBA32_U; }

		// s32
		if (cstring_equals(value, S_("r32_s")))    { return GFX_TYPE_R32_S; }
		if (cstring_equals(value, S_("rg32_s")))   { return GFX_TYPE_RG32_S; }
		if (cstring_equals(value, S_("rgb32_s")))  { return GFX_TYPE_RGB32_S; }
		if (cstring_equals(value, S_("rgba32_s"))) { return GFX_TYPE_RGBA32_S; }

		// floating
		if (cstring_equals(value, S_("r16_f")))    { return GFX_TYPE_R16_F; }
		if (cstring_equals(value, S_("rg16_f")))   { return GFX_TYPE_RG16_F; }
		if (cstring_equals(value, S_("rgb16_f")))  { return GFX_TYPE_RGB16_F; }
		if (cstring_equals(value, S_("rgba16_f"))) { return GFX_TYPE_RGBA16_F; }

		if (cstring_equals(value, S_("r32_f")))    { return GFX_TYPE_R32_F; }
		if (cstring_equals(value, S_("rg32_f")))   { return GFX_TYPE_RG32_F; }
		if (cstring_equals(value, S_("rgb32_f")))  { return GFX_TYPE_RGB32_F; }
		if (cstring_equals(value, S_("rgba32_f"))) { return GFX_TYPE_RGBA32_F; }

		if (cstring_equals(value, S_("r64_f")))    { return GFX_TYPE_R64_F; }
		if (cstring_equals(value, S_("rg64_f")))   { return GFX_TYPE_RG64_F; }
		if (cstring_equals(value, S_("rgb64_f")))  { return GFX_TYPE_RGB64_F; }
		if (cstring_equals(value, S_("rgba64_f"))) { return GFX_TYPE_RGBA64_F; }

		// matrices
		if (cstring_equals(value, S_("mat2"))) { return GFX_TYPE_MAT2; }
		if (cstring_equals(value, S_("mat3"))) { return GFX_TYPE_MAT3; }
		if (cstring_equals(value, S_("mat4"))) { return GFX_TYPE_MAT4; }
	}
	return GFX_TYPE_NONE;
}

enum Blend_Mode json_read_blend_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);

		if (cstring_equals(value, S_("mix"))) { return BLEND_MODE_MIX; }
		if (cstring_equals(value, S_("pma"))) { return BLEND_MODE_PMA; }
		if (cstring_equals(value, S_("add"))) { return BLEND_MODE_ADD; }
		if (cstring_equals(value, S_("sub"))) { return BLEND_MODE_SUB; }
		if (cstring_equals(value, S_("mul"))) { return BLEND_MODE_MUL; }
		if (cstring_equals(value, S_("scr"))) { return BLEND_MODE_SCR; }
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

enum Mipmap_Mode json_read_mipmap_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("near"))) {
			return MIPMAP_MODE_NEAR;
		}
		if (cstring_equals(value, S_("lerp"))) {
			return MIPMAP_MODE_LERP;
		}
	}
	return MIPMAP_MODE_NONE;
}

enum Filter_Mode json_read_filter_mode(struct JSON const * json) {
	if (json->type == JSON_STRING) {
		struct CString const value = json_as_string(json);
		if (cstring_equals(value, S_("near"))) {
			return FILTER_MODE_LERP;
		}
		if (cstring_equals(value, S_("lerp"))) {
			return FILTER_MODE_LERP;
		}
	}
	return FILTER_MODE_NONE;
}

enum Addr_Flag json_read_addr_flags(struct JSON const * json) {
	enum Addr_Flag flags = ADDR_FLAG_NONE;
	if (json->type == JSON_ARRAY) {
		uint32_t const count = json_count(json);
		for (uint32_t i = 0; i < count; i++) {
			struct CString const value = json_at_string(json, i);
			if (cstring_equals(value, S_("mirror"))) {
				flags |= ADDR_FLAG_MIRROR;
			}
			else if (cstring_equals(value, S_("edge"))) {
				flags |= ADDR_FLAG_EDGE;
			}
			else if (cstring_equals(value, S_("repeat"))) {
				flags |= ADDR_FLAG_REPEAT;
			}
		}
	}
	return flags;
}

struct Gfx_Sampler json_read_sampler(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { return (struct Gfx_Sampler){0}; }
	struct Gfx_Sampler result = {
		.mipmap     = json_read_mipmap_mode(json_get(json, S_("mipmap"))),
		.min_filter = json_read_filter_mode(json_get(json, S_("min_filter"))),
		.mag_filter = json_read_filter_mode(json_get(json, S_("mag_filter"))),
		.addr_x     = json_read_addr_flags(json_get(json, S_("wrap_x"))),
		.addr_y     = json_read_addr_flags(json_get(json, S_("wrap_y"))),
		.addr_z     = json_read_addr_flags(json_get(json, S_("wrap_z"))),
	};
	json_read_many_flt(json_get(json, S_("border")), 4, &result.border.x);
	return result;
}

struct Texture_Settings json_read_texture_settings(struct JSON const * json) {
	struct Texture_Settings result = {
		.sublevels = (uint32_t)json_get_number(json, S_("sublevels")),
	};
	return result;
}

struct Target_Format json_read_target_format(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	return (struct Target_Format) {
		.base = {
			.flags = json_read_texture_flags(json_get(json, S_("flags"))),
			.type = json_read_gfx_type(json_get(json, S_("type"))),
		},
		.read = json_get_boolean(json, S_("read")),
	};

	// process errors
	fail: ERR("unknown target format");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Target_Format){0};
}

// ----- ----- ----- ----- -----
//     graphics objects
// ----- ----- ----- ----- -----

struct Handle json_read_target(struct JSON const * json) {
	if (json->type != JSON_OBJECT) { goto fail; }

	struct JSON const * buffers_json = json_get(json, S_("buffers"));
	if (buffers_json->type != JSON_ARRAY) { goto fail; }

	struct Array formats = array_init(sizeof(struct Target_Format));
	formats.allocate = realloc_arena;

	uint32_t const buffers_count = json_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Target_Format const format = json_read_target_format(json_at(buffers_json, i));
		array_push_many(&formats, 1, &format);
	}

	struct Handle result = (struct Handle){0};
	if (formats.count > 0) {
		struct uvec2 size = {0};
		json_read_many_u32(json_get(json, S_("size")), 2, &size.x);
		if (size.x > 0 && size.y > 0) {
			result = gpu_target_init(&(struct GPU_Target_Asset){
				.size = size,
				.formats = formats,
			});
		}
	}

	array_free(&formats);
	return result;

	// process errors
	fail: ERR("failed to read target asset");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Handle){0};
}
