#include "framework/formatter.h"

//
#include "gpu_types.h"

GLenum gpu_vertex_value_type(enum Data_Type value) {
	switch (value) {
		default: break;

		case DATA_TYPE_R8_U:  return GL_UNSIGNED_BYTE;
		case DATA_TYPE_R8_S:  return GL_BYTE;
		case DATA_TYPE_R16_U: return GL_UNSIGNED_SHORT;
		case DATA_TYPE_R16_S: return GL_SHORT;
		case DATA_TYPE_R32_U: return GL_UNSIGNED_INT;
		case DATA_TYPE_R32_S: return GL_INT;
		case DATA_TYPE_R16_F: return GL_HALF_FLOAT;
		case DATA_TYPE_R32_F: return GL_FLOAT;
		case DATA_TYPE_R64_F: return GL_DOUBLE;
	}
	ERR("unknown vertex value type");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_index_value_type(enum Data_Type value) {
	switch (value) {
		default: break;

		case DATA_TYPE_R8_U:  return GL_UNSIGNED_BYTE;
		case DATA_TYPE_R16_U: return GL_UNSIGNED_SHORT;
		case DATA_TYPE_R32_U: return GL_UNSIGNED_INT;
	}
	ERR("unknown index value type");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

enum Data_Type translate_program_data_type(GLint value) {
	switch (value) {
		case GL_UNSIGNED_INT_SAMPLER_2D: return DATA_TYPE_UNIT_U;
		case GL_INT_SAMPLER_2D:          return DATA_TYPE_UNIT_S;
		case GL_SAMPLER_2D:              return DATA_TYPE_UNIT_F;

		// u8, s8
		case GL_UNSIGNED_BYTE: return DATA_TYPE_R8_U;
		case GL_BYTE:          return DATA_TYPE_R8_S;

		// u16, s16
		case GL_UNSIGNED_SHORT: return DATA_TYPE_R16_U;
		case GL_SHORT:          return DATA_TYPE_R16_S;

		// u32
		case GL_UNSIGNED_INT:      return DATA_TYPE_R32_U;
		case GL_UNSIGNED_INT_VEC2: return DATA_TYPE_RG32_U;
		case GL_UNSIGNED_INT_VEC3: return DATA_TYPE_RGB32_U;
		case GL_UNSIGNED_INT_VEC4: return DATA_TYPE_RGBA32_U;

		// s32
		case GL_INT:      return DATA_TYPE_R32_S;
		case GL_INT_VEC2: return DATA_TYPE_RG32_S;
		case GL_INT_VEC3: return DATA_TYPE_RGB32_S;
		case GL_INT_VEC4: return DATA_TYPE_RGBA32_S;

		// floats
		case GL_HALF_FLOAT: return DATA_TYPE_R16_F;

		case GL_FLOAT:      return DATA_TYPE_R32_F;
		case GL_FLOAT_VEC2: return DATA_TYPE_RG32_F;
		case GL_FLOAT_VEC3: return DATA_TYPE_RGB32_F;
		case GL_FLOAT_VEC4: return DATA_TYPE_RGBA32_F;

		case GL_DOUBLE:      return DATA_TYPE_R64_F;
		case GL_DOUBLE_VEC2: return DATA_TYPE_RG64_F;
		case GL_DOUBLE_VEC3: return DATA_TYPE_RGB64_F;
		case GL_DOUBLE_VEC4: return DATA_TYPE_RGBA64_F;

		// matrices
		case GL_FLOAT_MAT2: return DATA_TYPE_MAT2;
		case GL_FLOAT_MAT3: return DATA_TYPE_MAT3;
		case GL_FLOAT_MAT4: return DATA_TYPE_MAT4;
	}
	ERR("unknown GL type");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return DATA_TYPE_NONE;
}

GLint gpu_min_filter_mode(enum Filter_Mode mipmap, enum Filter_Mode texture) {
	switch (mipmap) {
		// choose 1 or lerp 4 texels
		case FILTER_MODE_NONE: switch (texture) {
			case FILTER_MODE_NONE:  return GL_NEAREST;
			case FILTER_MODE_POINT: return GL_NEAREST;
			case FILTER_MODE_LERP:  return GL_LINEAR;
		} break;

		// choose 1 mip-map
		// choose 1 or lerp 4 texels
		case FILTER_MODE_POINT: switch (texture) {
			case FILTER_MODE_NONE:  return GL_NEAREST_MIPMAP_NEAREST;
			case FILTER_MODE_POINT: return GL_NEAREST_MIPMAP_NEAREST;
			case FILTER_MODE_LERP:  return GL_LINEAR_MIPMAP_NEAREST;
		} break;

		// choose 2 mip-maps
		// choose 1 or lerp 4 texels
		// lerp 2 values
		case FILTER_MODE_LERP: switch (texture) {
			case FILTER_MODE_NONE:  return GL_NEAREST_MIPMAP_LINEAR;
			case FILTER_MODE_POINT: return GL_NEAREST_MIPMAP_LINEAR;
			case FILTER_MODE_LERP:  return GL_LINEAR_MIPMAP_LINEAR;
		} break;
	}
	ERR("unknown min filter mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLint gpu_mag_filter_mode(enum Filter_Mode value) {
	// choose 1 or lerp 4 texels
	switch (value) {
		case FILTER_MODE_NONE:  return GL_NEAREST;
		case FILTER_MODE_POINT: return GL_NEAREST;
		case FILTER_MODE_LERP:  return GL_LINEAR;
	}
	ERR("unknown mag filter mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLint gpu_wrap_mode(enum Wrap_Mode value) {
	switch (value) {
		case WRAP_MODE_NONE:          return GL_CLAMP_TO_EDGE;
		case WRAP_MODE_EDGE:          return GL_CLAMP_TO_EDGE;
		case WRAP_MODE_REPEAT:        return GL_REPEAT;
		case WRAP_MODE_BORDER:        return GL_CLAMP_TO_BORDER;
		case WRAP_MODE_MIRROR_EDGE:   return GL_MIRROR_CLAMP_TO_EDGE;
		case WRAP_MODE_MIRROR_REPEAT: return GL_MIRRORED_REPEAT;
	}
	ERR("unknown wrap mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_sized_internal_format(enum Texture_Type texture_type, enum Data_Type data_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR: switch (data_type) {
			default: break;

			case DATA_TYPE_R8_U:    return GL_R8UI;
			case DATA_TYPE_RG8_U:   return GL_RG8UI;
			case DATA_TYPE_RGB8_U:  return GL_RGB8UI;
			case DATA_TYPE_RGBA8_U: return GL_RGBA8UI;

			case DATA_TYPE_R8_UNORM:    return GL_R8;
			case DATA_TYPE_RG8_UNORM:   return GL_RG8;
			case DATA_TYPE_RGB8_UNORM:  return GL_RGB8;
			case DATA_TYPE_RGBA8_UNORM: return GL_RGBA8;

			case DATA_TYPE_R8_S:    return GL_R8I;
			case DATA_TYPE_RG8_S:   return GL_RG8I;
			case DATA_TYPE_RGB8_S:  return GL_RGB8I;
			case DATA_TYPE_RGBA8_S: return GL_RGBA8I;

			case DATA_TYPE_R8_SNORM:    return GL_R8_SNORM;
			case DATA_TYPE_RG8_SNORM:   return GL_RG8_SNORM;
			case DATA_TYPE_RGB8_SNORM:  return GL_RGB8_SNORM;
			case DATA_TYPE_RGBA8_SNORM: return GL_RGBA8_SNORM;

			case DATA_TYPE_R16_U:    return GL_R16UI;
			case DATA_TYPE_RG16_U:   return GL_RG16UI;
			case DATA_TYPE_RGB16_U:  return GL_RGB16UI;
			case DATA_TYPE_RGBA16_U: return GL_RGBA16UI;

			case DATA_TYPE_R16_UNORM:    return GL_R16;
			case DATA_TYPE_RG16_UNORM:   return GL_RG16;
			case DATA_TYPE_RGB16_UNORM:  return GL_RGB16;
			case DATA_TYPE_RGBA16_UNORM: return GL_RGBA16;

			case DATA_TYPE_R16_S:    return GL_R16I;
			case DATA_TYPE_RG16_S:   return GL_RG16I;
			case DATA_TYPE_RGB16_S:  return GL_RGB16I;
			case DATA_TYPE_RGBA16_S: return GL_RGBA16I;

			case DATA_TYPE_R16_SNORM:    return GL_R16_SNORM;
			case DATA_TYPE_RG16_SNORM:   return GL_RG16_SNORM;
			case DATA_TYPE_RGB16_SNORM:  return GL_RGB16_SNORM;
			case DATA_TYPE_RGBA16_SNORM: return GL_RGBA16_SNORM;

			case DATA_TYPE_R32_U:    return GL_R32UI;
			case DATA_TYPE_RG32_U:   return GL_RG32UI;
			case DATA_TYPE_RGB32_U:  return GL_RGB32UI;
			case DATA_TYPE_RGBA32_U: return GL_RGBA32UI;

			case DATA_TYPE_R32_S:    return GL_R32I;
			case DATA_TYPE_RG32_S:   return GL_RG32I;
			case DATA_TYPE_RGB32_S:  return GL_RGB32I;
			case DATA_TYPE_RGBA32_S: return GL_RGBA32I;

			case DATA_TYPE_R32_F:    return GL_R32F;
			case DATA_TYPE_RG32_F:   return GL_RG32F;
			case DATA_TYPE_RGB32_F:  return GL_RGB32F;
			case DATA_TYPE_RGBA32_F: return GL_RGBA32F;
		} break;

		case TEXTURE_TYPE_DEPTH: switch (data_type) {
			default: break;
			case DATA_TYPE_R16_U: return GL_DEPTH_COMPONENT16;
			case DATA_TYPE_R32_U: return GL_DEPTH_COMPONENT24;
			case DATA_TYPE_R32_F: return GL_DEPTH_COMPONENT32F;
		} break;

		case TEXTURE_TYPE_STENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_R8_U: return GL_STENCIL_INDEX8;
		} break;

		case TEXTURE_TYPE_DSTENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_R32_U: return GL_DEPTH24_STENCIL8;
			case DATA_TYPE_R32_F: return GL_DEPTH32F_STENCIL8;
		} break;
	}
	ERR("unknown sized internal format");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_pixel_data_format(enum Texture_Type texture_type, enum Data_Type data_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR: switch (data_type_get_count(data_type)) {
			default: break;

			case 1: return data_type_is_integer(data_type) ? GL_RED_INTEGER  : GL_RED;
			case 2: return data_type_is_integer(data_type) ? GL_RG_INTEGER   : GL_RG;
			case 3: return data_type_is_integer(data_type) ? GL_RGB_INTEGER  : GL_RGB;
			case 4: return data_type_is_integer(data_type) ? GL_RGBA_INTEGER : GL_RGBA;
		} break;

		case TEXTURE_TYPE_DEPTH:    return GL_DEPTH_COMPONENT;
		case TEXTURE_TYPE_STENCIL:  return GL_STENCIL_INDEX;
		case TEXTURE_TYPE_DSTENCIL: return GL_DEPTH_STENCIL;
	}
	ERR("unknown pixel data format");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_pixel_data_type(enum Texture_Type texture_type, enum Data_Type data_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR: switch (data_type_get_element_type(data_type)) {
			default: break;

			case DATA_TYPE_R8_U:     return GL_UNSIGNED_BYTE;
			case DATA_TYPE_R8_UNORM: return GL_UNSIGNED_BYTE;

			case DATA_TYPE_R8_S:     return GL_BYTE;
			case DATA_TYPE_R8_SNORM: return GL_BYTE;

			case DATA_TYPE_R16_U:     return GL_UNSIGNED_SHORT;
			case DATA_TYPE_R16_UNORM: return GL_UNSIGNED_SHORT;

			case DATA_TYPE_R16_S:     return GL_SHORT;
			case DATA_TYPE_R16_SNORM: return GL_SHORT;

			case DATA_TYPE_R32_U: return GL_UNSIGNED_INT;
			case DATA_TYPE_R32_S: return GL_INT;
			case DATA_TYPE_R32_F: return GL_FLOAT;
		} break;

		case TEXTURE_TYPE_DEPTH: switch (data_type) {
			default: break;
			case DATA_TYPE_R16_U: return GL_UNSIGNED_SHORT;
			case DATA_TYPE_R32_U: return GL_UNSIGNED_INT;
			case DATA_TYPE_R32_F: return GL_FLOAT;
		} break;

		case TEXTURE_TYPE_STENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_R8_U: return GL_UNSIGNED_BYTE;
		} break;

		case TEXTURE_TYPE_DSTENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_R32_U: return GL_UNSIGNED_INT_24_8;
			case DATA_TYPE_R32_F: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		} break;
	}
	ERR("unknown pixel data type");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_attachment_point(enum Texture_Type texture_type, uint32_t index) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR: if (index < 32) {
			return GL_COLOR_ATTACHMENT0 + index;
		} break;

		case TEXTURE_TYPE_DEPTH:    return GL_DEPTH_ATTACHMENT;
		case TEXTURE_TYPE_STENCIL:  return GL_STENCIL_ATTACHMENT;
		case TEXTURE_TYPE_DSTENCIL: return GL_DEPTH_STENCIL_ATTACHMENT;
	}
	ERR("unknown attachment point");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_comparison_op(enum Comparison_Op value) {
	switch (value) {
		case COMPARISON_OP_NONE:       break;
		case COMPARISON_OP_FALSE:      return GL_NEVER;
		case COMPARISON_OP_TRUE:       return GL_ALWAYS;
		case COMPARISON_OP_LESS:       return GL_LESS;
		case COMPARISON_OP_EQUAL:      return GL_EQUAL;
		case COMPARISON_OP_MORE:       return GL_GREATER;
		case COMPARISON_OP_NOT_EQUAL:  return GL_NOTEQUAL;
		case COMPARISON_OP_LESS_EQUAL: return GL_LEQUAL;
		case COMPARISON_OP_MORE_EQUAL: return GL_GEQUAL;
	}
	ERR("unknown comparison operation");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_cull_mode(enum Cull_Mode value) {
	switch (value) {
		case CULL_MODE_NONE:  break;
		case CULL_MODE_BACK:  return GL_BACK;
		case CULL_MODE_FRONT: return GL_FRONT;
		case CULL_MODE_BOTH:  return GL_FRONT_AND_BACK;
	}
	ERR("unknown cull mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_winding_order(enum Winding_Order value) {
	switch (value) {
		case WINDING_ORDER_POSITIVE: return GL_CCW;
		case WINDING_ORDER_NEGATIVE: return GL_CW;
	}
	ERR("unknown winding order");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_stencil_op(enum Stencil_Op value) {
	switch (value) {
		case STENCIL_OP_ZERO:      return GL_ZERO;
		case STENCIL_OP_KEEP:      return GL_KEEP;
		case STENCIL_OP_REPLACE:   return GL_REPLACE;
		case STENCIL_OP_INVERT:    return GL_INVERT;
		case STENCIL_OP_INCR:      return GL_INCR;
		case STENCIL_OP_DECR:      return GL_DECR;
		case STENCIL_OP_INCR_WRAP: return GL_INCR_WRAP;
		case STENCIL_OP_DECR_WRAP: return GL_DECR_WRAP;
	}
	ERR("unknown stencil operation");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

struct GPU_Blend_Mode gpu_blend_mode(enum Blend_Mode value) {
	switch (value) {
		case BLEND_MODE_NONE: break;

		case BLEND_MODE_MIX: return (struct GPU_Blend_Mode){
			.mask = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, },
			.color_src = GL_SRC_ALPHA, /**/ .color_op = GL_FUNC_ADD, /**/ .color_dst = GL_ONE_MINUS_SRC_ALPHA,
			.alpha_src = GL_ONE,       /**/ .alpha_op = GL_MAX,      /**/ .alpha_dst = GL_ONE,
		};

		case BLEND_MODE_PMA: return (struct GPU_Blend_Mode){
			.mask = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, },
			.color_src = GL_ONE, /**/ .color_op = GL_FUNC_ADD, /**/ .color_dst = GL_ONE_MINUS_SRC_ALPHA,
			.alpha_src = GL_ONE, /**/ .alpha_op = GL_MAX,      /**/ .alpha_dst = GL_ONE,
		};

		case BLEND_MODE_ADD: return (struct GPU_Blend_Mode){
			.mask = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, },
			.color_src = GL_ONE, /**/ .color_op = GL_FUNC_ADD, /**/ .color_dst = GL_ONE,
			.alpha_src = GL_ONE, /**/ .alpha_op = GL_FUNC_ADD, /**/ .alpha_dst = GL_ONE,
		};

		case BLEND_MODE_SUB: return (struct GPU_Blend_Mode){
			.mask = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, },
			.color_src = GL_ONE, /**/.color_op = GL_FUNC_REVERSE_SUBTRACT, /**/ .color_dst = GL_ONE,
			.alpha_src = GL_ONE, /**/.alpha_op = GL_FUNC_REVERSE_SUBTRACT, /**/ .alpha_dst = GL_ONE,
		};

		case BLEND_MODE_MUL: return (struct GPU_Blend_Mode){
			.mask = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, },
			.color_src = GL_ZERO, /**/ .color_op = GL_FUNC_ADD, /**/ .color_dst = GL_SRC_COLOR,
			.alpha_src = GL_ZERO, /**/ .alpha_op = GL_FUNC_ADD, /**/ .alpha_dst = GL_SRC_ALPHA,
		};

		case BLEND_MODE_SCR: return (struct GPU_Blend_Mode){
			.mask = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, },
			.color_src = GL_ONE_MINUS_DST_COLOR, /**/ .color_op = GL_FUNC_ADD, /**/ .color_dst = GL_ONE,
			.alpha_src = GL_ONE_MINUS_DST_ALPHA, /**/ .alpha_op = GL_FUNC_ADD, /**/ .alpha_dst = GL_ONE,
		};
	}
	ERR("unknown blend function");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct GPU_Blend_Mode){0};
}

