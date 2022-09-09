#include "framework/assets/json.h"
#include "framework/logger.h"

#include "json_read.h"

//
#include "json_read_types.h"

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
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		};
	}

	if (cstring_equals(type, S_("depth_r32_f"))) {
		return (struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_DEPTH,
			.data_type = DATA_TYPE_R32_F,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		};
	}

	fail: logger_to_console("unknown texture type\n"); DEBUG_BREAK();
	return (struct Texture_Parameters){0};
}

struct Texture_Settings json_read_texture_settings(struct JSON const * json) {
	struct Texture_Settings result = {
		.mipmap        = json_read_filter_mode(json_get(json, S_("mipmap"))),
		.minification  = json_read_filter_mode(json_get(json, S_("minification"))),
		.magnification = json_read_filter_mode(json_get(json, S_("magnification"))),
		.wrap_x        = json_read_wrap_mode(json_get(json, S_("wrap_x"))),
		.wrap_y        = json_read_wrap_mode(json_get(json, S_("wrap_y"))),
	};
	json_read_many_flt(json_get(json, S_("border")), 4, &result.border.x);
	return result;
}