struct GPU_Depth_Mode gpu_depth_mode(enum Depth_Mode value, bool reversed_z) {
	switch (value) {
		case DEPTH_MODE_NONE: break;

		case DEPTH_MODE_TRANSPARENT: return (struct GPU_Depth_Mode){
			.mask = GL_TRUE, /**/ .op = reversed_z ? GL_GREATER : GL_LESS,
		};

		case DEPTH_MODE_OPAQUE: return (struct GPU_Depth_Mode){
			.mask = GL_TRUE, /**/ .op = reversed_z ? GL_GREATER : GL_LESS,
		};
	}
	ERR("unknown depth function");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct GPU_Depth_Mode){0};
}

GLint gpu_swizzle_op(enum Swizzle_Op value, uint32_t index) {
	switch (value) {
		case SWIZZLE_OP_NONE: switch (index) {
			case 0: return GL_RED;
			case 1: return GL_GREEN;
			case 2: return GL_BLUE;
			case 3: return GL_ALPHA;
		} break;

		case SWIZZLE_OP_0: return GL_ZERO;
		case SWIZZLE_OP_1: return GL_ONE;
		case SWIZZLE_OP_R: return GL_RED;
		case SWIZZLE_OP_G: return GL_GREEN;
		case SWIZZLE_OP_B: return GL_BLUE;
		case SWIZZLE_OP_A: return GL_ALPHA;
	}
	ERR("unknown swizzle operation");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_mesh_mode(enum Mesh_Mode value) {
	switch (value) {
		case MESH_MODE_NONE: break;

		case MESH_MODE_POINTS:         return GL_POINTS;
		case MESH_MODE_LINES:          return GL_LINES;
		case MESH_MODE_LINE_STRIP:     return GL_LINE_STRIP;
		case MESH_MODE_LINE_LOOP:      return GL_LINE_LOOP;
		case MESH_MODE_TRIANGLES:      return GL_TRIANGLES;
		case MESH_MODE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
		case MESH_MODE_TRIANGLE_FAN:   return GL_TRIANGLE_FAN;
	}
	ERR("unknown mesh mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_buffer_mode(enum Buffer_Mode value) {
	switch (value) {
		case BUFFER_MODE_NONE: break;

		case BUFFER_MODE_UNIFORM: return GL_UNIFORM_BUFFER;
		case BUFFER_MODE_STORAGE: return GL_SHADER_STORAGE_BUFFER;
	}
	ERR("unknown mesh mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return GL_NONE;
}
